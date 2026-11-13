#!/bin/sh
set -eu

# Eldoria design reminder: the installer should feel calm, clear, and native to macOS.

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
DIST_DIR="$ROOT_DIR/dist/macos"
APP_DIR="$DIST_DIR/Eldoria.app"
ICON_FILE="$ROOT_DIR/assets/icons/Eldoria.icns"
PAYLOAD_DIR="$DIST_DIR/pkgroot"
PKG_DIR="$DIST_DIR/pkg"

: "${ELDORIA_MACOS_BIN:?Defina ELDORIA_MACOS_BIN apontando para o executável macOS compilado nativamente.}"

rm -rf "$DIST_DIR"
mkdir -p "$APP_DIR/Contents/MacOS" "$APP_DIR/Contents/Resources/assets" "$PAYLOAD_DIR/Applications" "$PKG_DIR"

cp "$ELDORIA_MACOS_BIN" "$APP_DIR/Contents/MacOS/Eldoria"
cp -R "$ROOT_DIR/assets/." "$APP_DIR/Contents/Resources/assets/"
if [ -f "$ICON_FILE" ]; then
    cp "$ICON_FILE" "$APP_DIR/Contents/Resources/Eldoria.icns"
else
    echo "Aviso: ícone macOS não encontrado em $ICON_FILE" >&2
fi
cp -R "$APP_DIR" "$PAYLOAD_DIR/Applications/"

cat > "$APP_DIR/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDisplayName</key><string>Eldoria</string>
    <key>CFBundleExecutable</key><string>Eldoria</string>
    <key>CFBundleIdentifier</key><string>com.eldoria.game</string>
    <key>CFBundleIconFile</key><string>Eldoria</string>
    <key>CFBundleName</key><string>Eldoria</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleShortVersionString</key><string>0.1.0</string>
    <key>CFBundleVersion</key><string>0.1.0</string>
    <key>LSMinimumSystemVersion</key><string>10.13</string>
</dict>
</plist>
PLIST

chmod 755 "$APP_DIR/Contents/MacOS/Eldoria"

pkgbuild \
    --root "$PAYLOAD_DIR" \
    --identifier com.eldoria.game \
    --version 0.1.0 \
    --install-location / \
    "$PKG_DIR/Eldoria-component.pkg"

productbuild \
    --package "$PKG_DIR/Eldoria-component.pkg" \
    "$DIST_DIR/Eldoria-macOS-10.13.pkg"

echo "Criado: $DIST_DIR/Eldoria-macOS-10.13.pkg"
