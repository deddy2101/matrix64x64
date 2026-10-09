#include "BleManager.h"
#include "CommandHandler.h"
#include <Preferences.h>

BleManager::BleManager()
    : _settings(nullptr)
    , _cmdHandler(nullptr)
    , _server(nullptr)
    , _rxChar(nullptr)
    , _txChar(nullptr)
    , _unlockChar(nullptr)
    , _mutex(nullptr)
    , _rxQueue(nullptr)
    , _failCount(0)
    , _lockUntil(0)
    , _unlockUntil(0)
    , _pairingActive(false)
    , _pairingCode(0)
    , _pairingHandle(0)
    , _lastConnHandle(0)
    , _pairingStart(0)
{}

void BleManager::loadStaticPin() {
    Preferences prefs;
    prefs.begin("ble", true);
    _staticPin = prefs.getString("pin", BLE_DEFAULT_STATIC_PIN);
    prefs.end();
    if (_staticPin.length() != 6) _staticPin = BLE_DEFAULT_STATIC_PIN;
}

bool BleManager::setPairingPin(const String& pin) {
    if (pin.length() != 6) return false;
    for (size_t i = 0; i < pin.length(); i++) {
        if (pin[i] < '0' || pin[i] > '9') return false;
    }
    Preferences prefs;
    prefs.begin("ble", false);
    prefs.putString("pin", pin);
    prefs.end();
    _staticPin = pin;
    return true;
}

void BleManager::begin(Settings* settings, CommandHandler* cmdHandler) {
    _settings = settings;
    _cmdHandler = cmdHandler;

    loadStaticPin();

    _mutex = xSemaphoreCreateMutex();
    _rxQueue = xQueueCreate(16, sizeof(RxItem));

    NimBLEDevice::init(_settings->getDeviceName());
    NimBLEDevice::setMTU(247);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    // Sicurezza: bonding + MITM + secure connections, PIN mostrato sulla matrice
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

    _server = NimBLEDevice::createServer();
    _server->setCallbacks(this, false);
    _server->advertiseOnDisconnect(false);  // gestito da startAdvertisingIfFree()

    NimBLEService* service = _server->createService(BLE_SERVICE_UUID);
    _rxChar = service->createCharacteristic(
        BLE_RX_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR |
        NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN);
    _rxChar->setCallbacks(this);
    _txChar = service->createCharacteristic(
        BLE_TX_UUID,
        NIMBLE_PROPERTY::NOTIFY);
    _txChar->setCallbacks(this);
    // Sblocco con PIN statico: volutamente NON cifrata (serve prima del pairing)
    _unlockChar = service->createCharacteristic(
        BLE_UNLOCK_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
    _unlockChar->setCallbacks(this);
    service->start();

    // Advertising: UUID del servizio nel pacchetto, nome nella scan response
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(BLE_SERVICE_UUID);
    adv->setScanResponse(true);
    adv->start();

    DEBUG_PRINTF("[BLE] Advertising as \"%s\"\n", _settings->getDeviceName());
}

// ═══════════════════════════════════════════
// Slot client
// ═══════════════════════════════════════════

BleManager::Client* BleManager::findClient(uint16_t handle) {
    for (auto& c : _clients) {
        if (c.active && c.handle == handle) return &c;
    }
    return nullptr;
}

BleManager::Client* BleManager::allocClient(uint16_t handle) {
    Client* result = nullptr;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    for (auto& c : _clients) {
        if (!c.active) {
            c.handle = handle;
            c.authed = false;
            c.subscribed = false;
            c.welcomeSent = false;
            c.unlocked = false;
            c.known = false;
            c.wantSecurity = false;
            c.closeAfterMsg = false;
            c.pendingMsg = MSG_NONE;
            c.pendingArg = 0;
            c.rx = "";
            c.rxOverflow = false;
            c.tx = "";
            c.txPos = 0;
            c.active = true;
            result = &c;
            break;
        }
    }
    xSemaphoreGive(_mutex);
    return result;
}

void BleManager::releaseClient(uint16_t handle) {
    xSemaphoreTake(_mutex, portMAX_DELAY);
    Client* c = findClient(handle);
    if (c) {
        c->active = false;
        c->authed = false;
        c->subscribed = false;
        c->unlocked = false;
        c->rx = "";
    }
    xSemaphoreGive(_mutex);
}

void BleManager::startAdvertisingIfFree() {
    if (_server->getConnectedCount() < BLE_MAX_CLIENTS) {
        NimBLEDevice::startAdvertising();
    }
}

void BleManager::endPairing() {
    _pairingActive = false;
    _pairingHandle = 0;
    _unlockUntil = 0;
}

void BleManager::forgetBonds() {
    NimBLEDevice::deleteAllBonds();
    DEBUG_PRINTLN(F("[BLE] All bonds deleted"));
}

// ═══════════════════════════════════════════
// Callback NimBLE (task host: NIENTE comandi qui)
// ═══════════════════════════════════════════

void BleManager::onConnect(NimBLEServer* server, ble_gap_conn_desc* desc) {
    DEBUG_PRINTF("[BLE] Connect, handle %u\n", desc->conn_handle);

    Client* c = allocClient(desc->conn_handle);
    if (!c) {
        DEBUG_PRINTLN(F("[BLE] No free slot, disconnecting"));
        server->disconnect(desc->conn_handle);
        return;
    }
    c->peerAddr = desc->peer_id_addr;
    _lastConnHandle = desc->conn_handle;

    // Nessuna cifratura né PIN qui: lo stato (KNOWN/LOCKED) viene comunicato
    // quando il telefono si iscrive alla caratteristica UNLOCK.
    startAdvertisingIfFree();  // spazio per altri telefoni
}

void BleManager::onDisconnect(NimBLEServer* server, ble_gap_conn_desc* desc) {
    DEBUG_PRINTF("[BLE] Disconnect, handle %u\n", desc->conn_handle);
    if (_pairingHandle == desc->conn_handle) {
        endPairing();
    }
    releaseClient(desc->conn_handle);
    startAdvertisingIfFree();
}

// Chiamata dalla libreria solo quando serve un PIN da mostrare all'utente.
// Il PIN dinamico viene concesso SOLO a chi ha appena dato il PIN statico
// giusto; altrimenti il pairing non può riuscire (nessuno conosce il codice).
// Gira nel task host: imposta solo lo stato, il display lo disegna il loop().
uint32_t BleManager::onPassKeyRequest() {
    unsigned long now = millis();
    uint32_t code = esp_random() % 1000000;

    if (_unlockUntil != 0 && (long)(_unlockUntil - now) > 0) {
        _pairingCode = code;
        _pairingStart = now;
        _pairingActive = true;
        DEBUG_PRINTLN(F("[BLE] Pairing: PIN shown"));
    } else {
        DEBUG_PRINTLN(F("[BLE] Pairing refused: PIN statico non dato"));
    }
    return code;
}

void BleManager::onAuthenticationComplete(ble_gap_conn_desc* desc) {
    Client* c = findClient(desc->conn_handle);
    bool wasPairingHandle = (_pairingHandle == desc->conn_handle);
    if (!c) return;

    if (desc->sec_state.encrypted && desc->sec_state.authenticated) {
        DEBUG_PRINTF("[BLE] Handle %u authenticated\n", desc->conn_handle);
        c->authed = true;
        c->unlocked = true;
        if (wasPairingHandle || _pairingActive) endPairing();
        return;
    }

    DEBUG_PRINTF("[BLE] Handle %u authentication failed\n", desc->conn_handle);
    if (wasPairingHandle) endPairing();

    if (c->known && !c->unlocked) {
        // Il display ricorda questo telefono ma il telefono ha dimenticato
        // l'associazione: cancella il bond e chiedi il PIN statico.
        NimBLEDevice::deleteBond(NimBLEAddress(c->peerAddr));
        c->known = false;
        c->pendingArg = 0;
        c->pendingMsg = MSG_LOCKED;
    } else {
        _server->disconnect(desc->conn_handle);
    }
}

void BleManager::onSubscribe(NimBLECharacteristic* chr, ble_gap_conn_desc* desc, uint16_t subValue) {
    Client* c = findClient(desc->conn_handle);
    if (!c) return;

    if (chr == _txChar) {
        c->subscribed = (subValue & 1) != 0;
    } else if (chr == _unlockChar && (subValue & 1) && !c->authed) {
        // Telefono già associato: cifratura con le chiavi salvate, nessun PIN.
        // Altrimenti serve il PIN statico.
        if (NimBLEDevice::isBonded(NimBLEAddress(c->peerAddr))) {
            c->known = true;
            c->wantSecurity = true;
            c->pendingMsg = MSG_KNOWN;
        } else {
            c->pendingMsg = MSG_LOCKED;
        }
    }
}

void BleManager::handleUnlockWrite(Client* c, const std::string& value) {
    if (c->authed || c->unlocked) return;

    unsigned long now = millis();
    if (_lockUntil != 0 && (long)(_lockUntil - now) > 0) {
        c->pendingArg = (uint16_t)((_lockUntil - now) / 1000 + 1);
        c->closeAfterMsg = true;
        c->pendingMsg = MSG_LOCKOUT;
        return;
    }

    // Confronto a tempo costante
    String pin(value.c_str());
    pin.trim();
    uint8_t diff = (pin.length() != _staticPin.length());
    for (size_t i = 0; i < _staticPin.length(); i++) {
        char a = i < pin.length() ? pin[i] : 0;
        diff |= (uint8_t)(a ^ _staticPin[i]);
    }

    if (diff == 0) {
        _failCount = 0;
        c->unlocked = true;
        // Da ora il PIN dinamico può comparire per la durata del pairing
        _unlockUntil = now + PAIRING_TIMEOUT_MS;
        _pairingHandle = c->handle;
        c->wantSecurity = true;
        c->pendingMsg = MSG_OK;
        DEBUG_PRINTF("[BLE] Handle %u: PIN statico ok\n", c->handle);
        return;
    }

    _failCount++;
    DEBUG_PRINTF("[BLE] Handle %u: PIN statico errato (%u/%u)\n",
                 c->handle, _failCount, MAX_PIN_ATTEMPTS);
    if (_failCount >= MAX_PIN_ATTEMPTS) {
        _failCount = 0;
        _lockUntil = now + LOCKOUT_MS;
        c->pendingArg = (uint16_t)(LOCKOUT_MS / 1000);
        c->closeAfterMsg = true;
        c->pendingMsg = MSG_LOCKOUT;
    } else {
        c->pendingArg = MAX_PIN_ATTEMPTS - _failCount;
        c->pendingMsg = MSG_WRONG;
    }
}

void BleManager::onWrite(NimBLECharacteristic* chr, ble_gap_conn_desc* desc) {
    Client* c = findClient(desc->conn_handle);
    if (!c) return;

    std::string value = chr->getValue();

    if (chr == _unlockChar) {
        handleUnlockWrite(c, value);
        return;
    }

    if (!c->authed) return;  // ignora chi non è autenticato

    for (char ch : value) {
        if (ch == '\n') {
            if (c->rxOverflow) {
                c->rxOverflow = false;
                c->rx = "";
                // avvisa l'app dal loop: riga scartata
                String* err = new String("\x01" "ERR,message too large");
                RxItem item = { c->handle, err };
                if (xQueueSend(_rxQueue, &item, 0) != pdTRUE) delete err;
                continue;
            }
            c->rx.trim();
            if (c->rx.length() > 0) {
                String* line = new String(c->rx);
                RxItem item = { c->handle, line };
                if (xQueueSend(_rxQueue, &item, 0) != pdTRUE) {
                    delete line;  // coda piena: comando perso
                }
            }
            c->rx = "";
        } else if (!c->rxOverflow) {
            if (c->rx.length() >= MAX_LINE) {
                c->rxOverflow = true;
                c->rx = "";
            } else {
                c->rx += ch;
            }
        }
    }
}

// ═══════════════════════════════════════════
// Loop: esecuzione comandi e invio
// ═══════════════════════════════════════════

void BleManager::queueTx(Client* c, const String& msg) {
    c->tx += msg;
    c->tx += '\n';
}

void BleManager::sendWelcome(Client* c) {
    queueTx(c, "WELCOME,LED Matrix Controller");
    queueTx(c, _cmdHandler->getStatusResponse());
    c->welcomeSent = true;
}

void BleManager::sendUnlockMessage(Client* c) {
    // exchange atomico: il task host può scrivere un nuovo messaggio nel frattempo
    uint8_t msg = __atomic_exchange_n(&c->pendingMsg, (uint8_t)MSG_NONE, __ATOMIC_SEQ_CST);
    if (msg == MSG_NONE) return;

    char text[24];
    switch (msg) {
        case MSG_KNOWN:   strcpy(text, "KNOWN"); break;
        case MSG_LOCKED:  strcpy(text, "LOCKED"); break;
        case MSG_OK:      strcpy(text, "OK"); break;
        case MSG_WRONG:   snprintf(text, sizeof(text), "ERR,wrong,%u", c->pendingArg); break;
        case MSG_LOCKOUT: snprintf(text, sizeof(text), "ERR,locked,%u", c->pendingArg); break;
        default: return;
    }

    struct os_mbuf* om = ble_hs_mbuf_from_flat(text, strlen(text));
    if (!om) {
        c->pendingMsg = msg;  // riprova al prossimo giro
        return;
    }
    if (ble_gattc_notify_custom(c->handle, _unlockChar->getHandle(), om) != 0) {
        c->pendingMsg = msg;
    }
}

void BleManager::flushTx(Client* c) {
    if (c->txPos >= c->tx.length()) {
        c->tx = "";
        c->txPos = 0;
        return;
    }

    uint16_t mtu = ble_att_mtu(c->handle);
    size_t payload = (mtu > 23 ? mtu : 23) - 3;
    const uint16_t attrHandle = _txChar->getHandle();

    for (int i = 0; i < MAX_NOTIFY_PER_UPDATE && c->txPos < c->tx.length(); i++) {
        size_t n = min(payload, c->tx.length() - c->txPos);
        struct os_mbuf* om = ble_hs_mbuf_from_flat(c->tx.c_str() + c->txPos, n);
        if (!om) break;  // niente mbuf: riprova al prossimo giro
        int rc = ble_gattc_notify_custom(c->handle, attrHandle, om);
        if (rc != 0) break;  // BLE_HS_ENOMEM / congestione: riprova (om già liberato dallo stack)
        c->txPos += n;
    }

    if (c->txPos >= c->tx.length()) {
        c->tx = "";
        c->txPos = 0;
    }
}

void BleManager::update() {
    unsigned long now = millis();

    // Timeout pairing: chiudi il telefono che non ha digitato il PIN
    if (_pairingActive && now - _pairingStart > PAIRING_TIMEOUT_MS) {
        uint16_t h = _pairingHandle;
        endPairing();
        if (h) _server->disconnect(h);
    }
    // Finestra del PIN dinamico scaduta senza che sia partito il pairing
    if (_unlockUntil != 0 && (long)(now - _unlockUntil) >= 0) {
        _unlockUntil = 0;
    }

    // Comandi ricevuti
    RxItem item;
    for (int i = 0; i < MAX_CMDS_PER_UPDATE && xQueueReceive(_rxQueue, &item, 0) == pdTRUE; i++) {
        String line = *item.line;
        delete item.line;

        Client* c = findClient(item.handle);
        if (!c) continue;  // client sparito nel frattempo

        // Marcatore interno (0x01): messaggio di errore generato dal trasporto
        if (line.length() > 0 && line[0] == '\x01') {
            queueTx(c, line.substring(1));
            continue;
        }

        // Non stampare i dati OTA / immagini completi (troppo grandi)
        if (line.startsWith("ota,data,") || line.startsWith("image,upload,")) {
            DEBUG_PRINTF("[BLE] RX #%u: %.20s... [%u bytes]\n", c->handle, line.c_str(), (unsigned)line.length());
        } else {
            DEBUG_PRINTF("[BLE] RX #%u: %s\n", c->handle, line.c_str());
        }

        String response = _cmdHandler->processCommand(line);
        if (!response.isEmpty()) {
            queueTx(c, response);
        }
    }

    // Sblocco, cifratura, benvenuto + invio
    for (auto& c : _clients) {
        if (!c.active) continue;

        sendUnlockMessage(&c);

        if (c.closeAfterMsg && c.pendingMsg == MSG_NONE) {
            c.closeAfterMsg = false;
            _server->disconnect(c.handle);
            continue;
        }

        if (c.wantSecurity && c.pendingMsg == MSG_NONE) {
            c.wantSecurity = false;
            NimBLEDevice::startSecurity(c.handle);
        }

        if (c.authed && c.subscribed && !c.welcomeSent) sendWelcome(&c);
        if (c.authed && c.subscribed) flushTx(&c);
    }
}

void BleManager::broadcast(const String& message) {
    for (auto& c : _clients) {
        if (!c.active || !c.authed || !c.subscribed) continue;
        // Client lento: i broadcast sono stati che verranno superati, meglio scartarli
        if (c.tx.length() - c.txPos > MAX_TX_PENDING) continue;
        queueTx(&c, message);
    }
}
