#ifndef ELDORIA_CONFIG_H
#define ELDORIA_CONFIG_H

/* Eldoria design reminder: keep the same game feel across low and high visual profiles. */

#define ELDORIA_TITLE "Eldoria — Veios de Aurora"
#define ELDORIA_DEFAULT_WIDTH 1280
#define ELDORIA_DEFAULT_HEIGHT 720
#define ELDORIA_TARGET_FPS 60
#define ELDORIA_FIXED_STEP (1.0f / 60.0f)
#define ELDORIA_WORLD_SEED 0xA17E0F3Du

#ifndef ELDORIA_PROFILE
#define ELDORIA_PROFILE 0
#endif

#define ELDORIA_PROFILE_COMPATIBLE 0
#define ELDORIA_PROFILE_STANDARD 1
#define ELDORIA_PROFILE_QUALITY 2

#if defined(_WIN32)
#define ELDORIA_PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
#define ELDORIA_PLATFORM_NAME "macOS"
#elif defined(__linux__)
#define ELDORIA_PLATFORM_NAME "Linux"
#else
#define ELDORIA_PLATFORM_NAME "Desktop"
#endif

#endif
