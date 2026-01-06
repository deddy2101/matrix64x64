# 🔮 LED Matrix Firmware Server

Sistema completo per distribuzione e gestione firmware OTA per dispositivi ESP32.

## 🌟 Features

✅ **Interfaccia Pubblica (Porta 80)** - Download firmware (sola lettura)
✅ **Admin Panel (Porta 81)** - Upload e gestione firmware (protetto con password)
✅ **Docker Ready** - Deploy con un comando
✅ **Volume Persistente** - Dati salvati in `./data/binaries`
✅ **Auto Manifest** - Genera automaticamente `manifest.json`
✅ **API REST** - Endpoint per app Flutter
✅ **Versioning** - Supporta formato SemVer + Build (X.Y.Z+N)
✅ **MD5 Checksum** - Verifica integrità firmware

---

## 🚀 Quick Start

```bash
# 1. Clona repo
git clone https://github.com/tuousername/firmware-server.git
cd firmware-server

# 2. Configura credenziali (opzionale)
cp .env.example .env
# Modifica .env con le tue credenziali

# 3. Avvia server
docker-compose up -d

# 4. Accedi alle interfacce
# Pubblica: http://localhost (porta 80)
# Admin:    http://localhost:81 (porta 81)
```

---

## 📁 Struttura Progetto

```
firmware-server/
├── docker-compose.yml       # Orchestrazione 2 servizi
├── .env.example             # Template credenziali
├── .gitignore
├── README.md
│
├── public/                  # Nginx (porta 80)
│   ├── nginx.conf
│   └── index.html           # UI pubblica
│
├── admin/                   # Node.js (porta 81)
│   ├── Dockerfile
│   ├── package.json
│   ├── server.js            # API server
│   └── public/
│       └── index.html       # UI admin
│
└── data/                    # Volume persistente
    └── binaries/            # Firmware storage
        ├── 1.0.0+1/
        │   └── firmware.bin
        ├── 1.1.0+3/
        │   └── firmware.bin
        └── manifest.json    # Auto-generato
```

---

## 🌐 Interfacce

### 🔓 Porta 80 - Public Interface

**URL**: `http://binaries.server21.it`

Interfaccia pubblica per visualizzare e scaricare firmware.

**Features:**
- ✅ Visualizzazione versioni disponibili
- ✅ Download diretto firmware
- ✅ Informazioni su dimensione, MD5, data rilascio
- ✅ Badge "LATEST" per versione più recente
- ✅ Nessuna autenticazione richiesta

**API Endpoints:**
```
GET /api/manifest           # Lista versioni JSON
GET /binaries/{v}/firmware.bin  # Download firmware
```

### 🔐 Porta 81 - Admin Panel

**URL**: `http://binaries.server21.it:81`

Pannello amministratore per gestire firmware.

**Credenziali Default:**
- Username: `admin`
- Password: `changeme`

**Features:**
- ✅ Upload nuovo firmware (.bin)
- ✅ Eliminazione versioni
- ✅ Statistiche real-time
- ✅ Auto-generazione manifest
- ✅ Protezione Basic Auth

---

## 📤 Upload Firmware

### Via Admin Panel (Consigliato)

1. Apri `http://server:81`
2. Login con credenziali admin
3. Compila form:
   - Versione: `1.2.0+5`
   - File: Seleziona `.bin`
4. Click "Upload Firmware"
5. Il manifest viene auto-generato

### Via API (Programmatico)

```bash
curl -X POST http://server:81/api/upload \
  -u admin:changeme \
  -F "version=1.2.0+5" \
  -F "firmware=@firmware.bin"
```

---

## 🐳 Deploy Produzione

### 1. Setup DNS

Punta dominio al tuo server:
```
binaries.server21.it → IP_SERVER
```

### 2. Configura Credenziali

```bash
# Crea file .env
cp .env.example .env

# Modifica credenziali
nano .env
```

```env
ADMIN_USERNAME=il_tuo_username
ADMIN_PASSWORD=password_super_sicura
```

### 3. Avvia Container

```bash
docker-compose up -d
```

### 4. Verifica Funzionamento

```bash
# Public interface
curl http://binaries.server21.it/health

# Admin interface
curl http://binaries.server21.it:81/health

# Manifest
curl http://binaries.server21.it/api/manifest
```

---

## 🔒 Sicurezza

### HTTPS (Opzionale ma Consigliato)

Aggiungi reverse proxy con Certbot:

```yaml
# docker-compose.yml
services:
  nginx-proxy:
    image: nginx:alpine
    ports:
      - "443:443"
    volumes:
      - ./ssl:/etc/nginx/ssl
```

Oppure usa Cloudflare per SSL automatico.

### Firewall

```bash
# Apri solo porte necessarie
ufw allow 80/tcp
ufw allow 81/tcp
ufw enable
```

### Backup

```bash
# Backup directory binaries
tar -czf backup-$(date +%Y%m%d).tar.gz data/binaries/

# Restore
tar -xzf backup-20260106.tar.gz
```

---

## 🛠 API Reference

### Public Endpoints (Porta 80)

#### GET /api/manifest
Ritorna lista versioni disponibili.

**Response:**
```json
{
  "versions": [
    {
      "version": "1.1.0",
      "buildNumber": 3,
      "fullVersion": "1.1.0+3",
      "url": "/binaries/1.1.0+3/firmware.bin",
      "size": 1234567,
      "md5": "abc123...",
      "releaseDate": "2026-01-06T12:00:00Z"
    }
  ],
  "generatedAt": "2026-01-06T12:00:00Z"
}
```

#### GET /binaries/{version}/firmware.bin
Download diretto firmware.

---

### Admin Endpoints (Porta 81) 🔐

Tutti richiedono Basic Auth: `Authorization: Basic base64(user:pass)`

#### POST /api/upload
Upload nuovo firmware.

**Body (multipart/form-data):**
- `version`: `1.2.0+5`
- `firmware`: File `.bin`

**Response:**
```json
{
  "success": true,
  "version": "1.2.0+5",
  "file": "firmware.bin",
  "size": 1234567,
  "manifest": { ... }
}
```

#### GET /api/versions
Lista tutte le versioni (come manifest).

#### DELETE /api/versions/{version}
Elimina una versione.

**Response:**
```json
{
  "success": true,
  "version": "1.0.0+1",
  "manifest": { ... }
}
```

#### POST /api/regenerate-manifest
Rigenera manualmente il manifest.

---

## 💡 Integrazione Flutter

L'app Flutter usa `FirmwareRepository` service:

```dart
final repo = FirmwareRepository(
  baseUrl: 'http://binaries.server21.it'
);

// Scarica lista versioni
final versions = await repo.fetchAvailableVersions();

// Download firmware
final filePath = await repo.downloadFirmware(
  versions.first,
  onProgress: (received, total) {
    print('Progress: ${(received / total * 100).toInt()}%');
  }
);

// Verifica aggiornamenti
final update = await repo.checkForUpdate('1.0.0+1');
if (update != null) {
  print('Nuovo aggiornamento: ${update.fullVersion}');
}
```

---

## 🐛 Troubleshooting

### Container non si avvia

```bash
docker-compose logs public
docker-compose logs admin
```

### Porta già in uso

Modifica `docker-compose.yml`:
```yaml
ports:
  - "8080:80"   # Usa porta diversa
  - "8081:3000"
```

### Permessi directory

```bash
chmod -R 755 data/binaries
```

### Reset completo

```bash
docker-compose down -v
rm -rf data/binaries/*
docker-compose up -d
```

---

## 📊 Monitoring

### Logs in tempo reale

```bash
docker-compose logs -f
```

### Health Check

```bash
# Public
curl http://localhost/health

# Admin
curl http://localhost:81/health
```

### Spazio disco

```bash
du -sh data/binaries
```

---

## 🚢 Deploy su Server Remoto

### Via Git

```bash
# Sul server
git clone https://github.com/tuousername/firmware-server.git
cd firmware-server
cp .env.example .env
nano .env  # Configura credenziali
docker-compose up -d
```

### Via SCP

```bash
# Dal locale
scp -r firmware-server/ user@server:/opt/firmware-server
ssh user@server
cd /opt/firmware-server
docker-compose up -d
```

---

## 📝 Formato Versioni

**Formato richiesto:** `MAJOR.MINOR.PATCH+BUILD`

**Esempi validi:**
- `1.0.0+1`
- `1.2.3+10`
- `2.0.0+1`

**Esempi non validi:**
- `1.0.0` (manca build)
- `v1.0.0+1` (no prefisso)
- `1.0+1` (manca patch)

---

## 📜 Licenza

MIT

---

## 🤝 Contribuire

Pull requests benvenuti!

1. Fork il progetto
2. Crea feature branch (`git checkout -b feature/amazing`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing`)
5. Open Pull Request

---

## 📧 Supporto

Per bug o domande, apri una issue su GitHub.

---

**Made with ❤️ for ESP32 LED Matrix**
