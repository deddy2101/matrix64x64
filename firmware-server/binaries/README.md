# Firmware Binaries

Questa directory contiene i firmware disponibili per il dispositivo LED Matrix.

## Struttura

Ogni versione ha una directory dedicata con formato: `MAJOR.MINOR.PATCH+BUILD`

Esempio:
```
binaries/
├── 1.0.0+1/
│   └── firmware.bin
├── 1.0.0+2/
│   └── firmware.bin
├── 1.1.0+3/
│   └── firmware.bin
└── manifest.json  (generato automaticamente)
```

## Come aggiungere una nuova versione

1. Crea directory con formato versione: `mkdir 1.2.0+4`
2. Copia il file .bin: `cp firmware.bin 1.2.0+4/`
3. Rigenera manifest: `./scripts/generate_manifest.sh ./binaries`
4. (Opzionale) Riavvia container: `docker-compose restart`

## Manifest.json

Il file `manifest.json` viene generato automaticamente e contiene:
- Lista tutte le versioni disponibili
- URL download per ogni firmware
- Dimensione file
- MD5 checksum
- Data di rilascio

L'app Flutter scarica questo file per mostrare le versioni disponibili.
