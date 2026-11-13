cmake_minimum_required(VERSION 3.22.1)
project(eldoria_android C)

# Eldoria Android: C99 + raylib 5.5 + OpenGL ES; touch input is implemented in src/main.c.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(PLATFORM Android CACHE STRING "raylib platform" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

set(ELDORIA_NATIVE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
set(RAYLIB_SOURCE "${ELDORIA_NATIVE_ROOT}/external/raylib-5.5")

if(NOT EXISTS "${RAYLIB_SOURCE}/CMakeLists.txt")
    message(FATAL_ERROR "raylib 5.5 não encontrado em ${RAYLIB_SOURCE}")
endif()

add_subdirectory("${RAYLIB_SOURCE}" raylib-build)

add_library(main SHARED 
    "${ELDORIA_NATIVE_ROOT}/src/main.c"
    "${ELDORIA_NATIVE_ROOT}/src/net.c"
    "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c"
)
target_include_directories(main PRIVATE 
    "${ELDORIA_NATIVE_ROOT}/include"
    "${ANDROID_NDK}/sources/android/native_app_glue"
)
target_compile_definitions(main PRIVATE ELDORIA_PROFILE=1 PLATFORM_ANDROID ELDORIA_ENABLE_ANDROID_AUDIO)
target_compile_options(main PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
target_link_libraries(main PRIVATE raylib log android EGL GLESv2 OpenSLES m dl)

set_target_properties(main PROPERTIES OUTPUT_NAME "main")
