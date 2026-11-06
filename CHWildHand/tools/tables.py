"""Tables the game keeps in RAM (loaded from the SD card when it is opened, Sounds.cpp wh_tabs) instead of in flash: the scenes' colour rings and
the dealers' looks. tools/make_card.py appends them to the effects' RAM block; their offsets are T_RING, T_DL in CardIndex.h."""

import colours
import songs
_ct, _first, _bgl = colours.strings()
COL_TXG = 9                    # the colours' text block (Gfx.h COL_TXG; card_text.GROUPS)
# the pause menu's lines (Bonus.h drawPauseMenu): the block and line of the text, the block and first line of the values (255: no value).
# 0 RESUME, 1-6 MUSIC, EFFECTS, CARD, INDEX, SONG, EQ, 7-12 the COLORS options THEME .. ACCENT, 13-14 BG, BG HOLD, 15 SAVE GAME, 16 QUIT TO TITLE
PM = ([1, 20, 255, 0], [4, 7, 4, 14], [4, 8, 4, 16], [4, 9, 3, 13], [4, 10, 3, 36], [4, 12, 255, 0], [4, 13, 3, 18]) + \
     tuple([COL_TXG, k, COL_TXG, _first[k]] for k in range(6)) +      tuple([COL_TXG, _bgl + k, COL_TXG, _first[6 + k]] for k in range(2)) + ([1, 21, 255, 0], [1, 22, 255, 0])
PM = [v for r in PM for v in r]
assert len(_ct) > 7 and len(PM) == 17 * 4

P_BG, P_FELT, P_CARD, P_RED, P_INK, P_GOLD, P_GREY, P_DARK, P_TEXT, P_CHIP, P_SKIN, P_GLOW, P_FELTLT, P_SHADE, P_GOLDDK, P_WINE = range(16)

# each area's felt drifts round its own ring of six colours (Scene.h scenePal): table, title, shop, boss, bonus, game over, survived
RING = [
    [(30, 54, 34), (20, 58, 54), (36, 66, 28), (56, 62, 26), (28, 62, 44), (24, 48, 62)],
    [(44, 24, 20), (58, 28, 18), (70, 34, 16), (52, 20, 22), (40, 18, 26), (64, 30, 20)],
    [(44, 28, 44), (60, 34, 40), (72, 42, 30), (52, 30, 34), (40, 26, 54), (62, 36, 46)],
    [(66, 6, 10), (48, 4, 22), (84, 10, 10), (40, 2, 8), (72, 6, 24), (58, 2, 6)],
    [(76, 16, 96), (26, 74, 30), (104, 12, 20), (48, 8, 64), (18, 56, 44), (86, 22, 30)],
    [(30, 32, 42), (26, 34, 50), (40, 40, 46), (30, 28, 44), (24, 36, 40), (36, 38, 50)],
    [(34, 50, 70), (40, 64, 86), (56, 76, 96), (36, 56, 84), (46, 70, 80), (60, 80, 100)],
]

# the nine dealers (Dealers.h struct Dealer): ex, ex2, ey, es, lid, pupil, body, trim, hand, eye
DL = [
    (255, 10, 11, 4, P_TEXT, P_INK, P_INK, P_WINE, P_SHADE, P_RED),
    (255, 10, 4, 4, P_GOLDDK, P_INK, P_SHADE, P_RED, P_SHADE, P_GLOW),
    (255, 11, 7, 2, P_GOLDDK, P_INK, P_DARK, P_GOLDDK, P_SHADE, P_GLOW),
    (5, 15, 7, 4, P_CHIP, P_TEXT, P_INK, P_WINE, P_SHADE, P_TEXT),
    (5, 11, 9, 4, P_INK, P_RED, P_DARK, P_GOLD, P_GREY, P_INK),
    (7, 13, 9, 4, P_CHIP, P_GLOW, P_CHIP, P_GREY, P_SHADE, P_CHIP),
    (5, 14, 8, 4, P_GOLDDK, P_INK, P_INK, P_GOLDDK, P_DARK, P_RED),          # PLAGUE DOCTOR: red goggle lenses
    (255, 14, 8, 4, P_SKIN, P_GOLD, P_DARK, P_GOLDDK, P_GOLDDK, P_INK),     # SCARECROW: one button eye (the X is drawn in)
    (5, 14, 9, 4, P_TEXT, P_INK, P_CHIP, P_WINE, P_CHIP, P_TEXT),           # PORCELAIN DOLL: glass eyes
]

# drifting things (Scene.h drifts): how many, two colours, speed (| 32 falls, | 64 blinks), width << 4 | height, the sway's divisor.
# Rows 0-6: each area's motes (table, title, shop, boss, bonus, game over, survived); 7-12: the BG styles MATRX, EMBER, RAIN, FLIES,
# CARDS, FOG (the pause menu's BG 2..7)
F, B = 32, 64
PART = [
    (12, P_GOLDDK, P_GOLD, 3, 0x11, 6),
    (18, P_GOLD, P_RED, 5, 0x11, 6),
    (10, P_GOLDDK, P_GOLD, 2, 0x11, 6),
    (14, P_RED, P_GLOW, 6, 0x11, 6),
    (0, 0, 0, 0, 0x11, 6),
    (20, P_DARK, P_GREY, F | 6, 0x12, 255),          # (game over: ash falling)
    (16, P_GLOW, P_TEXT, 3, 0x11, 6),
    (36, P_GREY, P_GLOW, F | 5, 0x16, 255),          # MATRX: falling code
    (30, P_RED, P_GOLD, 7, 0x11, 3),                 # EMBER: sparks rising, swaying
    (40, P_GREY, P_SHADE, F | 16, 0x15, 255),        # RAIN
    (16, P_GLOW, P_GOLD, B | 1, 0x22, 1),            # FLIES: slow, wandering, blinking
    (12, P_CARD, P_RED, F | 2, 0x45, 4),             # CARDS: little cards fluttering down
    (14, P_GREY, P_SHADE, 1, 0xF1, 2),               # FOG: long wisps rising slowly
]
BGORD = [0, 7, 8, 9, 10, 11, 12, 7]                  # AUTO: night n's PART row (n & 7; 0: the area's own motes)


# the glyph tables of Art.h that go to RAM as well (name, row length): make_card.py takes them out of ArtCard.h
FROM_ART = [('RANK8', 8), ('SUIT7', 7)]
# the game's own tables (Gfx.h ...), the same bytes as the arrays they replace: T_<name>
EXTRA = [
    ('PM', PM),
    ('BGORD', BGORD),   # Bonus.h drawPauseMenu (TX_ values: a static_assert there)
    ('HINTW', list(b'PICK\0PLAY\0TOSS\0')),
    ('BL', [20, 88, 60, 50, 10, 108, 50, 60, 0, 128, 20, 80]),
    ('RANKCH', [50, 51, 52, 53, 54, 55, 56, 57, 84, 74, 81, 75, 65, 0]),
    ('LVCH', [4, 6, 10, 12, 12, 12, 14, 18, 25]),
    ('HMULT', [1, 2, 2, 3, 4, 4, 4, 7, 8]),
    ('HCHIPS', [5, 10, 20, 30, 30, 35, 40, 60, 100]),
    ('HOTPAL', [58, 12, 16, 22, 4, 6, 82, 18, 22]),
    ('SCNBG', [8, 10, 8, 8, 2, 6, 8, 4, 10, 6, 0, 2, 10, 4, 16, 4, 4, 8, 4, 6, 12]),
    ('CARDDIM', [0, 7, 13, 15, 6, 14, 6, 7, 13, 6, 13, 5, 7, 6, 14, 15]),
    ('DIM', [0, 0, 7, 0, 0, 7, 0, 0, 7, 0, 7, 7, 0, 7, 0, 0]),
    ('SHIM', [0, 1, 8, 11, 4, 5, 6, 7, 8, 9, 10, 11, 12, 2, 14, 3]),
    ('RAMP2', [8, 8, 8, 8, 8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 13, 13, 13, 13, 13, 13]),
    ('RAMP', [8, 8, 8, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 15, 15, 15, 15, 15, 15]),
    ('SIN32', [0, 12, 24, 35, 45, 53, 59, 63, 64, 63, 59, 53, 45, 35, 24, 12, 0, -12, -24, -35, -45, -53, -59, -63, -64, -63, -59, -53, -45, -35, -24, -12]),
    ('THREMAP', [0, 1, 4, 3, 13, 5, 6, 7, 8, 9, 10, 11, 12, 6, 14, 15]),   # a themed face: the bone paper -> the face, the ink -> silver, the shade lines -> grey
    ('THDIM', [0, 7, 4, 3, 6, 14, 6, 7, 13, 6, 13, 5, 7, 6, 14, 15]),   # the same, dimmed (CARDDIM with a dark face)
    ('KSONG', songs.KSONG),   # the stage songs
    ('LTK', [2, 3, 2, 2, 5, 3, 3]), ('RSH', [6, 5, 6, 5, 5, 6, 5]),
    ('COMBO', [2, 8, 3, 9, 0, 1, 5, 7]),
    ('EP', [3, 70, 120, 96, 4, 112, 119, 60]),
    ('BWS', [24, 28, 34, 38, 42]), ('BHS', [34, 39, 46, 51, 56]),
    ('STYLE', [0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x45, 0x54, 0x31]),
    ('DEALS', [2, 0, 0, 1, 0, 0, 1, -3, 0, 1, 4, 6, 1, 8, 0, 1, 0, 8]),
    ('MG', [5, 3, 5, 3, 3, 3, 15, 3, 16, 5, 3, 2]),
    ('PXYS', [4, 26, 52, 76, 100, 104, 100, 106, 98, 103, 2, -1, 1, -2, 1]),
    ('FCOL', [9, 3, 14, 6, 6, 6, 5, 6]),
    ('LAB', [0, 0, 7, 7, 0, 0, 7, 7, 0, 0]),
    ('DR', [0, 0, 128, 16, 0, 120, 128, 8, 0, 16, 12, 104, 116, 16, 12, 104]),
    ('OPTM', [2, 3, 5, 5, len(songs.KSONG) + 1, 9] + [len(o) for o in colours.OPTS] + [len(o) for o in colours.BG_OPTS]),   # the pause menu's settings' counts, then the COLORS options'
    ('HANDP', [0x28, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE]),
    ('SUIT5', [0xD8, 0xF8, 0xF8, 0x70, 0x20, 0x20, 0x70, 0xF8, 0x70, 0x20, 0x70, 0x70, 0xA8, 0xF8, 0x20, 0x20, 0x70, 0xF8, 0xF8, 0x20]),
]


def table(artTabs=()):
    """(bytes, {name: offset}); artTabs: (name, bytes) from Art.h (FROM_ART)"""
    out = bytearray()
    offs = {}
    offs['T_RING'] = len(out)
    for ring in RING:
        for c in ring:
            out += bytes(c)
    offs['T_DL'] = len(out)
    for d in DL:
        assert len(d) == 10
        out += bytes(d)
    offs['T_PART'] = len(out)
    for m in PART:
        assert len(m) == 6
        out += bytes(m)
    for name, b in list(artTabs) + EXTRA:
        offs['T_' + name] = len(out)
        out += bytes(v & 255 for v in b)
    return bytes(out), offs
