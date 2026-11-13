#!/bin/sh
set -eu

# Eldoria: native Intel macOS build, minimum target 10.13, compatible with newer Intel macOS releases.

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
RAYLIB_SRC="$ROOT_DIR/external/raylib-5.5/src"
BUILD_DIR="$ROOT_DIR/build/macos-x86_64"
MINIMUM_MACOS="${MACOSX_DEPLOYMENT_TARGET:-10.13}"
ARCH="x86_64"

if ! command -v clang >/dev/null 2>&1 || ! command -v make >/dev/null 2>&1; then
    echo "Erro: instale as Command Line Tools da Apple (xcode-select --install)." >&2
    exit 1
fi

if ! command -v sw_vers >/dev/null 2>&1; then
    echo "Erro: este script deve ser executado no macOS." >&2
    exit 1
fi

export MACOSX_DEPLOYMENT_TARGET="$MINIMUM_MACOS"
mkdir -p "$BUILD_DIR"

echo "Compilando raylib para macOS $MINIMUM_MACOS+ ($ARCH)..."
make -C "$RAYLIB_SRC" clean >/dev/null 2>&1 || true
make -C "$RAYLIB_SRC" \
    TARGET_PLATFORM=PLATFORM_DESKTOP_GLFW \
    PLATFORM_OS=OSX \
    PLATFORM=PLATFORM_DESKTOP \
    CC=clang \
    CUSTOM_CFLAGS="-O2 -arch $ARCH -mmacosx-version-min=$MINIMUM_MACOS" \
    LDFLAGS="-arch $ARCH -mmacosx-version-min=$MINIMUM_MACOS"

echo "Compilando Eldoria..."
clang -std=c99 -O2 -Wall -Wextra -Wpedantic \
    -arch "$ARCH" -mmacosx-version-min="$MINIMUM_MACOS" \
    -I"$ROOT_DIR/include" -I"$RAYLIB_SRC" \
    "$ROOT_DIR/src/main.c" \
    "$ROOT_DIR/src/net.c" \
    -o "$BUILD_DIR/Eldoria" \
    -L"$RAYLIB_SRC" -lraylib \
    -framework Cocoa -framework IOKit -framework OpenGL -framework CoreVideo -framework AudioToolbox

echo "Verificando o binário..."
file "$BUILD_DIR/Eldoria"
if ! file "$BUILD_DIR/Eldoria" | grep -q "Mach-O.*x86_64"; then
    echo "Erro: a compilação não produziu um binário Mach-O x86_64." >&2
    exit 1
fi

echo "Build pronta: $BUILD_DIR/Eldoria"

if [ "${1:-}" = "--pkg" ]; then
    export ELDORIA_MACOS_BIN="$BUILD_DIR/Eldoria"
    "$ROOT_DIR/packaging/macos/build-pkg.sh"
fi
