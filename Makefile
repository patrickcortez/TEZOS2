# 2D Engine Makefile
# Cross-platform build system with automatic platform detection

# Compiler
CC := gcc

# Detect platform
UNAME_S := $(shell uname -s)

# Project directories
SRC_DIR := src
PLATFORM_DIR := $(SRC_DIR)/platform
INCLUDE_DIR := include
BUILD_DIR := build
EXAMPLE_DIR := examples

# Output
LIB_NAME := libengine.a
EXAMPLE_BIN := basic_window

# Common flags
CFLAGS := -I$(INCLUDE_DIR) -Wall -Wextra -std=c99
LDFLAGS :=

# Debug/Release flags
DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS += -g -O0 -DENGINE_DEBUG -DENGINE_ENABLE_LOGGING
else
    CFLAGS += -O2 -DNDEBUG
endif

# Platform-specific settings
ifeq ($(UNAME_S), Linux)
    PLATFORM := linux
    PLATFORM_SRC := $(PLATFORM_DIR)/platform_x11.c
    LDFLAGS += -lX11 -lpthread -ldl -lm
    CFLAGS += -DENGINE_PLATFORM_LINUX
endif

ifeq ($(UNAME_S), Darwin)
    PLATFORM := macos
    PLATFORM_SRC := $(PLATFORM_DIR)/platform_macos.c
    LDFLAGS += -framework Cocoa -framework CoreGraphics
    CFLAGS += -DENGINE_PLATFORM_MACOS
endif

# Windows (MinGW/MSYS)
ifneq (,$(findstring MINGW,$(UNAME_S)))
    PLATFORM := windows
    PLATFORM_SRC := $(PLATFORM_DIR)/platform_win32.c
    LDFLAGS += -lgdi32 -luser32
    CFLAGS += -DENGINE_PLATFORM_WIN32
    EXAMPLE_BIN := basic_window.exe
endif

ifneq (,$(findstring MSYS,$(UNAME_S)))
    PLATFORM := windows
    PLATFORM_SRC := $(PLATFORM_DIR)/platform_win32.c
    LDFLAGS += -lgdi32 -luser32
    CFLAGS += -DENGINE_PLATFORM_WIN32
    EXAMPLE_BIN := basic_window.exe
endif

# Source files
ENGINE_SRCS := $(SRC_DIR)/engine.c $(SRC_DIR)/graphics.c $(SRC_DIR)/ui.c $(SRC_DIR)/input.c $(SRC_DIR)/audio.c $(SRC_DIR)/dialogs.c $(PLATFORM_SRC)
ENGINE_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(ENGINE_SRCS)))

# Examples
LIB_TARGET = $(BUILD_DIR)/$(LIB_NAME)
BASIC_WINDOW_TARGET = $(BUILD_DIR)/basic_window$(if $(findstring windows,$(PLATFORM)),.exe,)
GRAPHICS_TEST_TARGET = $(BUILD_DIR)/graphics_test$(if $(findstring windows,$(PLATFORM)),.exe,)
UI_DEMO_TARGET = $(BUILD_DIR)/ui_demo$(if $(findstring windows,$(PLATFORM)),.exe,)
INPUT_DEMO_TARGET = $(BUILD_DIR)/input_demo$(if $(findstring windows,$(PLATFORM)),.exe,)
FORMS_DEMO_TARGET = $(BUILD_DIR)/forms_demo$(if $(findstring windows,$(PLATFORM)),.exe,)
AUDIO_DEMO_TARGET = $(BUILD_DIR)/audio_demo$(if $(findstring windows,$(PLATFORM)),.exe,)
FILE_LIST_DEMO_TARGET = $(BUILD_DIR)/file_list_demo$(if $(findstring windows,$(PLATFORM)),.exe,)

# Example sources
EXAMPLE_SRC := $(EXAMPLE_DIR)/basic_window.c
EXAMPLE_OBJ := $(BUILD_DIR)/basic_window.o

GRAPHICS_TEST_SRC := $(EXAMPLE_DIR)/graphics_test.c
GRAPHICS_TEST_OBJ := $(BUILD_DIR)/graphics_test.o

UI_DEMO_SRC := $(EXAMPLE_DIR)/ui_demo.c
UI_DEMO_OBJ := $(BUILD_DIR)/ui_demo.o

INPUT_DEMO_SRC := $(EXAMPLE_DIR)/input_demo.c
INPUT_DEMO_OBJ := $(BUILD_DIR)/input_demo.o

FORMS_DEMO_SRC := $(EXAMPLE_DIR)/forms_demo.c
FORMS_DEMO_OBJ := $(BUILD_DIR)/forms_demo.o

AUDIO_DEMO_SRC := $(EXAMPLE_DIR)/audio_demo.c
AUDIO_DEMO_OBJ := $(BUILD_DIR)/audio_demo.o

FILE_LIST_DEMO_SRC := $(EXAMPLE_DIR)/file_list_demo.c
FILE_LIST_DEMO_OBJ := $(BUILD_DIR)/file_list_demo.o

# Default target
.PHONY: all
all: $(BUILD_DIR)/$(LIB_NAME) $(BUILD_DIR)/$(EXAMPLE_BIN) \
     $(BUILD_DIR)/graphics_test$(if $(findstring windows,$(PLATFORM)),.exe,) \
     $(BUILD_DIR)/ui_demo$(if $(findstring windows,$(PLATFORM)),.exe,) \
     $(BUILD_DIR)/input_demo$(if $(findstring windows,$(PLATFORM)),.exe,) \
     $(BUILD_DIR)/forms_demo$(if $(findstring windows,$(PLATFORM)),.exe,) \
     $(BUILD_DIR)/audio_demo$(if $(findstring windows,$(PLATFORM)),.exe,)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile engine sources
$(BUILD_DIR)/engine.o: $(SRC_DIR)/engine.c | $(BUILD_DIR)
	@echo "Compiling engine.c..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/graphics.o: $(SRC_DIR)/graphics.c | $(BUILD_DIR)
	@echo "Compiling graphics.c..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ui.o: $(SRC_DIR)/ui.c | $(BUILD_DIR)
	@echo "Compiling ui.c..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/input.o: $(SRC_DIR)/input.c | $(BUILD_DIR)
	@echo "Compiling input.c..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/audio.o: $(SRC_DIR)/audio.c | $(BUILD_DIR)
	@echo "Compiling audio.c..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dialogs.o: $(SRC_DIR)/dialogs.c | $(BUILD_DIR)
	@echo "Compiling dialogs.c..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/platform_x11.o: $(PLATFORM_DIR)/platform_x11.c | $(BUILD_DIR)
	@echo "Compiling platform_x11.c..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/platform_win32.o: $(PLATFORM_DIR)/platform_win32.c | $(BUILD_DIR)
	@echo "Compiling platform_win32.c..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/platform_macos.o: $(PLATFORM_DIR)/platform_macos.c | $(BUILD_DIR)
	@echo "Compiling platform_macos.c..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Create static library
$(BUILD_DIR)/$(LIB_NAME): $(ENGINE_OBJS)
	@echo "Creating static library..."
	@ar rcs $@ $^
	@echo "Built $(LIB_NAME) for $(PLATFORM)"

# Compile examples
$(BUILD_DIR)/basic_window.o: $(EXAMPLE_SRC) | $(BUILD_DIR)
	@echo "Compiling basic_window example..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/graphics_test.o: $(GRAPHICS_TEST_SRC) | $(BUILD_DIR)
	@echo "Compiling graphics_test example..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ui_demo.o: $(UI_DEMO_SRC) | $(BUILD_DIR)
	@echo "Compiling ui_demo example..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/input_demo.o: $(INPUT_DEMO_SRC) | $(BUILD_DIR)
	@echo "Compiling input_demo example..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/forms_demo.o: $(FORMS_DEMO_SRC) | $(BUILD_DIR)
	@echo "Compiling forms_demo example..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/audio_demo.o: $(AUDIO_DEMO_SRC) | $(BUILD_DIR)
	@echo "Compiling audio_demo example..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Link examples
$(BUILD_DIR)/$(EXAMPLE_BIN): $(EXAMPLE_OBJ) $(BUILD_DIR)/$(LIB_NAME)
	@echo "Linking basic_window..."
	@$(CC) $^ -o $@ $(LDFLAGS)
	@echo "Built $(EXAMPLE_BIN)"

$(BUILD_DIR)/graphics_test$(if $(findstring windows,$(PLATFORM)),.exe,): $(GRAPHICS_TEST_OBJ) $(BUILD_DIR)/$(LIB_NAME)
	@echo "Linking graphics_test..."
	@$(CC) $^ -o $@ $(LDFLAGS) -lm
	@echo "Built graphics_test"

$(BUILD_DIR)/ui_demo$(if $(findstring windows,$(PLATFORM)),.exe,): $(UI_DEMO_OBJ) $(BUILD_DIR)/$(LIB_NAME)
	@echo "Linking ui_demo..."
	@$(CC) $^ -o $@ $(LDFLAGS) -lm
	@echo "Built ui_demo"

$(BUILD_DIR)/input_demo$(if $(findstring windows,$(PLATFORM)),.exe,): $(INPUT_DEMO_OBJ) $(BUILD_DIR)/$(LIB_NAME)
	@echo "Linking input_demo..."
	@$(CC) $^ -o $@ $(LDFLAGS) -lm
	@echo "Built input_demo"

$(BUILD_DIR)/forms_demo$(if $(findstring windows,$(PLATFORM)),.exe,): $(FORMS_DEMO_OBJ) $(BUILD_DIR)/$(LIB_NAME)
	@echo "Linking forms_demo..."
	@$(CC) $^ -o $@ $(LDFLAGS) -lm
	@echo "Built forms_demo"

$(AUDIO_DEMO_TARGET): examples/audio_demo.c $(LIB_TARGET)
	@echo "Compiling audio_demo example..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built audio_demo"

$(FILE_LIST_DEMO_TARGET): examples/file_list_demo.c $(LIB_TARGET)
	@echo "Compiling file_list_demo example..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built file_list_demo"

# Run example
.PHONY: run
run: $(BUILD_DIR)/$(EXAMPLE_BIN)
	@echo "Running example..."
	@$(BUILD_DIR)/$(EXAMPLE_BIN)

# Clean
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)

# Show platform info
.PHONY: info
info:
	@echo "Platform: $(PLATFORM)"
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "Platform source: $(PLATFORM_SRC)"

# Help
.PHONY: help
help:
	@echo "2D Engine Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build library and example (default)"
	@echo "  run      - Build and run the example"
	@echo "  clean    - Remove build artifacts"
	@echo "  info     - Show platform information"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1  - Build with debug symbols (default)"
	@echo "  DEBUG=0  - Build optimized release version"
