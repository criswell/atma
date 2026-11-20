# atma - cross-platform retro engine.
#
# Pick a backend with PLATFORM=<name>. Core (src/core) and game (game) are
# shared; each backend contributes whatever .c files sit in
# src/platform/<name>/, so dropping a new .c file there is enough to add it
# to the build. host builds natively with gcc; zxn needs the z88dk toolchain.

# GAME_DIR picks the program to build (default game; try GAME_DIR=demo).
PLATFORM ?= host
GAME_DIR ?= game

# Typo guards. make silently ignores unknown variables, so a misspelled
# GAME_* assignment quietly builds the default program instead
# (GAME_IDIR=demo once built `game` and looked like a black-screen bug).
# Catch both a misspelled GAME_* variable and a GAME_DIR with no sources.
UNKNOWN_GAME_VARS := $(filter-out GAME_DIR,$(filter GAME_%,$(.VARIABLES)))
ifneq ($(UNKNOWN_GAME_VARS),)
$(error Unknown variable: $(UNKNOWN_GAME_VARS). make ignores unknown variables. Did you mean GAME_DIR? GAME_DIR is currently '$(GAME_DIR)')
endif
ifeq ($(wildcard $(GAME_DIR)/*.c),)
$(error GAME_DIR '$(GAME_DIR)' contains no .c files. Dirs with sources: $(foreach d,$(wildcard */),$(if $(wildcard $(d)*.c),$(d))))
endif

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

# Documentation pipeline. Doxygen reads the C sources and emits XML only;
# doxybook2 turns that XML into plain Markdown under docs/api; Zensical
# builds docs/ (handwritten pages plus docs/api) into one site.
# docs/api is generated, so it is gitignored and rebuilt by `make docs`.
DOCS_DIR   := docs
DOCS_API   := $(DOCS_DIR)/api
DOCS_XML   := build/doxygen/xml
DOCS_SITE  := build/docs/site
DOXYFILE   := Doxyfile
DOCS_TPL   := tools/doxybook2/templates
# doxybook2 slugifies "_" to "-" in link anchors but leaves the headings it
# emits untouched, so every function anchor would miss its heading. Keep
# underscores intact so anchors match. useFolders=false keeps every page in
# docs/api itself: doxybook2 writes links relative to the output root, which
# only resolve when the linking page sits in that same directory.
DOXYBOOK2_CONFIG := {"replaceUnderscoresInAnchors": false, "useFolders": false}
DOXYGEN    ?= doxygen
DOXYBOOK2  ?= doxybook2
ZENSICAL   ?= zensical

.PHONY: run clean docs docs-api docs-serve docs-clean
run: $(TARGET)
ifeq ($(PLATFORM),host)
	./$(TARGET)
else
	@command -v $(ZESARUX) >/dev/null 2>&1 || { \
	  echo "ZEsarUX not found as '$(ZESARUX)'. Install it, or point at the binary:"; \
	  echo "  make PLATFORM=zxn GAME_DIR=demo run ZESARUX=/path/to/zesarux"; \
	  exit 1; }
	$(ZESARUX) --noconfigfile --machine TBBlue --tbblue-fast-boot-mode --allowturbotbbluerom --snap $(TARGET)
endif
docs: docs-api
	@command -v $(ZENSICAL) >/dev/null 2>&1 || { \
	  echo "$(ZENSICAL) not found. Install it: pipx install zensical"; \
	  exit 1; }
	$(ZENSICAL) build

# Doxygen emits XML only; doxybook2 converts it to Markdown in docs/api.
# The output dir is wiped first so pages for deleted symbols do not linger.
docs-api:
	@command -v $(DOXYGEN) >/dev/null 2>&1 || { \
	  echo "$(DOXYGEN) not found. Install it: sudo zypper install doxygen"; \
	  exit 1; }
	@command -v $(DOXYBOOK2) >/dev/null 2>&1 || { \
	  echo "$(DOXYBOOK2) not found. Install it from"; \
	  echo "  https://github.com/matusnovak/doxybook2/releases"; \
	  exit 1; }
	@# Doxygen creates only the leaf of OUTPUT_DIRECTORY + XML_OUTPUT, not the
	@# parents; a fresh checkout has no build/ so create it ourselves.
	@mkdir -p $(DOCS_XML)
	$(DOXYGEN) $(DOXYFILE)
	rm -rf $(DOCS_API)
	@mkdir -p $(DOCS_API)
	$(DOXYBOOK2) --input $(DOCS_XML) --output $(DOCS_API) \
	  --templates $(DOCS_TPL) --config-data '$(DOXYBOOK2_CONFIG)'
	@# doxybook2 links files/dirs/pages with synthetic "#file-...", "#dir-..."
	@# and "#page-..." anchors that no heading in the target page defines, so
	@# drop them and let the links land on the page itself.
	find $(DOCS_API) -name '*.md' -print0 | xargs -0 sed -i -E \
	  's/(\.md)#(file|dir|page)-[^)]*/\1/g'

docs-serve: docs-api
	@command -v $(ZENSICAL) >/dev/null 2>&1 || { \
	  echo "$(ZENSICAL) not found. Install it: pipx install zensical"; \
	  exit 1; }
	$(ZENSICAL) serve

docs-clean:
	rm -rf $(DOCS_XML) $(DOCS_API) $(DOCS_SITE)

clean:
	rm -rf build
