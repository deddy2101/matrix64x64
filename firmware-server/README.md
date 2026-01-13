# LED Matrix Firmware Server

OTA firmware distribution and management system for ESP32 devices.

## Features

- **Public Interface (Port 80)**: Firmware download (read-only)
- **Admin Panel (Port 81)**: Firmware upload and management (password protected)
- **Docker Ready**: Single-command deployment
- **Persistent Volume**: Data stored in `./data/binaries`
- **Auto Manifest**: Automatically generates `manifest.json`
- **REST API**: Endpoints for Flutter app integration
- **Versioning**: SemVer + Build format support (X.Y.Z+N)
- **MD5 Checksum**: Firmware integrity verification

## Quick Start

```bash
# 1. Clone repository
git clone https://github.com/yourusername/firmware-server.git
cd firmware-server

# 2. Configure credentials (optional)
cp .env.example .env
# Edit .env with your credentials

# 3. Start server
docker-compose up -d

# 4. Access interfaces
# Public: http://localhost (port 80)
# Admin:  http://localhost:81 (port 81)
```

## Project Structure

```
firmware-server/
├── docker-compose.yml       # 2-service orchestration
├── .env.example             # Credential template
├── .gitignore
├── README.md
│
├── public/                  # Nginx (port 80)
│   ├── nginx.conf
│   └── index.html           # Public UI
│
├── admin/                   # Node.js (port 81)
│   ├── Dockerfile
│   ├── package.json
│   ├── server.js            # API server
│   └── public/
│       └── index.html       # Admin UI
│
└── data/                    # Persistent volume
    └── binaries/            # Firmware storage
        ├── 1.0.0+1/
        │   └── firmware.bin
        ├── 1.1.0+3/
        │   └── firmware.bin
        └── manifest.json    # Auto-generated
```

## Interfaces

### Port 80 - Public Interface

**URL**: `http://binaries.server21.it`

Public interface for viewing and downloading firmware.

**Features:**
- View available versions
- Direct firmware download
- Information on size, MD5, release date
- "LATEST" badge for newest version
- No authentication required

**API Endpoints:**
```
GET /api/manifest                    # List versions JSON
GET /binaries/{version}/firmware.bin # Download firmware
```

### Port 81 - Admin Panel

**URL**: `http://binaries.server21.it:81`

Administrator panel for firmware management.

**Default Credentials:**
- Username: `admin`
- Password: `changeme`

**Features:**
- Upload new firmware (.bin)
- Delete versions
- Real-time statistics
- Auto-generate manifest
- Basic Auth protection

## Firmware Upload

### Via Admin Panel

1. Open `http://server:81`
2. Login with admin credentials
3. Fill form:
   - Version: `1.2.0+5`
   - File: Select `.bin`
4. Click "Upload Firmware"
5. Manifest auto-generated

### Via API

```bash
curl -X POST http://server:81/api/upload \
  -u admin:changeme \
  -F "version=1.2.0+5" \
  -F "firmware=@firmware.bin"
```

## Production Deployment

### 1. DNS Setup

Point domain to server:
```
binaries.server21.it → SERVER_IP
```

### 2. Configure Credentials

```bash
# Create .env file
cp .env.example .env

# Edit credentials
nano .env
```

```env
ADMIN_USERNAME=your_username
ADMIN_PASSWORD=secure_password
```

### 3. Start Containers

```bash
docker-compose up -d
```

### 4. Verify

```bash
# Public interface
curl http://binaries.server21.it/health

# Admin interface
curl http://binaries.server21.it:81/health

# Manifest
curl http://binaries.server21.it/api/manifest
```

## Security

### HTTPS (Optional)

Add reverse proxy with Certbot:

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

Or use Cloudflare for automatic SSL.

### Firewall

```bash
# Open required ports only
ufw allow 80/tcp
ufw allow 81/tcp
ufw enable
```

### Backup

```bash
# Backup binaries directory
tar -czf backup-$(date +%Y%m%d).tar.gz data/binaries/

# Restore
tar -xzf backup-20260106.tar.gz
```

## API Reference

### Public Endpoints (Port 80)

#### GET /api/manifest
Returns list of available versions.

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
Direct firmware download.

### Admin Endpoints (Port 81)

All require Basic Auth: `Authorization: Basic base64(user:pass)`

#### POST /api/upload
Upload new firmware.

**Body (multipart/form-data):**
- `version`: `1.2.0+5`
- `firmware`: `.bin` file

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
List all versions (same as manifest).

#### DELETE /api/versions/{version}
Delete a version.

**Response:**
```json
{
  "success": true,
  "version": "1.0.0+1",
  "manifest": { ... }
}
```

#### POST /api/regenerate-manifest
Manually regenerate manifest.

## Flutter Integration

Flutter app uses `FirmwareRepository` service:

```dart
final repo = FirmwareRepository(
  baseUrl: 'http://binaries.server21.it'
);

// Download version list
final versions = await repo.fetchAvailableVersions();

// Download firmware
final filePath = await repo.downloadFirmware(
  versions.first,
  onProgress: (received, total) {
    print('Progress: ${(received / total * 100).toInt()}%');
  }
);

// Check for updates
final update = await repo.checkForUpdate('1.0.0+1');
if (update != null) {
  print('New update: ${update.fullVersion}');
}
```

## Troubleshooting

### Container not starting

```bash
docker-compose logs public
docker-compose logs admin
```

### Port already in use

Modify `docker-compose.yml`:
```yaml
ports:
  - "8080:80"   # Use different port
  - "8081:3000"
```

### Directory permissions

```bash
chmod -R 755 data/binaries
```

### Complete reset

```bash
docker-compose down -v
rm -rf data/binaries/*
docker-compose up -d
```

## Monitoring

### Real-time logs

```bash
docker-compose logs -f
```

### Health check

```bash
# Public
curl http://localhost/health

# Admin
curl http://localhost:81/health
```

### Disk space

```bash
du -sh data/binaries
```

## Remote Deployment

### Via Git

```bash
# On server
git clone https://github.com/yourusername/firmware-server.git
cd firmware-server
cp .env.example .env
nano .env  # Configure credentials
docker-compose up -d
```

### Via SCP

```bash
# From local
scp -r firmware-server/ user@server:/opt/firmware-server
ssh user@server
cd /opt/firmware-server
docker-compose up -d
```

## Version Format

**Required format:** `MAJOR.MINOR.PATCH+BUILD`

**Valid examples:**
- `1.0.0+1`
- `1.2.3+10`
- `2.0.0+1`

**Invalid examples:**
- `1.0.0` (missing build)
- `v1.0.0+1` (no prefix)
- `1.0+1` (missing patch)

## License

MIT

## Support

For bugs or questions, open an issue on GitHub.
