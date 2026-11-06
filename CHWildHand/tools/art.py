"""Wild Hand art: ASCII grids -> Art.h (run-length rows) + out/art.png preview.

python tools/art.py   (from CHWildHand/CHWildHand)
Each grid character is one palette index (see PAL); '.' is transparent.
Rows are stored as runs: (len-1)<<4 | colour, 15 = skip, one row per line,
row starts with its run count.
"""
import os, sys

# palette index per char, and the RGB used for the preview (keep in step with setup())
PAL = {
    'z': (0, (10, 23, 18)), 'f': (1, (27, 90, 60)), 'w': (2, (243, 236, 216)), 'r': (3, (208, 52, 60)),
    'k': (4, (29, 27, 41)), 'g': (5, (233, 185, 73)), 'e': (6, (142, 163, 148)), 'd': (7, (15, 58, 39)),
    'W': (8, (246, 242, 232)), 'b': (9, (63, 127, 216)), 's': (10, (242, 180, 140)), 'y': (11, (255, 244, 176)),
    'l': (12, (35, 112, 75)), 'h': (13, (201, 191, 166)), 'G': (14, (154, 106, 36)), 'R': (15, (122, 34, 50)),
    'c': (4, (29, 27, 41)),   # suit colour slot: remapped at draw time (drawn as BLK by default)
}
# 'c' must be distinguishable at draw time: store it as index 4 but via a remap table the
# card code swaps 4 -> RED for red suits.  Outlines that must stay black use 'k'... so 'c'
# gets its own code below.
CCODE = 15   # never used: we store 'c' as 3 (RED) and remap 3 -> BLK for black suits

ART = {}

def grid(name, rows):
    w = len(rows[0])
    for i, r in enumerate(rows):
        assert len(r) == w, f"{name} row {i} is {len(r)} wide, not {w}"
        for ch in r:
            assert ch == '.' or ch in PAL, f"{name}: bad char {ch!r}"
    ART[name] = rows

# ---- horror theme (2026-11): a haunted card house. Colour roles by character: k ink, w bone, W pale/white,
# r blood (suit colour on the courts), R deep wine, g candle amber, G tarnished bronze, y flame, e grave grey,
# h bone shadow, s pallid flesh, l sickly green, d moss, b night purple (once the palette has it).

def M(rows):
    """Mirror half rows (left half given) into full rows."""
    return [r + r[::-1] for r in rows]


# ---- rank glyphs 6x8 (bold), '10' fitted in 6
RANKS = {
 '2': [".####.", "##..##", "....##", "...##.", "..##..", ".##...", "##....", "######"],
 '3': ["#####.", "....##", "....##", ".####.", "....##", "....##", "....##", "#####."],
 '4': ["...##.", "..###.", ".####.", "##.##.", "######", "...##.", "...##.", "...##."],
 '5': ["######", "##....", "#####.", "....##", "....##", "....##", "##..##", ".####."],
 '6': [".####.", "##....", "##....", "#####.", "##..##", "##..##", "##..##", ".####."],
 '7': ["######", "....##", "...##.", "...##.", "..##..", "..##..", ".##...", ".##..."],
 '8': [".####.", "##..##", "##..##", ".####.", "##..##", "##..##", "##..##", ".####."],
 '9': [".####.", "##..##", "##..##", "##..##", ".#####", "....##", "....##", ".####."],
 'T': ["##..###.", "##.##.##", "##.##.##", "##.##.##", "##.##.##", "##.##.##", "##.##.##", "##..###."],
 'J': ["..####", "....##", "....##", "....##", "....##", "##..##", "##..##", ".####."],
 'Q': [".####.", "##..##", "##..##", "##..##", "##..##", "##.###", ".####.", "....##"],
 'K': ["##..##", "##.##.", "####..", "###...", "####..", "##.##.", "##..##", "##..##"],
 'A': [".####.", "##..##", "##..##", "##..##", "######", "##..##", "##..##", "##..##"],
}
SUIT7 = [
 [".##.##.", "#######", "#######", "#######", ".#####.", "..###..", "...#..."],   # heart
 ["...#...", "..###..", ".#####.", "#######", ".#####.", "..###..", "...#..."],   # diamond
 ["..###..", "..###..", "##.#.##", "#######", "##.#.##", "...#...", "..###.."],   # club
 ["...#...", "..###..", ".#####.", "#######", "#######", "...#...", "..###.."],   # spade
]

# ---- court portraits 12x16; 'r' = suit colour (remapped for black suits). Hollow, regal, decayed.
grid('JACK', M([  # the knave: a hooded mourner, empty sockets
 "....kk",
 "...krr",
 "..krrr",
 ".krrkk",
 ".krkee",
 ".krkke",
 ".krere",
 ".krkek",
 ".krrkh",
 "..krrk",
 ".kGrrr",
 "kGrrrw",
 "krrrrw",
 "kRrrrG",
 "krrrrG",
 "kkkkkk",
]))
grid('QUEEN', M([  # the widow: a spiked crown, a veil, tears of blood, a sewn mouth
 "..g..g",
 "..gkgk",
 ".kgggg",
 "kRGGGG",
 "kRkeee",
 "kRkkke",
 "kRerre",
 "kRerre",
 "kRReek",
 "kRRkwk",
 "kRkeee",
 "kkwwww",
 "krwrwr",
 "krrrrg",
 "kRrrrg",
 "kkkkkk",
]))
grid('KING', M([  # the dead king: a skull under a heavy crown
 ".g..g.",
 ".gkgkg",
 ".kgggg",
 ".kGrGG",
 ".kwwww",
 "kwwwww",
 "kwkkkw",
 "kwkrkw",
 "kwwwwk",
 ".kwkwk",
 "..kkkk",
 "krrrgk",
 "krrrgr",
 "kgrrgr",
 "krrrgr",
 "kkkkkk",
]))

# ---- the wild cards, 16x16 each
grid('W0', M([  # BIG MULT: a horned demon skull
 "kk......",
 "kwk.....",
 "kwhk....",
 ".khwk...",
 "..kwrkkk",
 "..krrrrr",
 ".krrrrrr",
 ".krkkkrr",
 ".krkykrr",
 ".krrrrrk",
 ".kRrrrrr",
 "..kRkwkw",
 "..kRkkkk",
 "...kRRRR",
 "....kkkk",
 "........",
]))
grid('W1', [  # BIG CHIPS: a stack of bone chips, the top one marked with an eye (2026-11: rounded, lit top-left)
 "................",
 "....kkkkkkkk....",
 "..kkWWwwwwwwkk..",
 ".kWwwwkkkkwwwhk.",
 ".kWwwkrrWrkwwhk.",
 ".kwwwwkkkkwwhhk.",
 ".kkhhhhhhhhhhkk.",
 ".kWrrwrrwRRwRRk.",
 ".krrrrrrRRRRRRk.",
 ".kkkkkkkkkkkkkk.",
 ".kwrrwrrwRRwRRk.",
 ".krrrrrrRRRRRRk.",
 ".kkkkkkkkkkkkkk.",
 ".kwrrwrrwRRwRRk.",
 "..krrrrrRRRRRk..",
 "...kkkkkkkkkk...",
])
grid('W2', [  # HEARTS: a stitched heart, still dripping
 "....kk...kk.....",
 "...kRRk.kRRk....",
 "..kkkRkkkRkkkk..",
 ".krrrRrrkRrrRRk.",
 "krrwrrrrkrrrrRRk",
 "krwrrrrwkwrrrRRk",
 "krrrrrrrkrrrrRRk",
 "krrrrrrwkwrrrRRk",
 ".krrrrrrkrrrRRk.",
 ".krrrrrwkwrrRRk.",
 "..krrrrrkrrRRk..",
 "...krrrrkrRRk...",
 "....krrrrRRk....",
 ".....kRrRRk.....",
 "......kRrk......",
 ".......kr.......",
])
grid('W3', [  # SPADES: a gravedigger's spade in fresh earth
 ".....kkkkk......",
 ".....khGhk......",
 "......kGk.......",
 "......kGk.......",
 "......kGk.......",
 ".....kkGkk......",
 "....keeeeek.....",
 "....kWeeeek.....",
 "....kWeeeek.....",
 "....keeeeek.....",
 ".....keeek......",
 "...kkkGeGkkk....",
 "..kGGGGkGGGGk...",
 ".kGhGGGGGhGGGk..",
 "kGGGGwwGGGGGhGk.",
 "kkkkkkkkkkkkkkk.",
])
grid('W4', [  # TWINS: two masks, one grinning (bone), one weeping blood (grey) (2026-11: overlapped, shaded)
 ".......kkkkkkkk.",
 "......khhhhhhhek",
 "......khkkhhkkek",
 "......khhkhhkhek",
 "......khrhhhhrek",
 ".kkkkkkkkhhhhrek",
 "kWWwwwwwhkhkkkek",
 "kWkkwwkkhkkeeekk",
 "kkwwkkwwhk.kkkk.",
 "kWwwwwwwhk......",
 "kwkwwwwkhk......",
 "kwwkkkkwhk......",
 ".kwwkrkwk.......",
 ".khwwwwhk.......",
 "..khhhhk........",
 "...kkkk.........",
])
grid('W5', M([  # FACES: a crowned skull
 "........",
 "........",
 ".k...k..",
 "kgk.kgk.",
 "kggkgggk",
 "kgrgggyg",
 "kGGGGGGG",
 ".kwwwwww",
 "kwkkkwww",
 "kwkkkwww",
 "kwwwwwwk",
 ".kwwwwww",
 "..kwkwkw",
 "..kkwkwk",
 "...kkkkk",
 "........",
]))
grid('W6', [  # LINES: three claw marks
 "................",
 "..........k...k.",
 "......k..kr..kr.",
 ".....kr..kr.kRk.",
 ".....kr.krk.kRk.",
 "....krk.kRk.kRk.",
 "....kRk.kRkkRk..",
 "...kRk.kRk.kRk..",
 "...kRk.kRk.kRk..",
 "..kRk.kRk.kRk...",
 "..kRk.kRk.krk...",
 "..kRk.krk.kr....",
 ".kRk..krk.kr....",
 ".krk..kr..k.....",
 ".kr...k.........",
 ".k..............",
])
grid('W7', [  # ACES: a card with an open eye
 "..kkkkkkkkkkkk..",
 ".kwwwwwwwwwwwwk.",
 ".kwwwwwwwwwwwwk.",
 ".kwwwwwwwwwwwwk.",
 ".kwwwwkkkkwwwwk.",
 ".kwwkkrrrrkkwwk.",
 ".kwkrWWWWWWrkwk.",
 ".kkrWllkkllWrkk.",
 ".kkrWllkkllWrkk.",
 ".kwkrWWWWWWrkwk.",
 ".kwwkkrrrrkkwwk.",
 ".kwwwwkkrkwwwwk.",
 ".kwwwwwwrwwwwwk.",
 ".kwwwwwwrwwwwwk.",
 ".khhhhhhhhhhhhk.",
 "..kkkkkkkkkkkk..",
])

# ---- Wild Jack, now the Undertaker who keeps the shop (16x16): leering, blinking, hungry, reacting
JACK_TOP = [   # (2026-11 polish: a sheened top hat with a blood band, a bone-pale skull in the brim's shadow)
 "....kkkkkkkk....",
 "....kbbkkkkk....",
 "....kbkkkkkk....",
 "....kRrRRRRk....",
 "....kbkkkkkk....",
 "..kbbbkkkkkkkk..",
 "...kehhhhhhek...",
]
def face(brow, eye, cheek, mouth, chin="....kehhhhek...."):
    return JACK_TOP + ["...k" + r + "k..." for r in (brow, eye, "hhhkkhhe", cheek, mouth)] + [chin] + NECK
NECK = [
 "...kWWkrrkWWk...",
 ".kkbkWkrrkWkbkk.",
 "kbbkkkkRRkkkkbbk",
]
grid('MASCOT', face("Wkkhhkke", "hkrhhkre", "ehhhhhhe", "hWkWkWke"))      # the leer: red eyes slid sideways, a sewn grin
grid('MASCOT_BLINK', face("Whhhhhhe", "hkkhhkke", "ehhhhhhe", "hWkWkWke"))
grid('MASCOT_WOW', face("Wkkhhkke", "hyyhhyye", "ehWkkWhe", "hkRrrRke"))      # hungry: eyes lit, the jaw open on fangs
grid('MASCOT_SAD', face("kkhhhhkk", "hrkhhkre", "ehkkkkhe", "hkhhhhke"))      # reacting: a sour scowl, eyes down


# ---- gore pass (2026-11, 1.1 "more blood"). On the courts blood is R (wine): 'r' there is the suit colour and turns black.
def G(rows, pts):
    g = [list(r) for r in rows]
    H, W = len(g), len(g[0])
    for x, y, ch in pts:
        g[y][x] = ch
    for x, y, ch in pts:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            xx, yy = x + dx, y + dy
            if 0 <= xx < W and 0 <= yy < H and g[yy][xx] == '.':
                g[yy][xx] = 'k'
    return [''.join(r) for r in g]


def S(ch, *xy):
    return [(x, y, ch) for x, y in xy]


GORE = {
    'JACK': S('R', (4, 7), (7, 7), (5, 8), (6, 11), (5, 12), (5, 13), (5, 14), (8, 13)) + S('w', (6, 12)),          # bleeding eyes and mouth, gory hands
    'QUEEN': S('R', (3, 8), (8, 8), (4, 10), (5, 10), (6, 10), (7, 10), (4, 11), (6, 11), (7, 11), (6, 12)),        # blood tears, a cut throat
    'KING': S('R', (3, 4), (3, 5), (8, 4), (8, 5), (3, 8), (8, 8), (4, 10), (7, 10), (4, 11)) + S('k', (6, 4), (5, 5)),   # the crown bites in
    'W0': S('r', (5, 14), (5, 15), (10, 14), (6, 13)) + S('R', (10, 15)),                                           # BIG MULT: the demon drools
    'W3': S('r', (7, 9), (8, 9), (7, 10), (8, 8), (7, 11)) + S('R', (6, 9), (7, 12)),                               # SPADES: a bloody spade
    'W7': S('r', (3, 4), (12, 3), (2, 12), (13, 11), (12, 12)) + S('R', (8, 14)),                                   # ACES: the card is spattered
}
for _n in ('MASCOT', 'MASCOT_BLINK', 'MASCOT_WOW', 'MASCOT_SAD'):                                                    # the Undertaker: drool and spatter
    GORE[_n] = S('r', (7, 12), (10, 9), (5, 6), (12, 13)) + S('R', (11, 9), (7, 14))
for _n, _p in GORE.items():
    ART[_n] = G(ART[_n], _p)


def fix_widths():
    for n, rows in ART.items():
        w = max(len(r) for r in rows)
        ART[n] = [r.ljust(w, '.') for r in rows]


def bits(rows):
    # 1-bit glyph rows, MSB = left
    return [int(''.join('1' if c == '#' else '0' for c in r).ljust(8, '0'), 2) for r in rows]


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    fix_widths()
    # WINE (15) is also the skip code, so wine pixels use a separate escape:
    # we store wine as colour 0 (BG) and the drawer maps 0 -> 15. BG is never used in art.
    for n, rows in ART.items():
        for r in rows:
            assert 'z' not in r, n
    L = ["// Generated by tools/art.py: do not edit by hand.", "#pragma once", ""]
    L.append("// rank glyphs 6x8, one byte a row (MSB left): 2..9, T(10), J, Q, K, A")
    L.append("static const uint8_t RANK8[13][8] = {")
    for k in "23456789TJQKA":
        L.append("    { " + ", ".join(f"0x{b:02X}" for b in bits(RANKS[k])) + " },")
    L.append("};")
    L.append("// suit pips 7x7 (MSB left): H D C S")
    L.append("static const uint8_t SUIT7[4][7] = {")
    for s in SUIT7:
        L.append("    { " + ", ".join(f"0x{b:02X}" for b in bits(s)) + " },")
    L.append("};")
    for n, rows in ART.items():
        data = []
        w, h = len(rows[0]), len(rows)
        for r in rows:
            r = r.rstrip('.')
            rr = []
            x = 0
            while x < len(r):
                ch = r[x]; m = 1
                while x + m < len(r) and r[x + m] == ch and m < 16: m += 1
                col = 15 if ch == '.' else (0 if ch == 'R' else PAL[ch][0])
                rr.append(((m - 1) << 4) | col)
                x += m
            data += [len(rr)] + rr
        L.append(f"static const uint8_t ART_{n}[] = {{ {w}, {h}, " + ", ".join(str(v) for v in data) + " };")
    L.append("static const uint8_t *const JART[8] = { " + ", ".join(f"ART_W{i}" for i in range(8)) + " };")
    open(os.path.join(root, "Art.h"), "w", newline="").write("\n".join(L) + "\n")
    # preview
    try:
        from PIL import Image
    except ImportError:
        return
    names = list(ART)
    S = 6
    img = Image.new("RGB", (len(names) * 18 * S, 18 * S), (27, 90, 60))
    for i, n in enumerate(names):
        for y, r in enumerate(ART[n]):
            for x, ch in enumerate(r):
                if ch == '.': continue
                c = PAL[ch][1]
                for dy in range(S):
                    for dx in range(S):
                        img.putpixel(((i * 18 + 1 + x) * S + dx, (1 + y) * S + dy), c)
    os.makedirs(os.path.join(root, "out"), exist_ok=True)
    img.save(os.path.join(root, "out", "art.png"))


if __name__ == "__main__":
    main()
