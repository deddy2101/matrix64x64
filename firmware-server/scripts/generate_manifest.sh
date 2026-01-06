#!/bin/bash
# Script per generare manifest.json con lista versioni firmware

BINARIES_DIR="${1:-/usr/share/nginx/html/binaries}"
MANIFEST_FILE="$BINARIES_DIR/manifest.json"

echo "Generazione manifest.json..."

# Inizia array JSON
echo "{" > "$MANIFEST_FILE"
echo '  "versions": [' >> "$MANIFEST_FILE"

first=true

# Itera su tutte le directory versione (formato: MAJOR.MINOR.PATCH+BUILD)
for version_dir in "$BINARIES_DIR"/*/ ; do
    [ -d "$version_dir" ] || continue

    version=$(basename "$version_dir")

    # Salta se è una directory speciale
    [[ "$version" == "." || "$version" == ".." ]] && continue

    # Cerca file .bin nella directory
    firmware_file=$(find "$version_dir" -name "*.bin" -type f | head -n 1)

    if [ -z "$firmware_file" ]; then
        echo "⚠ Nessun file .bin trovato in $version_dir"
        continue
    fi

    firmware_name=$(basename "$firmware_file")
    firmware_size=$(stat -f%z "$firmware_file" 2>/dev/null || stat -c%s "$firmware_file" 2>/dev/null)
    firmware_md5=$(md5sum "$firmware_file" 2>/dev/null | awk '{print $1}' || md5 -q "$firmware_file" 2>/dev/null)
    firmware_date=$(date -r "$firmware_file" -Iseconds 2>/dev/null || date -d "@$(stat -c%Y "$firmware_file")" -Iseconds 2>/dev/null)

    # Estrai major.minor.patch e build number
    if [[ $version =~ ^([0-9]+\.[0-9]+\.[0-9]+)\+([0-9]+)$ ]]; then
        version_string="${BASH_REMATCH[1]}"
        build_number="${BASH_REMATCH[2]}"
    else
        echo "⚠ Formato versione non valido: $version (atteso: X.Y.Z+N)"
        continue
    fi

    # Aggiungi virgola se non è il primo elemento
    if [ "$first" = false ]; then
        echo "," >> "$MANIFEST_FILE"
    fi
    first=false

    # Scrivi entry JSON
    cat >> "$MANIFEST_FILE" <<EOF
    {
      "version": "$version_string",
      "buildNumber": $build_number,
      "fullVersion": "$version",
      "url": "/binaries/$version/$firmware_name",
      "size": $firmware_size,
      "md5": "$firmware_md5",
      "releaseDate": "$firmware_date"
    }
EOF

    echo "✓ Aggiunta versione: $version ($firmware_size bytes)"
done

# Chiudi array JSON
echo "" >> "$MANIFEST_FILE"
echo "  ]," >> "$MANIFEST_FILE"
echo "  \"generatedAt\": \"$(date -Iseconds)\"" >> "$MANIFEST_FILE"
echo "}" >> "$MANIFEST_FILE"

echo "✓ Manifest generato: $MANIFEST_FILE"
echo ""
echo "Contenuto:"
cat "$MANIFEST_FILE"
