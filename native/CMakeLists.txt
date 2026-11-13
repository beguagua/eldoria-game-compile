cmake_minimum_required(VERSION 3.10)

# Eldoria macOS policy: Intel x86_64, deployable on 10.13 and later Intel macOS releases.
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment target")
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Target CPU architecture")
endif()

project(eldoria C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT DEFINED RAYLIB_PATH)
    set(RAYLIB_PATH "" CACHE PATH "Diretório contendo raylib.h e libraylib")
endif()

find_package(raylib QUIET)
if(NOT raylib_FOUND AND RAYLIB_PATH)
    add_library(raylib STATIC IMPORTED)
    if(WIN32)
        set_target_properties(raylib PROPERTIES
            IMPORTED_LOCATION "${RAYLIB_PATH}/lib/raylib.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_PATH}/include")
    else()
        set_target_properties(raylib PROPERTIES
            IMPORTED_LOCATION "${RAYLIB_PATH}/libraylib.a"
            INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_PATH}")
    endif()
endif()

if(NOT raylib_FOUND AND NOT TARGET raylib)
    if(EXISTS "${CMAKE_SOURCE_DIR}/external/raylib-5.5/CMakeLists.txt")
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        add_subdirectory(external/raylib-5.5)
    else()
        message(FATAL_ERROR "raylib não encontrada. Instale-a, configure -DRAYLIB_PATH ou mantenha external/raylib-5.5 no source.")
    endif()
endif()

add_executable(eldoria
    src/main.c
    src/net.c
)
target_include_directories(eldoria PRIVATE include)
target_compile_definitions(eldoria PRIVATE ELDORIA_PROFILE=0)
target_compile_options(eldoria PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
target_link_libraries(eldoria PRIVATE raylib)

if(APPLE)
    target_link_libraries(eldoria PRIVATE "-framework Cocoa" "-framework IOKit" "-framework OpenGL" "-framework CoreVideo")
elseif(WIN32)
    enable_language(RC)
    configure_file("${CMAKE_SOURCE_DIR}/assets/icons/Eldoria.ico" "${CMAKE_CURRENT_BINARY_DIR}/Eldoria.ico" COPYONLY)
    configure_file("${CMAKE_SOURCE_DIR}/src/eldoria.rc" "${CMAKE_CURRENT_BINARY_DIR}/eldoria.rc" COPYONLY)
    target_sources(eldoria PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/eldoria.rc")
    target_link_libraries(eldoria PRIVATE opengl32 gdi32 winmm ws2_32)
    set_target_properties(eldoria PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(UNIX)
    target_link_libraries(eldoria PRIVATE GL m pthread dl rt X11)
endif()

set_target_properties(eldoria PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

file(COPY "${CMAKE_SOURCE_DIR}/assets" DESTINATION "${CMAKE_BINARY_DIR}")
