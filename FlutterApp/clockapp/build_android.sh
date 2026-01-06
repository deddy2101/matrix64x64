#!/bin/bash

echo "🔢 Incrementing build number..."
cider bump build

echo "📦 Building Android app bundle..."
flutter build appbundle

echo "✅ Done! APK/Bundle in: build/app/outputs/bundle/release/"
