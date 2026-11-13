#!/bin/sh
set -eu
ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
DIST_DIR="$ROOT_DIR/dist/linux-x86_64"
rm -rf "$DIST_DIR" "$ROOT_DIR/dist/Eldoria-Linux-x86_64.tar.gz"
mkdir -p "$DIST_DIR/assets"
BINARY="$ROOT_DIR/build/bin/eldoria"
if [ ! -f "$BINARY" ]; then BINARY="$ROOT_DIR/build/eldoria"; fi
if [ ! -f "$BINARY" ]; then echo "Binário Eldoria não encontrado em build/bin/eldoria ou build/eldoria" >&2; exit 1; fi
cp "$BINARY" "$DIST_DIR/Eldoria"
cp -R "$ROOT_DIR/assets/." "$DIST_DIR/assets/"
chmod +x "$DIST_DIR/Eldoria"
printf 'Eldoria para Linux x86_64\n\nExecute ./Eldoria.\n' > "$DIST_DIR/LEIA-ME.txt"
(cd "$ROOT_DIR/dist" && tar -czf Eldoria-Linux-x86_64.tar.gz linux-x86_64)
echo "Criado: $ROOT_DIR/dist/Eldoria-Linux-x86_64.tar.gz"
