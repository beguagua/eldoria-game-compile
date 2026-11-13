# Eldoria Makefile
# Multi-platform build system for Linux, macOS, and Windows (MinGW)

# Project Settings
PROJECT_NAME = eldoria
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# Source Files
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/net.c
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Compiler and Flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Wshadow -O2
INCLUDES = -I$(INC_DIR) -Iexternal/raylib-5.5/src

# Raylib Settings
# By default, we look for raylib in the external directory
RAYLIB_SRC = external/raylib-5.5/src
RAYLIB_LIB = $(RAYLIB_SRC)/libraylib.a

# OS Detection
ifeq ($(OS),Windows_NT)
    PLATFORM = WINDOWS
    EXE_EXT = .exe
    LDFLAGS = -L$(RAYLIB_SRC) -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM = LINUX
        EXE_EXT =
        LDFLAGS = -L$(RAYLIB_SRC) -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
    ifeq ($(UNAME_S),Darwin)
        PLATFORM = MACOS
        EXE_EXT =
        LDFLAGS = -L$(RAYLIB_SRC) -lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
    endif
endif

TARGET = $(BIN_DIR)/$(PROJECT_NAME)$(EXE_EXT)

.PHONY: all clean run setup-raylib

all: setup-raylib $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@cp -r assets $(BIN_DIR)/ 2>/dev/null || :

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build raylib if the static library doesn't exist
setup-raylib:
	@if [ ! -f $(RAYLIB_LIB) ]; then \
		echo "Building raylib..."; \
		$(MAKE) -C $(RAYLIB_SRC) PLATFORM=PLATFORM_DESKTOP; \
	fi

run: all
	cd $(BIN_DIR) && ./$(PROJECT_NAME)$(EXE_EXT)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@if [ -d $(RAYLIB_SRC) ]; then \
		$(MAKE) -C $(RAYLIB_SRC) clean; \
	fi
