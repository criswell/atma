# atma — cross-platform retro engine.
#
# Pick a backend with PLATFORM=<name>. Core (src/core) and game (game) are
# shared; each backend contributes whatever .c files sit in
# src/platform/<name>/, so dropping a new .c file there is enough to add it
# to the build. host builds natively with gcc; zxn needs the z88dk toolchain.

PLATFORM ?= host
BUILD    := build/$(PLATFORM)
CORE     := $(wildcard src/core/*.c)
PLAT     := $(wildcard src/platform/$(PLATFORM)/*.c)
GAME     := $(wildcard game/*.c)
SRCS     := $(CORE) $(PLAT) $(GAME)

ifeq ($(PLATFORM),host)
# Native build: run and debug the same engine the retro targets will use.
CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Iinclude -D_POSIX_C_SOURCE=200809L
TARGET  := $(BUILD)/atma
$(TARGET): $(SRCS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@
else ifeq ($(PLATFORM),zxn)
# ZX Spectrum Next build via zcc (z88dk): SDCC backend, size-optimized,
# emitting a .nex file. The sprite/Layer 2/vt_sound libs join when the gfx
# and sound backends stop being stubs; nothing references them yet.
CC      := zcc
CFLAGS  := +zxn -vn -SO3 --opt-code-size -clib=sdcc_iy -Iinclude
TARGET  := $(BUILD)/atma.nex
$(TARGET): $(SRCS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -startup=31 -create-app -subtype=nex -o $(BUILD)/atma
endif

.PHONY: run clean
run: $(TARGET)
	./$(TARGET)
clean:
	rm -rf build
