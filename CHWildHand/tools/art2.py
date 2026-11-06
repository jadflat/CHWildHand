"""Wild Hand art, part 2 (the dealers and wild cards 8-39): ASCII grids -> Art2.h + out/art2.png preview.

python tools/art2.py   (from CHWildHand/CHWildHand)
Same run-length format and palette characters as tools/art.py (drawArt reads both). '.' is transparent.
Dealer heads are 24x24 (tools/dealers24.py); their eyes are 2x2 or 4x4 blocks the game draws over (pupils, blinks), see Dealers.h.
"""
import os

PAL = {
    'f': (1, (30, 54, 34)), 'w': (2, (226, 216, 192)), 'r': (3, (168, 26, 34)), 'k': (4, (18, 12, 22)), 'g': (5, (214, 146, 50)),
    'e': (6, (120, 124, 132)), 'd': (7, (14, 26, 18)), 'W': (8, (234, 228, 210)), 'b': (9, (98, 66, 146)), 's': (10, (178, 150, 150)),
    'y': (11, (255, 212, 128)), 'l': (12, (44, 72, 46)), 'h': (13, (168, 158, 138)), 'G': (14, (106, 62, 24)), 'R': (0, (72, 14, 26)),   # 0 is drawn as WINE
}
ART = {}


def grid(name, rows, n=16):
    assert len(rows) == n, f"{name}: {len(rows)} rows"
    for i, r in enumerate(rows):
        assert len(r) == n, f"{name} row {i} is {len(r)} wide"
        for ch in r:
            assert ch == '.' or ch in PAL, f"{name}: bad char {ch!r}"
    ART[name] = rows


# Horror theme (2026-11): the same colour roles as tools/art.py (k ink, w bone, W pale, r blood, R wine, g candle,
# G tarnished bronze, y flame, e grave grey, h bone shadow, s pallid flesh, l sickly green, d moss, b night purple).

def M(rows):
    """Mirror half rows (left half given) into full rows."""
    return [r + r[::-1] for r in rows]


def P(rows, pts):
    """Patch single pixels: pts is a list of (x, y, char)."""
    g = [list(r) for r in rows]
    for x, y, ch in pts:
        g[y][x] = ch
    return [''.join(r) for r in g]


def ring(fill, rim, r0, r1, out):
    """A disc or ring around (7.5, 7.5): fill inside r0..r1, ink 'out' px beyond, rim on the top-left edge."""
    rows = []
    for y in range(16):
        row = ''
        for x in range(16):
            d = ((x - 7.5) ** 2 + (y - 7.5) ** 2) ** 0.5
            if r0 <= d < r1:
                row += rim if (d > r1 - 1 and x + y < 13) else fill
            elif (r1 <= d < r1 + out) or (r0 > 0 and r0 - out <= d < r0):
                row += 'k'
            else:
                row += '.'
        rows.append(row)
    return rows


# ---------------------------------------------------------------- dealers: 24x24 heads, tools/dealers24.py
from dealers24 import HEADS
for _n, _r in HEADS.items():
    grid(_n, _r, 24)

# ---------------------------------------------------------------- wild cards 8-39
grid('W8', M([  # DIAMONDS: a blood gem with an eye inside
    ".......k",
    "......kr",
    ".....krr",
    "....krWr",
    "...krrkk",
    "..krrkWW",
    ".krrkWll",
    "krrkWllk",
    ".krrkWll",
    "..krrkWW",
    "...krrkk",
    "....kRrr",
    ".....kRr",
    "......kR",
    ".......k",
    "........",
]))
grid('W9', M([  # CLUBS: a nailed cudgel
    "......ek",
    "...e.kkk",
    "....kGGG",
    "...kGhGG",
    "eekGGGeG",
    "..kGGrGG",
    "..kGeGGG",
    "eekGGGGG",
    "...kGGGG",
    "....kGGG",
    ".....kkG",
    "......kG",
    "......kG",
    "......kh",
    "......kh",
    "......kk",
]))
grid('W10', M([  # LOW BALL: a tombstone sinking into the moss
    "........",
    "........",
    "........",
    "........",
    "....kkkk",
    "...keeee",
    "..keeeee",
    "..keeeek",
    "..keekkk",
    "..keeeek",
    "..keeeek",
    "..keeeee",
    "kdlkeeee",
    "kdddldld",
    "kkkkkkkk",
    "........",
]))
grid('W11', M([  # EVENS: rusted scales, a bone in each pan
    ".......k",
    "......kg",
    ".kkkkkkG",
    "kGGGGGGG",
    ".kkkkkkG",
    ".k....kG",
    ".k....kG",
    "k.k...kG",
    "kwhk..kG",
    "kGGk..kG",
    ".kk...kG",
    "......kG",
    "....kkGG",
    "...kGGGG",
    "...kkkkk",
    "........",
]))
grid('W12', [  # BIG FIVE: a severed hand, crawling on its fingertips
    "....kkkkkkk.....",
    ".kkkkwwkwwkkkk..",
    ".kwwkssksskwwk..",
    ".ksskssksskssk..",
    ".ksskssksskssk..",
    ".k" + "s" * 11 + "k..",
    ".kshsssssssssk..",
    ".k" + "s" * 11 + "kkk",
    "..k" + "s" * 9 + "swk.",
    "...k" + "s" * 8 + "kk..",
    "....kssssssk....",
    "....kkskkskk....",
    "....krrwwrrk....",
    ".....kRrrRk.....",
    "......krk.......",
    ".......k........",
])
grid('W13', [  # SOLO: one candle in the dark
    "................",
    ".......k........",
    "......kyk.......",
    ".....kygyk......",
    ".....kyWyk......",
    "......kgk.......",
    ".......k........",
    ".....kwhwwk.....",
    ".....kwhwwk.....",
    ".....kwwwhk.....",
    ".....kwwwhk.....",
    ".....kwwwhk.....",
    "...kkkkkkkkkk...",
    "..kGGGGGGGGGGk..",
    "...kkGGGGGGkk...",
    ".....kkkkkk.....",
])
grid('W14', P(M([  # LAST CALL: a cracked funeral bell, a bone clapper
    ".......k",
    "......kG",
    ".....kkk",
    "....kGGG",
    "...kGgGG",
    "...kGgGG",
    "...kGgGG",
    "..kGGgGG",
    "..kGGGGG",
    ".kGGGGGG",
    "kGGGGGGG",
    "kkkkkkkk",
    "......kw",
    "......kk",
    "........",
    "........",
]), [(10, 4, 'k'), (10, 5, 'k'), (9, 6, 'k'), (10, 7, 'k'), (11, 8, 'k'), (11, 9, 'k'), (12, 10, 'k')]))
grid('W15', [  # OPENER: a skeleton key with a skull for a bow
    "................",
    "................",
    "................",
    "................",
    ".kkkkk..........",
    "kwwwwwk.........",
    "kkkwkkkkkkkkkkk.",
    "kwwkwwkGGGGGGGGk",
    ".kwkwkkkkkkGkGkk",
    "..kkk.....kGkGk.",
    "..........kkkkk.",
    "................",
    "................",
    "................",
    "................",
    "................",
])
grid('W16', P(ring('R', 'G', 5.0, 6.3, 0.8), [  # RAINBOW: a summoning ring, a candle at each point
    (7, 0, 'y'), (8, 0, 'y'), (7, 1, 'g'), (8, 1, 'g'),
    (14, 7, 'l'), (14, 8, 'l'), (15, 7, 'k'), (15, 8, 'k'),
    (7, 14, 'r'), (8, 14, 'r'), (7, 15, 'k'), (8, 15, 'k'),
    (1, 7, 'b'), (1, 8, 'b'), (0, 7, 'k'), (0, 8, 'k'),
    (6, 6, 'k'), (7, 6, 'k'), (8, 6, 'k'), (9, 6, 'k'),
    (5, 7, 'k'), (6, 7, 'W'), (7, 7, 'r'), (8, 7, 'r'), (9, 7, 'W'), (10, 7, 'k'),
    (5, 8, 'k'), (6, 8, 'W'), (7, 8, 'r'), (8, 8, 'r'), (9, 8, 'W'), (10, 8, 'k'),
    (6, 9, 'k'), (7, 9, 'k'), (8, 9, 'k'), (9, 9, 'k'),
]))
grid('W17', P(M([  # THRIFT: a burial sack, tied, stitched shut
    "........",
    "....k.kk",
    "....khkh",
    ".....khh",
    "....kRRR",
    "...khhhh",
    "..khhhhh",
    ".khhhhhh",
    ".khhhhhh",
    "khhhhhhh",
    "khhhhhhh",
    "khhhhhhh",
    ".khhhhhh",
    "..kkkkkk",
    "........",
    "........",
]), [(6, 7, 'k'), (9, 7, 'k'), (7, 8, 'k'), (8, 9, 'k'), (6, 10, 'k'), (9, 10, 'k'), (7, 9, 'w'), (8, 8, 'w'),
     (12, 8, 'G'), (13, 9, 'G'), (13, 10, 'G'), (13, 11, 'G'), (12, 12, 'G'), (11, 12, 'G')]))
grid('W18', P(M([  # PIGGY: a coffin with a coin slot
    "........",
    ".....kkk",
    "....kGGG",
    "...kGGGk",
    "..kGGGGG",
    "..kGGGGg",
    "..kGGGGg",
    "..kGGggg",
    "...kGGGg",
    "...kGGGg",
    "...kGGGG",
    "....kGGG",
    "....kGGG",
    ".....kkk",
    "........",
    "........",
]), [(3, 5, 'h'), (3, 6, 'h'), (4, 8, 'h'), (4, 9, 'h'), (7, 0, 'k'), (8, 0, 'k'), (7, 1, 'g'), (8, 1, 'g')]))
grid('W19', M([  # VETERAN: a tarnished medal with a skull, on a mourning ribbon
    "...kkkkk",
    "...kbkRk",
    "...kbkRk",
    "....kbkR",
    "....kbkR",
    ".....kkk",
    "....kGGG",
    "...kGGkk",
    "..kGGkww",
    "..kGkwkw",
    "..kGkwwk",
    "..kGGkwk",
    "...kGGkk",
    "....kGGG",
    ".....kkk",
    "........",
]))
grid('W20', [  # ECHO: a raven, cawing
    "................",
    "................",
    ".....kkk........",
    "....kbbrk.......",
    ".kkkbbbbk.......",
    "keekkbbbbk......",
    ".kk.kbbbbbk.....",
    "....kbebbbbk....",
    "....kbbebbbbk...",
    ".....kbbebbbbk..",
    "......kbbbbbbbk.",
    ".......kkkbbbbbk",
    "........kek.kkkk",
    ".......kekek....",
    "..kGGGGGGGGGGk..",
    "...kkkkkkkkkk...",
])
DIE = ["kkkkkkk", "kwwwwwk", "kwrwwwk", "kwwwwwk", "kwwwrwk", "kwwwwwk", "khhhhhk", "kkkkkkk"]
grid('W21', [  # DBL DOWN: two bone dice with blood pips
    (DIE[y - 2] if 2 <= y < 10 else '.......') + '..' + (DIE[y - 7] if 7 <= y < 15 else '.......')
    for y in range(16)
])
grid('W22', P(ring('G', 'g', 0, 6.6, 0.9), [  # LUCKY 7: a bloody seven on a tarnished coin
    (5, 4, 'r'), (6, 4, 'r'), (7, 4, 'r'), (8, 4, 'r'), (9, 4, 'r'), (10, 4, 'r'),
    (10, 5, 'r'), (9, 6, 'r'), (9, 7, 'r'), (8, 8, 'r'), (8, 9, 'r'), (7, 10, 'r'), (7, 11, 'r'),
    (5, 5, 'R'), (5, 6, 'R'), (11, 5, 'k'), (10, 6, 'k'), (10, 7, 'k'), (9, 8, 'k'), (9, 9, 'k'), (8, 10, 'k'), (8, 11, 'k'),
]))
grid('W23', P(M([  # GOLEM: a cracked gargoyle, eyes lit green
    ".k......",
    ".kek..kk",
    "..keekee",
    "..keeeee",
    ".keeeeee",
    ".kekkkee",
    ".keklkee",
    ".keeeeek",
    ".keeeeek",
    ".keeeeee",
    ".kekwkwk",
    ".keekkkk",
    "..keeeee",
    "...kkkkk",
    "........",
    "........",
]), [(13, 4, 'h'), (13, 7, 'h'), (13, 8, 'h'), (13, 9, 'h'), (9, 2, 'k'), (10, 3, 'k'), (10, 4, 'k'),
     (4, 3, 'W')]))


# ---------------------------------------------------------------- wild cards 24-39 (the live-trigger cards)
grid('W24', M([  # LEECH: a blood drop with fangs
    ".......k", ".......k", "......kr", "......kr", ".....krr", ".....krW", "....krrr", "...krrrr",
    "..krWrrr", "..krWrrr", "..krrrkk", "..kRrkWk", "...kRRRR", "....kkkR", ".....kkk", "........",
]))
grid('W25', M([  # MOLD: a green toadstool that keeps growing
    "........", "....kkkk", "..kkllll", ".kllldll", "kllWllll", "klllllld", "kkkkkkkk", "......kw",
    "......kw", "......kw", "......kw", ".....kww", "...kkkkk", "..kddddd", "........", "........",
]))
grid('W26', M([  # TROPHY: a bronze cup of blood, for every hex beaten
    "........", "..kkkkkk", "..kyyggg", "..kgrrrr", "..kgrrrr", "...kgrrr", "....kggg", ".....kkg",
    "......kg", "......kg", ".....kgg", "....kggg", "...kGGGG", "...kkkkk", "........", "........",
]))
grid('W27', M([  # TOLL BELL: a funeral bell with a bone clapper
    ".......k", "......kk", ".....kGG", "....kGGG", "....kGyG", "...kGyGG", "...kGyGG", "..kGyGGG",
    "..kGyGGG", ".kGyGGGG", ".kGGGGGG", "kkkkkkkk", "......kw", ".......k", "........", "........",
]))
grid('W28', P(M([  # MAGGOT JAR: a stoppered jar, something writhing inside
    "....kkkk", "....kGGG", "...kkkkk", "..kWdddd", ".kWddddd", ".kWddddd", ".kWddddd", ".kWddddd",
    ".kWddddd", ".kWddddd", ".kWddddd", ".kWddddd", "..khhhhh", "...kkkkk", "........", "........",
]), [(5, 5, 'w'), (6, 6, 'w'), (9, 7, 'w'), (4, 8, 'w'), (7, 9, 'w'), (10, 10, 'w'), (5, 11, 'w'), (8, 4, 'w'), (6, 8, 'w')]))
grid('W29', M([  # WHISPER: a ghost
    "....kkkk", "...kWWWW", "..kWWWWW", "..kWkkWW", "..kWkkWW", "..kWWWWW", "..kWWWkk", "..kWWkWk",
    "..kWWWkk", "..kWWWWW", "..kWkWWk", "..kWkkWk", "..kkk.kk", "........", "........", "........",
]))
grid('W30', M([  # WICK: a stub of candle, a tall flame
    ".......y", ".......y", "......yy", ".....ygy", ".....ygg", "....yggr", "......kg", ".......k",
    "....kkWW", "....kWWW", "....kWWh", "....kWrh", "....kWWh", "....kWWh", "....kkkk", "........",
]))
grid('W31', P(M([  # ASH URN: a grey urn, ash rising
    "........", "........", "..kkkkkk", "..keWWWW", "...kkeee", "..keeeee", ".keeehee", ".keeeeee",
    ".keeeGGG", ".keeehee", ".keeeeee", "..keeeee", "...keeee", "...kkkkk", "..kkkkkk", "........",
]), [(7, 0, 'h'), (8, 0, 'e'), (6, 1, 'e'), (9, 1, 'h')]))
grid('W32', M([  # GHOUL LUCK: a green grin, yellow eyes
    "........", "....kkkk", "..kkllll", ".kllllll", "kllykkll", "klllllll", "kllllWll", "kllkkkkk",
    "kllkWkWk", "kllkkkkk", ".kllllll", "..kkllll", "....kkkk", "........", "........", "........",
]))
grid('W33', M([  # MOTHS: a night moth, eyes on its wings
    "..k.....", "...k..kk", ".kkk.kkb", "kbbkkkbb", "kbyykbbb", "kbykkbbb", "kbbbkkbb", ".kbbbkkk",
    "..kbbbkb", "..kkbbbb", "....kbbb", "....kbbk", ".....kk.", "........", "........", "........",
]))
grid('W34', M([  # WISHBONE: a bone to pull
    "kwk.....", "kwwk....", ".kwwk...", ".kwwk...", "..kwwk..", "..kwwk..", "...kwwk.", "...kwwwk",
    "....kwwh", ".....kww", "......kw", "......kw", ".....kww", ".....khh", "......kk", "........",
]))
grid('W35', M([  # BLOOD PACT: a contract, a red wax seal
    "..kkkkkk", ".kWWWWWW", ".kWWWWWW", ".kWhhhhh", ".kWWWWWW", ".kWhhhhh", ".kWWWWWW", ".kWhhhWW",
    ".kWWWWWW", ".kWWWkrr", ".kWWkrrr", ".kWWkrWr", ".kWWWkrr", ".kWWWWWW", "..kkkkkk", "........",
]))
grid('W36', [  # NOOSE: a hanging rope, a coiled knot, the loop (2026-11: twisted rope, lit left)
    ".......kk.......", "......kgGk......", "......kGgk......", "......kgGk......", ".....kkkkkk.....", ".....kgggGk.....",
    ".....kkkkkk.....", ".....kgggGk.....", "....kkkkkkkk....", "....kgk..kGk....", "...kgk....kGk...", "...kgk....kGk...",
    "...kgk....kGk...", "...kGgk..kGGk...", "....kGgkkgGk....", ".....kkkkkk.....",
])
grid('W37', [  # SHOVEL: a gravedigger's spade, grip up, blade lit top-left (2026-11)
    "....kkkkkkkk....", "....kgGGGGGk....", "....kkkgGkkk....", "......kgGk......", "......kgGk......", "......kgGk......",
    "......kgGk......", ".....kkgGkk.....", "....khhhheek....", "...khWhhheeek...", "...khhhheeeek...", "...khhheeeeek...",
    "...khheeeedek...", "....kheeeddk....", ".....kkeekk.....", "......kkkk......",
])
grid('W38', M([  # SEER BONE: a pale eye, purple iris
    "........", "........", "....kkkk", "..kkWWWW", ".kWWWbbb", "kWWbbbkk", "kWWbbkWk", "kWWbbkkk",
    ".kWWbbbb", "..kkWWWW", "....kkkk", "........", "........", "........", "........", "........",
]))
grid('W39', M([  # SAND CLOCK: an hourglass, a thread of sand
    "..kkkkkk", "..kGGGGG", "...kWWWW", "...kWyyy", "....kWyy", ".....kWy", "......ky", "......ky",
    ".....kyy", "....kWyy", "...kWyyy", "...kWyyy", "...kyyyy", "..kGGGGG", "..kkkkkk", "........",
]))


# ---------------------------------------------------------------- gore pass (2026-11, 1.1 "more blood"): patches over the pictures above.
# Never inside a dealer's eye sockets (Dealers.h redraws them). Pixels painted next to transparency get an ink edge.
def G(rows, pts):
    g = [list(r) for r in rows]
    H, W = len(g), len(g[0])
    done = []
    for x, y, ch in pts:
        if 0 <= x < W and 0 <= y < H:
            g[y][x] = ch
            done.append((x, y))
    for x, y in done:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            xx, yy = x + dx, y + dy
            if 0 <= xx < W and 0 <= yy < H and g[yy][xx] == '.':
                g[yy][xx] = 'k'
    return [''.join(r) for r in g]


def D(x, y, n, tip='R', ch='r'):
    """A drip: n px of blood down from (x, y), a dark tip."""
    return [(x, y + i, ch) for i in range(n - 1)] + [(x, y + n - 1, tip)]


def S(ch, *xy):
    return [(x, y, ch) for x, y in xy]


GORE = {
    'D0': D(8, 21, 3, 'r') + D(12, 21, 3) + D(15, 21, 2) + S('r', (9, 18), (13, 18), (10, 20), (14, 20))          # BEHOLDER: a bloody maw, drool
          + S('R', (1, 16), (4, 13), (2, 16), (3, 15)) + S('r', (2, 15), (3, 14)) + S('W', (2, 14))                # a gash in the hide
          + D(11, 15, 3) + S('r', (3, 4), (20, 3)),                                                               # a blood tear, bloodshot stalks
    'D1': D(2, 15, 5) + D(6, 15, 4) + D(17, 15, 6) + D(20, 15, 3) + S('r', (2, 13), (19, 13), (5, 14), (20, 14))  # MIMIC: gore down the chest
          + S('s', (4, 12), (5, 12), (6, 13)) + S('r', (3, 12)) + S('W', (4, 13))                                 # a chewed finger in the teeth
          + S('r', (3, 22), (4, 22), (19, 22), (20, 22), (12, 23), (11, 23)),
    'D2': D(4, 20, 2) + D(6, 20, 3) + D(9, 20, 4) + S('r', (4, 16), (5, 16), (8, 16), (12, 16), (7, 17), (10, 18))  # DRAGON: blood pours off the jaw
          + S('R', (9, 16)) + S('r', (15, 17), (16, 18), (17, 14), (19, 16)) + S('R', (14, 19), (18, 13))
          + S('W', (5, 14), (6, 14)) + S('h', (7, 15)),                                                           # a gnawed bone in the teeth
    'D3': S('r', (8, 2), (12, 2), (7, 3), (14, 3), (11, 2)) + D(6, 12, 2) + D(17, 12, 2)                            # FLAYER: a raw brain, blood tears
          + D(9, 16, 2) + D(12, 16, 2) + S('r', (8, 15), (10, 14), (3, 21), (9, 21), (14, 21), (18, 21), (2, 22), (8, 22), (19, 22)),
    'D4': [(x, y, 'R') for y in range(16, 22) for x in range(1, 7) if (x + y) % 2 == 0 and abs(x - 3.5) + abs(y - 18.5) < 4.5]   # CUBE: a blood cloud
          + S('r', (3, 18), (4, 19), (3, 19), (2, 17))
          + S('W', (17, 10), (18, 10), (17, 11)) + S('r', (18, 11)) + S('R', (18, 12), (17, 13))                  # a loose eyeball, its nerve
          + [(16, y, 'h') for y in range(18, 22)] + S('w', (13, 18), (14, 18), (15, 18), (13, 20), (14, 20), (15, 20)) # a ribcage
          + S('R', (14, 19), (15, 19), (14, 21)) + S('r', (8, 18), (10, 18), (13, 19)),
    'D5': D(8, 13, 3) + D(14, 13, 3) + D(10, 19, 3) + D(13, 19, 2) + S('r', (9, 7), (15, 7), (12, 18), (9, 17)),   # LICH: blood tears, a dripping jaw
    'D6': S('r', (8, 17), (10, 17), (12, 17), (8, 18), (10, 18), (12, 18), (8, 19), (9, 19), (10, 19), (11, 19), (12, 19),
            (13, 19), (9, 20), (10, 20), (11, 20), (12, 20), (9, 21), (10, 21), (11, 21))                         # DOCTOR: a bloody handprint
          + S('r', (12, 15), (14, 16), (16, 18), (19, 20), (20, 21), (6, 13), (7, 14), (11, 13)) + D(21, 22, 2, 'r'),  # a bloodied beak
    'D7': D(6, 12, 3) + S('r', (6, 19), (8, 19), (10, 19), (12, 19), (12, 20), (8, 20), (14, 19))                  # SCARECROW: a bleeding X, a seeping mouth
          + S('R', (10, 10), (3, 14), (11, 13), (17, 14), (16, 13)) + S('r', (11, 10)) + S('r', (5, 23), (13, 23)),
    'D8': D(6, 13, 5) + D(15, 13, 5) + S('r', (9, 15), (8, 17), (11, 18), (12, 18)) + S('R', (10, 18))              # DOLL: tears of blood, a bleeding crack
          + S('r', (4, 20), (17, 21), (19, 20), (11, 21), (10, 22)),
    'W9': S('r', (7, 4), (8, 6), (9, 3), (6, 7), (9, 9)) + S('R', (10, 8), (7, 10)),                                # CLUBS: a gory cudgel
    'W13': D(7, 7, 3) + D(9, 7, 2),                                                                                 # SOLO: the candle bleeds
    'W17': S('R', (4, 12), (6, 12), (10, 11), (5, 11)) + S('r', (5, 12), (9, 12)),                                  # THRIFT: the sack soaks through
    'W20': D(1, 7, 3, 'r') + S('R', (2, 9)),                                                                        # ECHO: the raven's beak drips meat
    'W26': D(3, 4, 4),                                                                                              # TROPHY: the cup spills
    'W29': D(4, 5, 2) + D(11, 5, 3) + D(7, 9, 2) + D(8, 9, 3),                                                       # WHISPER: a bleeding ghost
    'W32': D(6, 10, 2) + D(9, 10, 3) + S('r', (5, 8), (10, 8)),                                                     # GHOUL LUCK: bloody teeth
    'W34': S('r', (6, 8), (5, 7), (10, 7)) + S('R', (9, 8), (6, 9)) + D(7, 14, 2, 'r'),                             # WISHBONE: torn gristle
    'W37': S('r', (6, 11), (7, 12), (8, 12), (7, 13), (5, 10)) + S('R', (9, 11), (8, 13)) + D(7, 14, 2, 'r'),        # SHOVEL: a bloody blade
    'W38': S('r', (2, 5), (2, 7), (13, 6), (12, 8), (3, 8)) + S('R', (7, 11), (8, 14)) + S('r', (7, 12), (8, 13))    # SEER BONE: torn out, nerve and all
           + S('r', (5, 13)),
}
for _n, _p in GORE.items():
    ART[_n] = G(ART[_n], _p)


def rle(rows):
    out = [len(rows[0]), len(rows)]
    for r in rows:
        runs, i = [], 0
        while i < len(r):
            ch, n = r[i], 1
            while i + n < len(r) and r[i + n] == ch and n < 16:
                n += 1
            col = 15 if ch == '.' else PAL[ch][0]
            runs.append(((n - 1) << 4) | col)
            i += n
        while runs and (runs[-1] & 15) == 15:       # trailing transparency is free
            runs.pop()
        out.append(len(runs))
        out += runs
    return out


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    L = ["// Generated by tools/art2.py: do not edit by hand. The dealers' heads and wild cards 8-39.", "#pragma once", ""]
    total = 0
    for name, rows in ART.items():
        b = rle(rows)
        total += len(b)
        L.append(f"static const uint8_t ART_{name}[] = {{ " + ", ".join(map(str, b)) + " };")
    L.append("static const uint8_t *const DART[9] = { " + ", ".join(f"ART_D{i}" for i in range(9)) + " };")
    L.append("static const uint8_t *const JART2[32] = { " + ", ".join(f"ART_W{i}" for i in range(8, 40)) + " };")
    with open(os.path.join(here, "..", "Art2.h"), "w", newline="") as f:
        f.write("\n".join(L) + "\n")
    try:
        from PIL import Image
        names = list(ART)
        img = Image.new("RGB", (len(names) * 26 * 4, 26 * 4), (15, 58, 39))
        for k, name in enumerate(names):
            for y, r in enumerate(ART[name]):
                for x, ch in enumerate(r):
                    if ch != '.':
                        for dy in range(4):
                            for dx in range(4):
                                img.putpixel(((k * 26 + 1 + x) * 4 + dx, (1 + y) * 4 + dy), PAL[ch][1])
        os.makedirs(os.path.join(here, "..", "out"), exist_ok=True)
        img.save(os.path.join(here, "..", "out", "art2.png"))
        # one PNG a sprite at 1x for artkit.craft
        for name in names:
            s = Image.new("RGBA", (len(ART[name][0]), len(ART[name])), (0, 0, 0, 0))
            for y, r in enumerate(ART[name]):
                for x, ch in enumerate(r):
                    if ch != '.':
                        s.putpixel((x, y), PAL[ch][1] + (255,))
            s.save(os.path.join(here, "..", "out", f"art2_{name}.png"))
    except ImportError:
        pass
    print(f"Art2.h: {len(ART)} sprites, {total} bytes")


if __name__ == "__main__":
    main()
