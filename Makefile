PLATFORM ?= host
BUILD    := build/$(PLATFORM)
CORE     := $(wildcard src/core/*.c)
PLAT     := $(wildcard src/platform/$(PLATFORM)/*.c)
GAME     := $(wildcard game/*.c)
SRCS     := $(CORE) $(PLAT) $(GAME)

ifeq ($(PLATFORM),host)
CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Iinclude -D_POSIX_C_SOURCE=200809L
TARGET  := $(BUILD)/atma
$(TARGET): $(SRCS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@
else ifeq ($(PLATFORM),zxn)
CC      := zcc
CFLAGS  := +zxn -vn -SO3 --opt-code-size -clib=sdcc_iy -Iinclude
LIBS    := -lzxnext_sprite -lzxnext_layer2 -lvt_sound
TARGET  := $(BUILD)/atma.nex
$(TARGET): $(SRCS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $^ $(LIBS) -startup=31 -create-app -subtype=nex -o $(BUILD)/atma
endif

.PHONY: run clean
run: $(TARGET)
	./$(TARGET)
clean:
	rm -rf build
