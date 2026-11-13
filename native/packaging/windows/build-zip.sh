#!/bin/sh
set -eu
ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
DIST_DIR="$ROOT_DIR/dist/windows-x86_64"
BUILD_DIR="$ROOT_DIR/build/windows-x86_64"
CC=${CC:-x86_64-w64-mingw32-gcc}
CXX=${CXX:-x86_64-w64-mingw32-g++}
CMAKE=${CMAKE:-cmake}
rm -rf "$DIST_DIR" "$ROOT_DIR/dist/Eldoria-Windows-x86_64.zip"
"$CMAKE" -S "$ROOT_DIR" -B "$BUILD_DIR" -G "Unix Makefiles" -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX" -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres -DCMAKE_BUILD_TYPE=Release
"$CMAKE" --build "$BUILD_DIR" --config Release
mkdir -p "$DIST_DIR/assets"
cp "$BUILD_DIR/bin/eldoria.exe" "$DIST_DIR/Eldoria.exe"
cp -R "$ROOT_DIR/assets/." "$DIST_DIR/assets/"
printf 'Eldoria para Windows 10+\n\nExecute Eldoria.exe.\n' > "$DIST_DIR/LEIA-ME.txt"
(cd "$ROOT_DIR/dist" && zip -qr Eldoria-Windows-x86_64.zip windows-x86_64)
echo "Criado: $ROOT_DIR/dist/Eldoria-Windows-x86_64.zip"
