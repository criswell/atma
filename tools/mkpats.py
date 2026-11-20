#!/usr/bin/env python3
"""Convert a PNG into C arrays the engine consumes on every platform.

Usage: python3 tools/mkpats.py <image.png> <output-base>

Outputs <output-base>.h and <output-base>.c containing:

  - the image's palette as {index, r, g, b} rows: entry 0 is the
    transparent color (source pixels with alpha < 128). Transparency is
    by index on every backend, so entry 0's RGB is never drawn; it is
    still chosen as a marker (an 8-bit RRRGGGBB form no real color
    shares) to keep the transparent slot visually distinct in the
    generated table. Every other color is quantized to the Next's 9-bit
    RGB333 space (3 bits red, 3 green, 3 blue; 512 colors) and expanded
    back to 8-bit components, so the numbers are exactly what both the
    host window and the Next hardware show;

  - the image sliced into 16x16 cell patterns (the Next's native sprite
    atom), each cell one byte per pixel holding a palette index from that
    table. Cells are stored row-major; cells overlapping the right/bottom
    edges are padded with index 0 (transparent).

Byte value N in a cell means "palette table row N". A game uploads the
table with atma_gfx_palette_set() at the same engine slots, so cell bytes
are engine palette indices too.

Nothing here knows about SDL or z88dk: the same arrays compile for every
backend. Requires Python 3 only (hand-rolled PNG decoder, no imports
beyond stdlib).
"""

import struct
import sys
import zlib

CELL = 16


def die(msg):
    print(f"mkpats: {msg}", file=sys.stderr)
    sys.exit(1)


def read_png(path):
    """Decode a non-interlaced 8-bit RGB/RGBA PNG into (w, h, pixels).

    pixels is a flat list of (r, g, b, a) tuples, row-major.
    """
    try:
        data = open(path, "rb").read()
    except OSError as e:
        die(str(e))
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        die(f"{path}: not a PNG file")

    pos = 8
    idat = b""
    header = None
    while pos < len(data):
        length, ctype = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        if ctype == b"IHDR":
            header = struct.unpack(">IIBBBBB", body)
        elif ctype == b"IDAT":
            idat += body
        elif ctype == b"IEND":
            break
        pos += 12 + length

    if header is None:
        die(f"{path}: missing IHDR")
    w, h, depth, ctype, _comp, _filt, interlace = header
    if depth != 8:
        die(f"{path}: bit depth {depth} not supported (want 8)")
    if interlace:
        die(f"{path}: interlaced PNGs not supported")
    if ctype == 6:
        bpp = 4
    elif ctype == 2:
        bpp = 3
    else:
        die(f"{path}: color type {ctype} not supported (want RGB or RGBA)")

    try:
        raw = zlib.decompress(idat)
    except zlib.error as e:
        die(f"{path}: bad image data ({e})")
    stride = w * bpp
    if len(raw) != (stride + 1) * h:
        die(f"{path}: truncated image data")

    def paeth(a, b, c):
        p = a + b - c
        pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
        if pa <= pb and pa <= pc:
            return a
        if pb <= pc:
            return b
        return c

    rows, prev = [], bytearray(stride)
    o = 0
    for _y in range(h):
        ftype = raw[o]
        o += 1
        line = bytearray(raw[o:o + stride])
        o += stride
        for x in range(stride):
            a = line[x - bpp] if x >= bpp else 0
            b = prev[x]
            c = prev[x - bpp] if x >= bpp else 0
            if ftype == 1:
                line[x] = (line[x] + a) & 255
            elif ftype == 2:
                line[x] = (line[x] + b) & 255
            elif ftype == 3:
                line[x] = (line[x] + (a + b) // 2) & 255
            elif ftype == 4:
                line[x] = (line[x] + paeth(a, b, c)) & 255
        rows.append(bytes(line))
        prev = line

    pixels = []
    for line in rows:
        for x in range(0, stride, bpp):
            px = line[x:x + bpp]
            alpha = px[3] if bpp == 4 else 255
            pixels.append((px[0], px[1], px[2], alpha))
    return w, h, pixels


def to_rgb333(r, g, b):
    """Quantize to the Next's 9-bit RGB333 space (3/3/3), expanded back to
    8-bit components."""
    return (r >> 5) * 255 // 7, (g >> 5) * 255 // 7, (b >> 5) * 255 // 7


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if len(args) != 2:
        die(f"usage: {sys.argv[0]} <image.png> <output-base>")
    png, out_base = args

    w, h, pixels = read_png(png)

    # Palette: entry 0 = transparent, then first-seen order after RGB333
    # quantization. Entry 0 carries a marker color chosen below: nothing
    # matches on it at run time (transparency is by index), but keeping
    # it off every real color's RRRGGGBB form makes the transparent slot
    # visually distinct in the generated table.
    palette = [None]  # entry 0 placeholder; the marker color is chosen below
    index_of = {}
    for r, g, b, alpha in pixels:
        if alpha < 128:
            continue
        key = to_rgb333(r, g, b)
        if key not in index_of:
            if len(palette) >= 256:
                die(f"{png}: more than 255 colors after RGB333 "
                    "quantization; simplify the art")
            index_of[key] = len(palette)
            palette.append((len(palette), *key))

    def byte333(r, g, b):
        """A color's 8-bit RRRGGGBB form (the top bits of each channel)."""
        return ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6)

    used = {byte333(r, g, b) for _i, r, g, b in palette[1:]}
    marker = next((c for c in range(256) if c not in used), None)
    if marker is None:
        die(f"{png}: every 8-bit color value used; no unique transparent "
            "marker exists, simplify the art")
    # Expand the marker byte to the RGB333 color it names (blue LSB 0).
    mr, mg, mb = (marker >> 5) & 7, (marker >> 2) & 7, (marker & 3) << 1
    palette[0] = (0, mr * 255 // 7, mg * 255 // 7, mb * 255 // 7)

    # Slice into cells, row-major, padding edge cells with transparency.
    cols = (w + CELL - 1) // CELL
    rows_n = (h + CELL - 1) // CELL
    cells = []
    for cr in range(rows_n):
        for cc in range(cols):
            cell = [0] * (CELL * CELL)
            for y in range(CELL):
                iy = cr * CELL + y
                if iy >= h:
                    break
                for x in range(CELL):
                    ix = cc * CELL + x
                    if ix < w:
                        pr, pg, pb_, pa = pixels[iy * w + ix]
                        cell[y * CELL + x] = (index_of[to_rgb333(pr, pg, pb_)]
                                              if pa >= 128 else 0)
            cells.append(cell)

    stem = out_base.rsplit("/", 1)[-1]
    guard = stem.upper() + "_H"

    h_path = out_base + ".h"
    c_path = out_base + ".c"

    with open(h_path, "w") as f:
        f.write(f"""/* Generated by tools/mkpats.py from {png}; do not edit by hand.
 * Rerun the tool after changing the source image instead. */

#ifndef {guard}
#define {guard}

/* Palette rows {{index, r, g, b}}: Next RGB333 values expanded to 8-bit
 * components. Entry 0 is the transparent color; its RGB is a marker the
 * generator picks to keep the transparent slot visually distinct
 * (nothing matches on it at run time: transparency is by index). */
#define {stem.upper()}_PALETTE_N {len(palette)}
extern const unsigned char {stem}_palette[{stem.upper()}_PALETTE_N][4];

/* The image as {len(cells)} cell(s) of {CELL}x{CELL} palette indexes
 * ({cols} column(s) x {rows_n} row(s), row-major), padded with index 0
 * where the image doesn't fill a cell. */
#define {stem.upper()}_CELLS {len(cells)}
#define {stem.upper()}_COLS {cols}
#define {stem.upper()}_ROWS {rows_n}
extern const unsigned char {stem}_cells[{stem.upper()}_CELLS][{CELL} * {CELL}];

#endif
""")

    with open(c_path, "w") as f:
        f.write(f"""/* Generated by tools/mkpats.py from {png}; do not edit by hand. */

#include "{stem}.h"

const unsigned char {stem}_palette[{stem.upper()}_PALETTE_N][4] = {{
""")
        for idx, r, g, b in palette:
            f.write(f"    {{ {idx}, {r}, {g}, {b} }},\n")
        f.write("};\n\n")
        f.write(f"const unsigned char {stem}_cells[{stem.upper()}_CELLS]"
                f"[{CELL} * {CELL}] = {{\n")
        for cell in cells:
            f.write("    {\n")
            for y in range(CELL):
                row = cell[y * CELL:(y + 1) * CELL]
                f.write("        " + ", ".join(str(v) for v in row) + ",\n")
            f.write("    },\n")
        f.write("};\n")

    print(f"{png}: {w}x{h}, {len(palette)} palette entries "
          f"(incl. transparent), {len(cells)} cell(s) -> {h_path}, {c_path}")


if __name__ == "__main__":
    main()
