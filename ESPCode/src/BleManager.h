#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "Notifier.h"
#include "Settings.h"
#include "Debug.h"

// UUID del servizio (contratto con l'app, vedi docs/BLE_MIGRATION_PLAN.md)
#define BLE_SERVICE_UUID "6f1a0001-8b5c-4d2e-9a3f-1c2d3e4f5a6b"
#define BLE_RX_UUID      "6f1a0002-8b5c-4d2e-9a3f-1c2d3e4f5a6b"  // app → ESP (comandi, cifrata)
#define BLE_TX_UUID      "6f1a0003-8b5c-4d2e-9a3f-1c2d3e4f5a6b"  // ESP → app (risposte)
#define BLE_UNLOCK_UUID  "6f1a0004-8b5c-4d2e-9a3f-1c2d3e4f5a6b"  // PIN statico (non cifrata)

// PIN statico di fabbrica (6 cifre). CAMBIALO prima di produrre i display:
// chi lo conosce può far comparire il PIN dinamico sul display. Dopo il primo
// accesso si cambia dall'app (comando ble,setpin,NNNNNN) ed è salvato in NVS.
#define BLE_DEFAULT_STATIC_PIN "127001"

#define BLE_MAX_CLIENTS 3

class CommandHandler;

/**
 * BleManager - Trasporto BLE (GATT) per il protocollo CSV a righe.
 *
 * ACCESSO (due PIN per un telefono nuovo, nessuno per uno già associato):
 *   1. il telefono scrive il PIN STATICO sulla caratteristica UNLOCK
 *   2. se giusto, il display mostra il PIN DINAMICO in sovrimpressione e parte
 *      il pairing: il telefono digita il PIN dinamico
 *   Senza PIN statico non compare nulla e il pairing viene rifiutato.
 *   3 errori di fila → blocco di 5 minuti.
 *
 * Messaggi sulla caratteristica UNLOCK (testo, ESP → app):
 *   KNOWN              telefono già associato: nessun PIN, cifratura con le chiavi salvate
 *   LOCKED             serve il PIN statico
 *   OK                 PIN statico giusto: digita il PIN dinamico mostrato sul display
 *   ERR,wrong,N        PIN statico sbagliato, N tentativi rimasti
 *   ERR,locked,S       troppi tentativi, riprova tra S secondi
 *
 * THREAD SAFETY: le callback NimBLE girano nel task host, non nel loop().
 * Le callback si limitano a riassemblare le righe e metterle in una coda
 * FreeRTOS; i comandi vengono eseguiti SOLO in update() (chiamato dal loop()).
 * Anche tutti gli invii (risposte, broadcast, messaggi UNLOCK) partono da update().
 */
class BleManager : public Notifier,
                   public NimBLEServerCallbacks,
                   public NimBLECharacteristicCallbacks {
public:
    BleManager();

    void begin(Settings* settings, CommandHandler* cmdHandler);
    void update();   // da chiamare nel loop()

    // Notifier: accoda il messaggio a tutti i client autenticati e iscritti
    void broadcast(const String& message) override;

    // Cambia il PIN statico (6 cifre) e lo salva in NVS
    bool setPairingPin(const String& pin) override;

    // Pairing: true mentre va mostrato il PIN dinamico sulla matrice
    bool isPairingActive() const { return _pairingActive; }
    uint32_t getPairingCode() const { return _pairingCode; }

    void forgetBonds();

private:
    // Messaggi UNLOCK in attesa di invio (scritti dal task host, letti dal loop)
    enum UnlockMsg : uint8_t {
        MSG_NONE = 0, MSG_KNOWN, MSG_LOCKED, MSG_OK, MSG_WRONG, MSG_LOCKOUT
    };

    struct Client {
        volatile bool active = false;
        volatile bool authed = false;       // cifrata + autenticata
        volatile bool subscribed = false;   // notify abilitate su TX
        volatile bool welcomeSent = false;
        volatile bool unlocked = false;     // ha dato il PIN statico giusto
        volatile bool known = false;        // telefono già associato (bond)
        volatile bool wantSecurity = false; // da avviare la cifratura (loop)
        volatile bool closeAfterMsg = false;
        volatile uint8_t pendingMsg = MSG_NONE;
        volatile uint16_t pendingArg = 0;
        uint16_t handle = 0;
        ble_addr_t peerAddr = {};
        String rx;                          // riga in riassemblaggio (task host)
        bool rxOverflow = false;            // riga troppo lunga: scarta fino al '\n'
        String tx;                          // dati da inviare (loop)
        size_t txPos = 0;
    };

    struct RxItem {
        uint16_t handle;
        String* line;
    };

    Settings* _settings;
    CommandHandler* _cmdHandler;
    NimBLEServer* _server;
    NimBLECharacteristic* _rxChar;
    NimBLECharacteristic* _txChar;
    NimBLECharacteristic* _unlockChar;

    Client _clients[BLE_MAX_CLIENTS];
    SemaphoreHandle_t _mutex;     // protegge l'assegnazione degli slot
    QueueHandle_t _rxQueue;

    String _staticPin;
    uint8_t _failCount;
    volatile unsigned long _lockUntil;    // 0 = nessun blocco
    volatile unsigned long _unlockUntil;  // finestra in cui è concesso il PIN dinamico

    volatile bool _pairingActive;
    volatile uint32_t _pairingCode;
    volatile uint16_t _pairingHandle;
    volatile uint16_t _lastConnHandle;
    unsigned long _pairingStart;

    static constexpr size_t MAX_LINE = 16384;     // lunghezza massima riga RX
    static constexpr size_t MAX_TX_PENDING = 4096; // oltre, i broadcast vengono scartati
    static constexpr int MAX_NOTIFY_PER_UPDATE = 8;
    static constexpr int MAX_CMDS_PER_UPDATE = 4;
    static constexpr unsigned long PAIRING_TIMEOUT_MS = 60000;
    static constexpr uint8_t MAX_PIN_ATTEMPTS = 3;
    static constexpr unsigned long LOCKOUT_MS = 300000;  // 5 minuti

    Client* findClient(uint16_t handle);
    Client* allocClient(uint16_t handle);
    void releaseClient(uint16_t handle);
    void queueTx(Client* c, const String& msg);
    void flushTx(Client* c);
    void sendWelcome(Client* c);
    void sendUnlockMessage(Client* c);
    void handleUnlockWrite(Client* c, const std::string& value);
    void endPairing();
    void startAdvertisingIfFree();
    void loadStaticPin();

    // NimBLEServerCallbacks
    void onConnect(NimBLEServer* server, ble_gap_conn_desc* desc) override;
    void onDisconnect(NimBLEServer* server, ble_gap_conn_desc* desc) override;
    uint32_t onPassKeyRequest() override;
    void onAuthenticationComplete(ble_gap_conn_desc* desc) override;

    // NimBLECharacteristicCallbacks
    void onWrite(NimBLECharacteristic* chr, ble_gap_conn_desc* desc) override;
    void onSubscribe(NimBLECharacteristic* chr, ble_gap_conn_desc* desc, uint16_t subValue) override;
};

#endif // BLE_MANAGER_H
