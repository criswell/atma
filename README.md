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

## ZX Spectrum Next (PLATFORM=zxn)

- The **z88dk** toolchain, built from source (distro packages are too old
  for the Next's Z80N instructions and SDCC variant). A git checkout
  built with `./build.sh`; the tree this repo is developed against is
  `~/work/z88dk` (v2.5+, current-ish master).
- Two environment variables, from z88dk's `set_environment.sh`:

  ```
  export PATH=~/path/to/z88dk/bin:$PATH
  export ZCCCFG=~/path/to/z88dk/lib/config
  ```

  `ZCCCFG` is the one that bites: without it `zcc +zxn` cannot find the
  zxn target config and dies confusingly.
- Binaries a zxn build actually touches, all from that `bin/`: `zcc`
  (frontend that drives everything), `zsdcc`/`z88dk-zsdcc` (the SDCC
  compiler proper), `z80asm`, `z88dk-ucpp` (preprocessor), `z88dk-copt`
  (peephole pass), `z88dk-appmake` (produces the `.nex`).
- Build prerequisites for z88dk itself: `gcc`/`clang`, `make`, `autoconf`,
  `libxml2` and its dev headers, `boost` and its dev headers, git. The
  build is long; see z88dk's docs.
- **zxnext_sprite** (github.com/stefanbylund/zxnext_sprite): thin C
  wrapper over the Next's hardware-sprite I/O ports, with prebuilt
  `lib/sdcc_iy/zxnext_sprite.lib` matching the engine's `-clib=sdcc_iy`
  convention. It joins the link when the zxn gfx backend stops being a
  stub. A sibling repo, `~/work/zxnext_sprite`, has the sources and a
  demo.
- Running the output: a `.nex` file boots on real Next hardware (from the
  SD card menu) or in an emulator (CSpect, ZEsarUX, zx).

# DESIGN NOTES

## One palette, the Next's colors

The engine is indexed-color everywhere: you set palette entries with
`atma_gfx_palette_set()` (RGB333, the Next's 9-bit color space, 512
colors) and every color drawn is an index into that table. The host is not
more capable than the Next here: it quantizes to the same 512-color space,
so what you see in the window is what the Next will show.

Palette entries are live state: change one and every pixel using that
index, sprites included, recolors on the next frame, on every platform.
Palette animation is portable.

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
