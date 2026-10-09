#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PUBSPEC="$APP_DIR/pubspec.yaml"

# ── Piattaforma: come parametro (android/ios/1/2) o menu interattivo ─────────
PLATFORM="${1:-}"
if [ -z "$PLATFORM" ]; then
  echo "Quale piattaforma vuoi buildare?"
  echo "  1) Android"
  echo "  2) iOS"
  read -rp "Scelta: " PLATFORM
fi

case "$PLATFORM" in
  1|android) PLATFORM="android" ;;
  2|ios) PLATFORM="ios" ;;
  *)
    echo "Errore: piattaforma non valida ('$PLATFORM'). Usa: android, ios, 1 oppure 2." >&2
    exit 1
    ;;
esac

if [ "$PLATFORM" = "android" ] && [ ! -f "$APP_DIR/android/key.properties" ]; then
  echo "Errore: $APP_DIR/android/key.properties non trovato." >&2
  echo "Serve il keystore di release per firmare il bundle." >&2
  exit 1
fi

# ── Versione: incrementa la patch (X.Y.Z) o lascia com'è ─────────────────────
# Il build number (+N) va SEMPRE incrementato, anche a versione invariata:
# gli store rifiutano un binario con lo stesso build number del precedente.
CURRENT_VERSION="$(grep -m1 '^version:' "$PUBSPEC" | sed 's/^version: *//')"
CURRENT_SEMVER="${CURRENT_VERSION%+*}"
CURRENT_BUILD="${CURRENT_VERSION#*+}"
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT_SEMVER"

echo ""
echo "Versione attuale: $CURRENT_VERSION"
echo "Vuoi incrementare la versione?"
echo "  1) Sì — incrementa patch ($MAJOR.$MINOR.$((PATCH + 1))) e build number"
echo "  2) No — lascia $CURRENT_SEMVER, incrementa solo il build number (richiesto dagli store)"
read -rp "Scelta: " BUMP_CHOICE

NEW_BUILD=$((CURRENT_BUILD + 1))
case "$BUMP_CHOICE" in
  1)
    NEW_SEMVER="$MAJOR.$MINOR.$((PATCH + 1))"
    ;;
  2)
    NEW_SEMVER="$CURRENT_SEMVER"
    ;;
  *)
    echo "Errore: scelta non valida ('$BUMP_CHOICE'). Usa: 1 oppure 2." >&2
    exit 1
    ;;
esac
NEW_VERSION="$NEW_SEMVER+$NEW_BUILD"

sed "s/^version: .*/version: $NEW_VERSION/" "$PUBSPEC" > "$PUBSPEC.tmp" && mv "$PUBSPEC.tmp" "$PUBSPEC"
echo "Versione: $CURRENT_VERSION → $NEW_VERSION"
echo ""

cd "$APP_DIR"

if [ "$PLATFORM" = "android" ]; then
  flutter build appbundle --release

  AAB_PATH="$APP_DIR/build/app/outputs/bundle/release/app-release.aab"
  echo ""
  echo "Bundle generato: $AAB_PATH"
else
  EXPORT_OPTIONS="$APP_DIR/ios/ExportOptions.plist"
  # ExportOptions.plist è opzionale: se manca, flutter/xcodebuild rilevano da
  # soli le impostazioni di firma già configurate nel progetto Xcode (firma
  # automatica, team già impostato) e generano un plist equivalente.
  EXPORT_OPTIONS_ARG=()
  if [ -f "$EXPORT_OPTIONS" ]; then
    EXPORT_OPTIONS_ARG=(--export-options-plist="$EXPORT_OPTIONS")
  fi

  # Sintassi ${arr[@]+...}: con set -u, bash 3.2 (quello di macOS) va in errore
  # espandendo un array vuoto.
  flutter build ipa --release ${EXPORT_OPTIONS_ARG[@]+"${EXPORT_OPTIONS_ARG[@]}"}

  IPA_PATH="$(find "$APP_DIR/build/ios/ipa" -maxdepth 1 -name '*.ipa' | head -n1)"
  echo ""
  echo "Ipa generato: $IPA_PATH"
  echo "Caricalo su App Store Connect con Transporter."
fi
