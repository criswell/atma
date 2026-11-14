# atma

# WHAT

`atma` is _intended_ to be a cross-platform game engine that targets classic
retro computers, but which can also build on modern PCs.

(TODO)

# REQUIREMENTS

## PC

- A C99 compiler (gcc is what the Makefile uses)
- GNU make
- pkg-config
- SDL3 (3.4 or newer, with development headers)
- Python 3 (to run tools/mkpats.py, the asset generator; standard library only)

# DESIGN NOTES

## One palette, the Next's colors

The engine is indexed-color everywhere: you set palette entries with
`atma_gfx_palette_set()` (RGB333, the Next's 9-bit color space) and every
color drawn is an index into that table. The host is not more capable than
the Next here: it quantizes to the same 512-color space, so what you see in
the window is what the Next will show.

Index 0 is the transparent color for sprites on every backend.

## Assets are compiled in

Games don't load image files at runtime. `tools/mkpats.py` converts a PNG
into generated C: a palette table plus 16x16 cell patterns of palette
indices (the Next's native sprite atom). Both backends consume the same
bytes, so the demo builds unchanged for host and zxn. Regenerate after
editing the source image:

```
python3 tools/mkpats.py demo/atma.png demo/atma_patterns
```

# DEMO

```
make GAME_DIR=demo run
```
