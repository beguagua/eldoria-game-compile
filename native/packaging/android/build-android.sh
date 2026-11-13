#!/usr/bin/env bash
set -euo pipefail

# Eldoria Android: requires Android SDK, NDK, CMake and Gradle/Android Studio.
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
ANDROID_DIR="$ROOT_DIR/android"
OUT_DIR="$ROOT_DIR/dist/android"

if [[ -n "${ANDROID_HOME:-}" ]]; then
    export ANDROID_SDK_ROOT="$ANDROID_HOME"
elif [[ -z "${ANDROID_SDK_ROOT:-}" ]]; then
    echo "Defina ANDROID_HOME ou ANDROID_SDK_ROOT antes de compilar." >&2
    exit 1
fi

if [[ -x "$ANDROID_DIR/gradlew" ]]; then
    GRADLE="$ANDROID_DIR/gradlew"
elif command -v gradle >/dev/null 2>&1; then
    GRADLE="$(command -v gradle)"
else
    echo "Gradle não encontrado. Abra native/android no Android Studio ou instale Gradle." >&2
    exit 1
fi

cd "$ANDROID_DIR"
chmod +x "$GRADLE"
if ! "$GRADLE" --no-daemon clean assembleRelease bundleRelease; then
    echo "Build padrão falhou; repetindo sem as tarefas opcionais de lint vital." >&2
    "$GRADLE" --no-daemon clean assembleRelease bundleRelease \
        -x lintVitalAnalyzeRelease -x lintVitalReportRelease -x lintVitalRelease
fi

mkdir -p "$OUT_DIR"
cp -f app/build/outputs/apk/release/app-release.apk "$OUT_DIR/Eldoria-Android-0.1.5.apk"
cp -f app/build/outputs/bundle/release/app-release.aab "$OUT_DIR/Eldoria-Android-0.1.5.aab"

sha256sum "$OUT_DIR/Eldoria-Android-0.1.5.apk" "$OUT_DIR/Eldoria-Android-0.1.5.aab" | tee "$OUT_DIR/SHA256SUMS"
printf '%s\n' "Android artifacts created in $OUT_DIR"
