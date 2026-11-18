# atma — cross-platform retro engine.
#
# Pick a backend with PLATFORM=<name>. Core (src/core) and game (game) are
# shared; each backend contributes whatever .c files sit in
# src/platform/<name>/, so dropping a new .c file there is enough to add it
# to the build. host builds natively with gcc; zxn needs the z88dk toolchain.

# GAME_DIR picks the program to build (default game; try GAME_DIR=demo).
PLATFORM ?= host
GAME_DIR ?= game
BUILD    := build/$(PLATFORM)/$(notdir $(GAME_DIR))
CORE     := $(wildcard src/core/*.c)
PLAT     := $(wildcard src/platform/$(PLATFORM)/*.c)
GAME     := $(wildcard $(GAME_DIR)/*.c)
SRCS     := $(CORE) $(PLAT) $(GAME)

ifeq ($(PLATFORM),host)
# Native build: run and debug the same engine the retro targets will use.
CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Iinclude -DATMA_PLATFORM_HOST $(shell pkg-config --cflags sdl3)
LIBS    := $(shell pkg-config --libs sdl3)
TARGET  := $(BUILD)/atma
$(TARGET): $(SRCS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@
else ifeq ($(PLATFORM),zxn)
# ZX Spectrum Next build via zcc (z88dk): SDCC backend, size-optimized,
# emitting a .nex file. The gfx backend drives the Next's hardware ports
# directly, so no sprite library joins the link; the remaining planned
# library is vt_sound, when the sound backend stops being a stub.
CC      := zcc
CFLAGS  := +zxn -vn -SO3 --opt-code-size -clib=sdcc_iy -Iinclude
TARGET  := $(BUILD)/atma.nex
$(TARGET): $(SRCS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -startup=31 -create-app -subtype=nex -o $(BUILD)/atma
endif

# ZEsarUX (native Linux Next emulator) used by `make PLATFORM=zxn run`.
# Looked up on PATH by name; override with ZESARUX=/path/to/zesarux.
# NB: ZEsarUX loads its machine ROMs from the real binary's own directory,
# so a name that resolves through a symlink into elsewhere can fail with
# "Unable to open rom file tbblue_loader.rom". If that happens, point
# ZESARUX at the real binary.
ZESARUX ?= zesarux

.PHONY: run clean
run: $(TARGET)
ifeq ($(PLATFORM),host)
	./$(TARGET)
else
	@command -v $(ZESARUX) >/dev/null 2>&1 || { \
	  echo "ZEsarUX not found as '$(ZESARUX)'. Install it, or point at the binary:"; \
	  echo "  make PLATFORM=zxn GAME_DIR=demo run ZESARUX=/path/to/zesarux"; \
	  exit 1; }
	$(ZESARUX) --noconfigfile --machine TBBlue --snap $(TARGET)
endif
clean:
	rm -rf build
