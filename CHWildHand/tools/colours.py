"""The card colour options (pause menu > COLORS): their names and the colours, packed as one data block of WILDHAND.DAT
(tools/make_card.py; the game reads it in Gfx.h applyCol() only when an option changes). tools/colcheck.py checks them
(contrast, colour-blind views) and draws the contact sheets.

Each FACE has a class: 0 light (the bone paper: the face is the palette's P_CARD, the black suits P_INK), 1 near black,
2 dark (a themed face: THREMAP swaps the paper to P_INK and the ink to P_SHADE). The suit and highlight colours are kept
per class: the build lightens or darkens each choice's ideal colour until it reads on every face of the class
(WCAG contrast: suits 4.5, the black suits 7, the menu's highlight bar against its text 4.5). So any mix is readable."""

# the faces: name, class, colour
FACES = [('BONE', 0, (226, 216, 192)), ('WHITE', 0, (246, 246, 242)),
         ('BLACK', 1, (14, 14, 16)), ('GREY', 2, (46, 46, 50)), ('SLATE', 2, (26, 38, 66)), ('NAVY', 1, (12, 18, 78)),
         ('GREEN', 2, (12, 58, 22)), ('TEAL', 2, (4, 54, 60)), ('WINE', 2, (86, 10, 30)), ('PURPLE', 2, (58, 16, 90)),
         ('BROWN', 2, (70, 38, 12)), ('LT GREY', 0, (200, 200, 204)), ('PALE TEAL', 0, (178, 230, 218))]   # (new faces at the end: saved indices keep)
BONE_CARD = (226, 216, 192)          # the palette's P_CARD (light texts) when the face is dark
BONE_SHADE = (168, 158, 138)         # the palette's P_SHADE (shade lines) on a light face
# the red suits: name, ideal colour (class 1 keeps the game's P_RED exactly)
REDS = [('RED', (250, 52, 56)), ('ORANGE', (255, 128, 16)), ('PINK', (255, 96, 176)), ('GOLD', (240, 196, 24)),
        ('VIOLET', (176, 112, 255)), ('BLUE', (60, 140, 255))]
# the black suits: name, ideal on a light face (P_INK, also the outlines everywhere: kept dark), ideal on a dark face (P_SHADE)
BLACKS = [('MONO', (18, 12, 22), (132, 136, 148)), ('PURE', (0, 0, 0), (250, 250, 250)), ('BLUE', (14, 30, 110), (120, 180, 255)),
          ('GREEN', (6, 56, 22), (120, 230, 150)), ('BROWN', (64, 32, 8), (236, 196, 130)), ('CYAN', (0, 64, 72), (110, 230, 240))]
# the highlight (the picked card's gold frame and the menus' bars: P_GOLD) and its glow (the cursor's outline, glints: P_GLOW)
HILITES = [('GOLD', (214, 146, 50), (255, 212, 128)), ('SILVER', (176, 184, 198), (244, 248, 255)), ('CYAN', (40, 184, 210), (160, 244, 255)),
           ('LIME', (130, 204, 60), (214, 255, 150)), ('PINK', (232, 98, 178), (255, 186, 230))]
# the accents: the faces' edge rules and studs (P_GOLDDK) and the card backs (P_WINE)
ACCENTS = [('BRASS', (106, 62, 24), (72, 14, 26)), ('STEEL', (78, 92, 112), (22, 32, 70)), ('JADE', (36, 104, 70), (12, 52, 36)),
           ('VIOLET', (100, 60, 140), (50, 20, 76))]
# the THEME presets: name, then FACE, RED, BLACK, HILITE, ACCENT (indices). 0 is the default (all 0): THEME BONE resets the colours.
# Every preset has red and black suits (RED 0, BLACK MONO or PURE); RG SAFE's pure black on white keeps them apart for protan and
# deutan eyes too (colcheck.py: dE 35 / 54). At most eight presets: the THEME is saved in three bits (sv.music bits 5-7).
# RG SAFE: for red-green colour blindness (protan, deutan): red against pure black on white, light against dark.
# BY SAFE: for blue-yellow colour blindness (tritan): red against white on grey. HI-CON: red and white on black.
PRESETS = [('BONE', 0, 0, 0, 0, 0),
           ('NOIR', 2, 0, 0, 1, 1),
           ('NAVY', 5, 0, 0, 1, 1),
           ('FOREST', 6, 0, 0, 0, 2),
           ('WINE', 8, 0, 0, 0, 0),
           ('RG SAFE', 1, 0, 1, 2, 1),
           ('BY SAFE', 3, 0, 1, 1, 1),
           ('HI-CON', 2, 0, 1, 3, 0)]
LABELS = ['THEME  %s', 'FACE  %s', 'RED  %s', 'BLACK  %s', 'HILITE  %s', 'ACCENT  %s']   # the pause menu's lines 7..12
OPTS = [PRESETS, FACES, REDS, BLACKS, HILITES, ACCENTS]     # the order of the game's cop[] (and of the names in the text block)


def lin(c):
    c /= 255
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def lum(rgb):
    r, g, b = (lin(v) for v in rgb)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def contrast(a, b):
    la, lb = lum(a), lum(b)
    return (max(la, lb) + 0.05) / (min(la, lb) + 0.05)


def fit(ideal, against, need):
    """ideal, blended towards white or black (whichever is the farther from the backgrounds) until it has contrast >= need with all of them"""
    if all(contrast(ideal, b) >= need for b in against):
        return tuple(ideal)
    to = (255, 255, 255) if lum(ideal) >= max(lum(b) for b in against) else (0, 0, 0)
    for k in range(1, 101):
        c = tuple(round(v + (t - v) * k / 100) for v, t in zip(ideal, to))
        if all(contrast(c, b) >= need for b in against):
            return c
    raise SystemExit('no readable variant of %r' % (ideal,))


def faces(cls):
    return [c for _, k, c in FACES if k == cls]


def variants():
    """the per-class colours: red[r][cls], black[b][cls], hilite[h][cls] = (gold, glow)"""
    inks = [fit(d, faces(0), 7) for _, d, _ in BLACKS]                          # (the inks of the light faces: the menu bar's text there)
    red = [[fit(c, faces(k), 4.5) for k in range(3)] for _, c in REDS]
    blk = [[inks[i] if k == 0 else fit(l, faces(k), 4.5) for k in range(3)] for i, (_, d, l) in enumerate(BLACKS)]
    hil = [[(fit(g, inks if k == 0 else faces(k), 4.5), w) for k in range(3)] for _, g, w in HILITES]
    return red, blk, hil


def rgb565(c):
    """as the game's gfx_rgb(), little-endian"""
    v = (c[0] & 0xF8) << 8 | (c[1] & 0xFC) << 3 | c[2] >> 3
    return [v & 255, v >> 8]


P_CARD, P_RED, P_INK, P_GOLD, P_GLOW, P_SHADE, P_GOLDDK, P_WINE = 2, 3, 4, 5, 11, 13, 14, 15


def data():
    """(the data block, {name: offset}) for Gfx.h applyCol(): PRE[presets][6] (the options a THEME sets), FCLS[faces] (each face's class),
    SRC[3 classes][8 entries][4] (a palette entry: its index, then where its colour is: offset / 2, option k, stride s: the colour
    (RGB565, 2 bytes) at offset + cop[k] * s), then the colours"""
    red, blk, hil = variants()
    out, off = bytearray(), {}
    def put(name, b):
        off[name] = len(out)
        out.extend(b)
    put('C_PRE', [v for p in PRESETS for v in p[1:]])
    put('C_FCLS', [k * 32 for _, k, _ in FACES])           # (x 32: the offset of its recipes)
    put('C_SRC', bytes(3 * 8 * 4))
    out.extend(bytes(len(out) % 2))                       # (the colours at even offsets: SRC keeps offset / 2)
    put('FACE', [v for _, _, c in FACES for v in rgb565(c)])
    put('BONE', rgb565(BONE_CARD) + rgb565(BONE_SHADE))
    put('RED', [v for r in red for c in r for v in rgb565(c)])
    put('BLK', [v for b in blk for c in b for v in rgb565(c)])
    put('HILG', [v for h in hil for g, w in h for v in rgb565(g)])
    put('HILW', [v for _, _, w in HILITES for v in rgb565(w)])
    put('ACCD', [v for _, a, _ in ACCENTS for v in rgb565(a)])
    put('ACCW', [v for _, _, w in ACCENTS for v in rgb565(w)])
    src = []
    for c in range(3):                      # (option k 0 with stride 0: a constant)
        e = [(P_CARD, off['FACE'], 1, 2) if c == 0 else (P_CARD, off['BONE'], 0, 0),
             (P_INK, off['BLK'], 3, 6) if c == 0 else (P_INK, off['FACE'], 1, 2),
             (P_SHADE, off['BONE'] + 2, 0, 0) if c == 0 else (P_SHADE, off['BLK'] + 2 * c, 3, 6),
             (P_RED, off['RED'] + 2 * c, 2, 6), (P_GOLD, off['HILG'] + 2 * c, 4, 6), (P_GLOW, off['HILW'], 4, 2),
             (P_GOLDDK, off['ACCD'], 5, 2), (P_WINE, off['ACCW'], 5, 2)]
        assert all(o % 2 == 0 and o < 512 for _, o, _, _ in e)
        src += [v for i, o, k, st in e for v in (i, o // 2, k, st)]
    out[off['C_SRC']:off['C_SRC'] + len(src)] = bytes(src)
    assert len(out) <= 512 and len(PRESETS) <= 8 and all(len(o) <= 16 for o in OPTS)
    assert PRESETS[0][1:] == (0,) * 5 and all(p[1 + i] < len(o) for p in PRESETS for i, o in enumerate(OPTS[1:]))
    return bytes(out), {k: v for k, v in off.items() if k.startswith('C_')}


# the pause menu's BG lines (the backdrop's drifting things: Scene.h sceneFg) share this text block: their labels, then their values
BG_LABELS = ['BG  %s', 'BG HOLD  %s']
BG_OPTS = [['AUTO', 'SCENE', 'MATRX', 'EMBER', 'RAIN', 'FLIES', 'CARDS', 'FOG'], ['STAGE', 'ALWAYS']]


def strings():
    """the text block: LABELS, every option's names (in OPTS order), BG_LABELS, the BG options' names; the first name of each option
    (the six COLORS options, then BG and BG HOLD), the line of BG_LABELS[0]"""
    s, first = list(LABELS), []
    for o in OPTS:
        first.append(len(s))
        s += [e[0] for e in o]
    bgl = len(s)
    s += BG_LABELS
    for o in BG_OPTS:
        first.append(len(s))
        s += o
    return s, first, bgl
