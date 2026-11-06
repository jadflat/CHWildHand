"""Wild Hand's own 5x7 font: ASCII pictures -> Font.h (+ out/font.png preview).

python tools/make_font.py   (from CHWildHand/CHWildHand)

Never edit Font.h by hand. TextDraw.h draws with it (gfx_text, gfx_textFx ... in place of CHGfx's).
Characters ' ' to 'Z' (ASCII 32..90); TextDraw.h draws lower case as upper case and anything else as '?'.
Every glyph is 5 columns x 7 rows in a 6 px advance, like the library's font, so no layout moves.
Packing: 35 bits a glyph, row-major, LSB first: bit 35*(ch-32) + 5*row + col.
"""
import os
import sys

FIRST, LAST = 32, 90

# Bands of glyphs: a header line names each glyph at column 6*k, then its 7 rows ('#' ink, '.' blank).
# The space is blank and not drawn here.
ART = r"""
!     "     #     $     %     &     '     (     )     *
..#.. .#.#. .#.#. ..#.. ##..# .##.. ..#.. ..##. .##.. .....
..#.. .#.#. ##### .#### ##.#. #..#. ..#.. .#... ...#. ..#..
..#.. ..... .#.#. #.... ...#. .##.. .#... .#... ...#. #.#.#
..#.. ..... .#.#. .###. ..#.. .#... ..... .#... ...#. .###.
..#.. ..... ##### ....# .#... #.#.# ..... .#... ...#. #.#.#
..... ..... .#.#. ####. .#.## #..#. ..... .#... ...#. ..#..
..#.. ..... ..... ..#.. #..## .##.# ..... ..##. .##.. .....

+     ,     -     .     /     0     1     2     3     4
..... ..... ..... ..... ....# .#### ..#.. ####. ####. #..#.
..#.. ..... ..... ..... ....# #...# .##.. ....# ....# #..#.
..#.. ..... ..... ..... ...#. #..## #.#.. ....# ....# #..#.
##### ..... .###. ..... ..#.. #.#.# ..#.. .###. .###. #####
..#.. ..... ..... ..... .#... ##..# ..#.. #.... ....# ...#.
..#.. ..#.. ..... ..... #.... #...# ..#.. #.... ....# ...#.
..... .#... ..... ..#.. #.... ####. ##### ##### ####. ...#.

5     6     7     8     9     :     ;     <     =     >
##### .#### ##### .#### .#### ..... ..... ...#. ..... .#...
#.... #.... ....# #...# #...# ..... ..... ..#.. ..... ..#..
####. #.... ....# #...# #...# ..#.. ..#.. .#... ##### ...#.
....# ####. ...#. .###. .#### ..... ..... #.... ..... ....#
....# #...# ..#.. #...# ....# ..... ..... .#... ##### ...#.
....# #...# ..#.. #...# ....# ..#.. ..#.. ..#.. ..... ..#..
####. ####. ..#.. ####. ####. ..... .#... ...#. ..... .#...

?     @     A     B     C     D     E     F     G     H
####. .###. .#### ###.. .#### ###.. ##### ##### .#### #...#
....# #...# #...# #..#. #.... #..#. #.... #.... #.... #...#
....# #.### #...# #..#. #.... #...# #.... #.... #.... #...#
..##. #.#.# ##### ####. #.... #...# ####. ####. #..## #####
..#.. #.### #...# #...# #.... #...# #.... #.... #...# #...#
..... #.... #...# #...# #.... #..#. #.... #.... #...# #...#
..#.. .#### #...# ####. .#### ###.. ##### #.... .#### #...#

I     J     K     L     M     N     O     P     Q     R
.###. ....# #...# #.... #...# #...# .#### ####. .#### ####.
..#.. ....# #..#. #.... ##.## ##..# #...# #...# #...# #...#
..#.. ....# #.#.. #.... #.#.# #.#.# #...# #...# #...# #...#
..#.. ....# ###.. #.... #...# #..## #...# ####. #...# ####.
..#.. ....# #..#. #.... #...# #...# #...# #.... #.#.# #..#.
..#.. #...# #...# #.... #...# #...# #...# #.... #..#. #...#
.###. .###. #...# ##### #...# #...# ####. #.... .##.# #...#

S     T     U     V     W     X     Y     Z
.#### ##### #...# #...# #...# #...# #...# #####
#.... ..#.. #...# #...# #...# #...# #...# ....#
#.... ..#.. #...# #...# #...# .#.#. .#.#. ...#.
.###. ..#.. #...# #...# #.#.# ..#.. ..#.. ..#..
....# ..#.. #...# .#.#. #.#.# .#.#. ..#.. .#...
....# ..#.. #...# .#.#. ##.## #...# ..#.. #....
####. ..#.. ####. ..#.. #...# #...# ..#.. #####
"""


def parse(art):
    glyphs = {}
    lines = [l.rstrip() for l in art.strip('\n').split('\n')]
    i = 0
    while i < len(lines):
        if not lines[i].strip():
            i += 1
            continue
        head, rows = lines[i], lines[i + 1:i + 8]
        assert len(rows) == 7, 'band at line %d: 7 rows' % i
        for k in range(0, len(head), 6):
            ch = head[k]
            if ch == ' ':
                continue
            g = [r[k:k + 5] for r in rows]
            assert all(len(r) == 5 and set(r) <= set('#.') for r in g), 'glyph %r' % ch
            assert ch not in glyphs, 'glyph %r twice' % ch
            glyphs[ch] = g
        i += 8
    glyphs[' '] = ['.....'] * 7
    missing = [chr(c) for c in range(FIRST, LAST + 1) if chr(c) not in glyphs]
    assert not missing, 'missing glyphs: %r' % missing
    return glyphs


def pack(glyphs):
    bits = []
    for c in range(FIRST, LAST + 1):
        for row in glyphs[chr(c)]:
            bits += [1 if p == '#' else 0 for p in row]
    n = (len(bits) + 7) // 8
    # the renderer reads two bytes at a time: the last row's second byte must exist
    last = 35 * (LAST - FIRST) + 30
    n = max(n, last // 8 + 2)
    data = bytearray(n)
    for i, b in enumerate(bits):
        if b:
            data[i >> 3] |= 1 << (i & 7)
    return bytes(data)


def header(data):
    out = ['// Font.h: Wild Hand\'s own 5x7 font, written by tools/make_font.py (the pictures are there). Do not edit.',
           '// Characters %r..%r; 35 bits a glyph, row-major, LSB first: bit 35 * (ch - 32) + 5 * row + col.' % (chr(FIRST), chr(LAST)),
           '#pragma once',
           '#include <stdint.h>',
           '',
           '#define WH_FONT_LAST %d' % LAST,
           'static const uint8_t WH_FONT[%d] = {' % len(data)]
    for i in range(0, len(data), 16):
        out.append('    ' + ', '.join('0x%02X' % b for b in data[i:i + 16]) + ',')
    out.append('};')
    return '\n'.join(out) + '\n'


def preview(glyphs, path):
    try:
        from PIL import Image
    except ImportError:
        return
    chars = [chr(c) for c in range(FIRST, LAST + 1)]
    cols, sc = 16, 4
    rows = (len(chars) + cols - 1) // cols
    img = Image.new('RGB', (cols * 6 * sc + sc, rows * 9 * sc + sc), (16, 12, 20))
    px = img.load()
    for n, ch in enumerate(chars):
        ox, oy = (n % cols) * 6 * sc + sc, (n // cols) * 9 * sc + sc
        for r, row in enumerate(glyphs[ch]):
            for c, p in enumerate(row):
                if p == '#':
                    for y in range(sc):
                        for x in range(sc):
                            px[ox + c * sc + x, oy + r * sc + y] = (236, 220, 200)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.save(path)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    sketch = os.path.dirname(here)
    glyphs = parse(ART)
    data = pack(glyphs)
    with open(os.path.join(sketch, 'Font.h'), 'w', newline='') as f:
        f.write(header(data))
    preview(glyphs, os.path.join(sketch, 'out', 'font.png'))
    print('Font.h: %d glyphs, %d B' % (LAST - FIRST + 1, len(data)))


if __name__ == '__main__':
    sys.exit(main())
