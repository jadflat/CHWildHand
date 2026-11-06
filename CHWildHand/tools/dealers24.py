"""Wild Hand: the nine dealer heads at 24x24 (BEHOLDER, MIMIC, DRAGON, MIND FLAYER, CUBE, LICH, PLAGUE DOCTOR,
SCARECROW, PORCELAIN DOLL), drawn with a little
shading code plus hand patches. tools/art2.py imports HEADS (name -> 24 rows of palette characters, '.' clear).
Eyes are 4x4 sockets the game draws over (Dealers.h DL: ex, ex2, ey, es). python tools/dealers24.py writes a preview.
Palette characters as tools/art2.py PAL: k ink, w bone, W pale, r blood, R wine, g gold, G bronze, y flame, e grey,
h bone shadow, s flesh, l green, f felt, d moss, b purple.
"""
N = 24
BAYER = ((0, 2), (3, 1))


class C:
    def __init__(self):
        self.g = [['.'] * N for _ in range(N)]

    def px(self, x, y, ch):
        if 0 <= x < N and 0 <= y < N:
            self.g[y][x] = ch

    def pts(self, ch, *p):
        for x, y in p:
            self.px(x, y, ch)

    def rect(self, x, y, w, h, ch):
        for j in range(h):
            for i in range(w):
                self.px(x + i, y + j, ch)

    def hl(self, x, y, w, ch):
        self.rect(x, y, w, 1, ch)

    def vl(self, x, y, h, ch):
        self.rect(x, y, 1, h, ch)

    def line(self, x0, y0, x1, y1, ch):
        n = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(n + 1):
            self.px(round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n), ch)

    def ell(self, cx, cy, rx, ry, ramp, light=(-1, -1), dith=1.0):
        """A shaded ellipse; ramp runs light to dark, the light comes from the top left."""
        L = len(ramp)
        for y in range(N):
            for x in range(N):
                dx, dy = (x - cx) / rx, (y - cy) / ry
                if dx * dx + dy * dy > 1:
                    continue
                v = (dx * light[0] + dy * light[1]) / 2 / 1.0           # -0.7..0.7
                t = 0.5 - v * 0.62 + dith * (BAYER[y & 1][x & 1] - 1.5) / (5.0 * L)
                self.px(x, y, ramp[min(L - 1, max(0, int(t * L)))])

    def ramp_rect(self, x, y, w, h, ramp):
        L = len(ramp)
        for j in range(h):
            for i in range(w):
                t = (j / max(1, h - 1)) * 0.7 + (i / max(1, w - 1)) * 0.3 + (BAYER[(y + j) & 1][(x + i) & 1] - 1.5) / (5.0 * L)
                self.px(x + i, y + j, ramp[min(L - 1, max(0, int(t * L)))])

    def outline(self, ch='k'):
        o = [r[:] for r in self.g]
        for y in range(N):
            for x in range(N):
                if self.g[y][x] != '.':
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < N and 0 <= yy < N and self.g[yy][xx] not in '.':
                        o[y][x] = ch
        self.g = o

    def rows(self):
        return [''.join(r) for r in self.g]


def beholder():
    c = C()
    for (x0, y0, x1, y1) in ((6, 10, 3, 5), (9, 8, 8, 4), (15, 8, 15, 4), (18, 10, 20, 5)):     # four eyestalks
        c.line(x0, y0, x1, y1, 'b'); c.line(x0 + 1, y0, x1 + 1, y1, 'R')
        c.ell(x1 + .5, y1 - .5, 2.2, 2.2, 'WWh'); c.px(x1, y1 - 1, 'r'); c.px(x1 + 1, y1 - 1, 'k')
    c.ell(11.5, 14, 10.8, 8.8, 'ssbbbbbbbRR', dith=0)                                                            # the body
    c.hl(6, 21, 12, 'R')
    c.ell(11.5, 12.5, 7.1, 5.8, 'RRR')                                                               # the lid ring
    c.ell(11.5, 12.5, 6.1, 4.9, 'WWWwwh')                                                            # the eyeball
    c.pts('r', (6, 12), (7, 12), (8, 13), (17, 13), (16, 13), (15, 14))                                          # two veins
    c.rect(10, 11, 4, 4, 'r')                                                                         # (the iris, redrawn by the game)
    c.rect(11, 12, 2, 2, 'k')
    c.hl(6, 18, 12, 'k'); c.hl(5, 19, 14, 'k'); c.hl(6, 20, 12, 'k')                                  # the maw
    for x in range(6, 18, 2):
        c.px(x, 18, 'w'); c.px(x + 1, 20, 'w')
    c.hl(8, 19, 8, 'R'); c.hl(9, 19, 6, 'r')
    c.pts('w', (5, 19), (18, 19))
    c.outline()
    return c.rows()


def mimic():
    c = C()
    c.rect(2, 1, 20, 10, 'G'); c.rect(1, 3, 22, 8, 'G')                                               # the lid
    c.ramp_rect(2, 1, 20, 9, 'ggGGG')
    c.hl(3, 1, 18, 'y'); c.hl(2, 2, 20, 'g'); c.hl(2, 9, 20, 'G')
    c.ramp_rect(1, 15, 22, 8, 'GGGGG')                                                                # the base
    c.hl(1, 15, 22, 'g')
    for x in (8, 15):                                                                                 # plank gaps
        c.vl(x, 2, 8, 'k'); c.vl(x, 16, 7, 'k'); c.vl(x - 1, 3, 3, 'g'); c.vl(x - 1, 17, 3, 'g')
    for x in (3, 19):                                                                                 # iron straps
        c.rect(x, 1, 3, 9, 'e'); c.vl(x, 1, 9, 'h'); c.vl(x + 2, 1, 9, 'k')
        c.rect(x, 15, 3, 8, 'e'); c.vl(x, 15, 8, 'h'); c.vl(x + 2, 15, 8, 'k')
        c.pts('W', (x + 1, 3), (x + 1, 7), (x + 1, 17), (x + 1, 20))
    c.rect(1, 10, 22, 5, 'R'); c.hl(1, 10, 22, 'k'); c.hl(1, 14, 22, 'k')                             # the open mouth, wine inside
    c.rect(2, 11, 20, 1, 'k')
    for x in range(2, 22, 3):                                                                          # teeth, top and bottom
        c.pts('w', (x, 10), (x + 1, 10), (x, 11), (x + 1, 11), (x, 12), (x + 1, 13), (x - 1, 14), (x, 14), (x, 13))
    for x in range(3, 21, 3):
        c.pts('W', (x, 10))
    c.rect(9, 13, 6, 3, 'r'); c.pts('R', (9, 14), (14, 14))                                           # the tongue, long and wet
    c.vl(10, 16, 6, 'r'); c.vl(11, 16, 8, 'r'); c.vl(12, 16, 8, 'r'); c.vl(13, 16, 6, 'r'); c.pts('R', (12, 17), (11, 19), (12, 21), (13, 20))
    c.pts('W', (11, 15), (12, 14))
    c.rect(8, 2, 8, 8, 'k')                                                                            # the eye in the lid, ringed
    c.rect(9, 3, 6, 6, 'G'); c.rect(10, 4, 4, 4, 'y')
    c.hl(9, 3, 6, 'g'); c.hl(9, 8, 6, 'k')
    c.pts('R', (4, 21), (5, 22), (17, 19), (20, 21))
    c.outline()
    return c.rows()


def dragon():
    c = C()
    c.line(16, 6, 22, 0, 'w'); c.line(17, 6, 22, 1, 'h'); c.line(14, 5, 18, 1, 'w'); c.line(15, 5, 18, 2, 'h')      # two horns
    c.ell(16.5, 16.5, 7, 6.5, 'gGGGR')                                                                              # the neck
    for y in range(11, 23):
        for x in range(11, 24):
            if c.g[y][x] in 'GgR' and ((x + (y & 1) * 2) % 4 == 0):
                c.px(x, y, 'g' if c.g[y][x] != 'R' else 'G')
    c.ell(13.5, 9.5, 8.5, 5.8, 'yggggGG')                                                                           # the skull
    c.ramp_rect(2, 8, 11, 5, 'yggGGG'); c.px(2, 8, '.'); c.px(2, 12, '.'); c.px(1, 9, '.'); c.px(1, 11, '.')        # the snout
    c.hl(3, 8, 8, 'y')
    c.rect(2, 13, 12, 3, 'k'); c.rect(3, 14, 10, 2, 'R')                                                                                       # the open mouth
    c.hl(2, 13, 12, 'k'); c.hl(3, 15, 8, 'r')
    c.ramp_rect(3, 16, 11, 4, 'gGGGR'); c.px(3, 19, '.'); c.px(4, 19, '.')                                          # the lower jaw
    c.hl(4, 16, 10, 'G'); c.hl(5, 19, 9, 'R')
    c.pts('w', (3, 14), (3, 13), (7, 14), (10, 14), (10, 13), (13, 14))                                              # fangs above
    c.pts('W', (3, 13), (10, 13)); c.pts('w', (4, 15), (8, 15), (12, 15), (4, 14), (12, 14))
    c.pts('r', (6, 15), (7, 15), (9, 15), (4, 17), (5, 17), (11, 17))                                                # blood and tongue
    c.pts('k', (3, 9), (3, 10))                                                                                     # the nostril
    c.pts('e', (1, 7), (2, 6), (0, 5), (1, 4))                                                                      # smoke
    c.hl(9, 6, 5, 'k'); c.hl(10, 5, 4, 'G'); c.pts('G', (8, 7))                                                     # the brow
    c.rect(11, 7, 2, 2, 'y')                                                                                        # (the eye, redrawn by the game)
    c.pts('w', (19, 10), (20, 11), (22, 15), (23, 17), (21, 19), (22, 21))                                          # spines down the back
    c.pts('h', (20, 10), (21, 11), (23, 16))
    c.outline()
    return c.rows()


def flayer():
    import math
    c = C()
    for k, (bx, ph, amp) in enumerate(((3, 0.0, 1.0), (8, 1.6, 1.0), (13, 3.2, 1.0), (18, 4.8, 1.0))):             # tentacles
        for y in range(14, 23):
            x = bx + round(amp * (1 if k & 1 else -1) * math.sin((y - 12) * 0.75 + ph))
            w = 4 if y < 18 else 3 if y < 21 else 2
            c.hl(x, y, w, 'b'); c.px(x + w - 1, y, 'R')
            if (y - 14) % 3 == 1:
                c.px(x, y, 's')
        c.px(bx, 22, 'R')
    c.ell(11.5, 8.5, 9.5, 8, 'ssbbbbbbbRR', dith=0)                                                                            # the swollen skull
    c.pts('R', (5, 3), (6, 2), (7, 2), (8, 1), (9, 1), (10, 1), (13, 1), (14, 1), (15, 2), (16, 2), (17, 3), (10, 3), (11, 3), (12, 3), (13, 3),
          (8, 4), (9, 4), (14, 4), (15, 4))                                                                          # brain folds
    c.pts('s', (7, 3), (8, 2), (12, 2), (14, 3), (6, 4))
    c.rect(3, 6, 7, 6, 'R'); c.rect(14, 6, 7, 6, 'R')                                                               # sockets' shadow
    c.rect(4, 7, 5, 4, 'k'); c.rect(15, 7, 5, 4, 'k')
    c.rect(5, 7, 4, 4, 'W'); c.rect(15, 7, 4, 4, 'W')                                                               # (blank eyes, redrawn)
    c.hl(3, 5, 7, 'k'); c.hl(14, 5, 7, 'k'); c.pts('k', (2, 6), (21, 6))                                            # heavy brows
    c.pts('k', (11, 11), (12, 11), (11, 12), (12, 12))
    c.hl(7, 14, 10, 'k'); c.hl(8, 15, 8, 'R'); c.pts('w', (8, 14), (10, 14), (13, 14), (15, 14))
    c.pts('R', (4, 12), (5, 13), (19, 12), (18, 13), (6, 14), (17, 14))
    c.outline()
    return c.rows()


def cube():
    c = C()
    for y in range(1, 6):                                                                                           # the top
        for x in range(1 + (5 - y), 20 + (5 - y)):
            c.px(x, y, 'l' if y > 2 else 'e')
    for y in range(6, 23):                                                                                          # the front
        for x in range(1, 20):
            t = (y - 6) / 16
            c.px(x, y, 'l' if t + (BAYER[y & 1][x & 1] - 1.5) / 9 < 0.55 else 'f')
    for x in range(19, 24):                                                                                         # the side
        top, bot = round(6 - (x - 19) * 1.25), round(22 - (x - 19) * 1.25)
        for y in range(top, bot + 1):
            c.px(x, y, 'd' if x > 21 else 'f')
    c.hl(2, 6, 18, 'W'); c.vl(1, 7, 6, 'W'); c.line(1, 5, 5, 1, 'W'); c.hl(5, 1, 18, 'h')
    c.hl(1, 22, 19, 'd'); c.vl(19, 6, 17, 'e')
    c.ell(9.5, 12.5, 6.2, 6, 'WWwwhh')                                                                              # the skull
    c.rect(5, 9, 4, 4, 'k'); c.rect(11, 9, 4, 4, 'k')                                                               # (sockets, redrawn)
    c.pts('k', (9, 14), (10, 14), (9, 15)); c.hl(6, 17, 8, 'k')
    for x in range(7, 14, 2):
        c.px(x, 17, 'W')
    c.pts('h', (7, 18), (9, 18), (11, 18), (13, 18))
    c.line(12, 20, 17, 15, 'h'); c.pts('W', (17, 15), (18, 14), (17, 14), (12, 20), (11, 21))                       # a bone
    c.pts('g', (3, 19), (4, 19), (3, 20), (15, 8)); c.pts('y', (3, 19))
    c.pts('h', (16, 9), (3, 10), (17, 20), (2, 14), (16, 12))                                                       # bubbles
    c.pts('r', (6, 21), (7, 21), (4, 7))
    c.outline()
    return c.rows()


def lich():
    c = C()
    c.ell(11.5, 13.5, 10.5, 10, 'bbRRRRk')                                                                          # the hood
    for y in range(8, 23):
        c.px(2 + (y // 5) % 2, y, 'k'); c.px(20 - (y // 5) % 2, y, 'k')
    c.ell(11.5, 13, 7.0, 8.2, 'kkk')                                                                                # the void under it
    c.ell(11.5, 10.5, 6.2, 6.4, 'WWwwhh')                                                                           # the skull
    c.rect(8, 15, 8, 4, 'w'); c.hl(8, 18, 8, 'h'); c.pts('k', (8, 18), (15, 18))                                  # the jaw
    c.hl(8, 17, 8, 'k')
    for x in range(8, 16, 2):
        c.px(x, 16, 'W'); c.px(x, 17, 'W'); c.px(x + 1, 18, 'W')
    c.rect(6, 8, 6, 5, 'k'); c.rect(12, 8, 6, 5, 'k')
    c.rect(7, 9, 4, 4, 'b'); c.rect(13, 9, 4, 4, 'b')                                                               # (sockets, redrawn)
    c.pts('k', (11, 13), (12, 13), (11, 14), (12, 14), (12, 12), (11, 12))
    c.line(9, 5, 10, 8, 'k'); c.pts('k', (10, 6), (11, 7)); c.line(16, 13, 15, 16, 'k'); c.pts('k', (16, 14), (14, 17))   # cracks
    c.rect(5, 4, 14, 3, 'G'); c.hl(5, 4, 14, 'y'); c.hl(5, 5, 14, 'g'); c.hl(5, 6, 14, 'G')                         # the crown
    for x, h in ((5, 3), (8, 4), (11, 5), (14, 4), (17, 3)):
        c.vl(x, 4 - h, h, 'g'); c.vl(x + 1, 4 - h, h, 'G'); c.px(x, 4 - h, 'y')
    c.pts('r', (11, 5), (12, 5), (11, 1)); c.pts('b', (8, 5), (15, 5)); c.pts('y', (11, 4))
    c.pts('b', (3, 4), (2, 3), (21, 4), (20, 2), (3, 6))                                                            # cold fire licks
    c.pts('s', (19, 3), (3, 2))
    c.outline()
    return c.rows()


def doctor():
    """Drawn by hand: wide black hat with a grey brim, bone mask, gold-rimmed goggles (lenses at 5,8 and 14,8: redrawn), the beak, the wine cloak."""
    return [
        ".....kkkkkkkkkkkkkk.....",
        ".....keeddddddddddk.....",
        ".....kedddddddddddk.....",
        ".....kwWwwwwwwwwhhk.....",
        "..keeeeeeeeeeeeeeeeeek..",
        "kedddddddddddddddddddddk",
        ".kkkWWWWWWwwwwwwwwwwkkk.",
        ".krkgggggGWwwgggggGwkRk.",
        ".krkgrrrrGGGGgrrrrGhkRk.",
        ".krkgrrrrGwwwgrrrrGhkRk.",
        ".krkgrrrrGWwwgrrrrGhkRk.",
        ".kRkgrrrrGwwwgrrrrGhkRk.",
        ".kRkGGGGGGwwwGGGGGGhkRk.",
        ".kRkhwwwwkWWwwhkwwwhkRk.",
        ".kRRkhhhhhkWwwwhkhhkRRk.",
        ".krrRRRRRRRkWwwwhkRRRRk.",
        ".krRRRRRRRRRkWwwhhkRRRk.",
        ".krRRRRkRRRRRkWwwhhkRRk.",
        ".kRRRRRkrRRRRRRkWwhhkRk.",
        ".kRRRRkrRRRRRRRRkWwhhk..",
        ".kRRRRkrRRRRRRRRRRkWhhk.",
        ".kRRRRkRRRRRRRRRRRRkwhk.",
        ".kRRRRkRRRRRRRRRRRRRkkk.",
        ".kkkkkkkkkkkkkkkkkkkkk..",
    ]


def scarecrow():
    """Drawn by hand: wine crown and bronze brim, burlap sack lit from the top left, stitched X eye, button socket at 14,8 (redrawn), zigzag mouth, cord, straw."""
    return [
        ".......kkkkkkkk.........",
        "......krrRRRRRRk........",
        "......krRRRRRRRRk.......",
        "......krRRRRkRRRk.......",
        ".....kggGGGGGGGGGk......",
        "..kkgggggggggggggggGGkk.",
        ".kGGGGGGGGGGGGGGGGGGGGGk",
        "kGkkGGGGGGGGGGGGGGGGkkGk",
        "kk.kygggkggggGGGGGGgk.kk",
        "..kykgggkgggggGkkkkGgk..",
        "..kygkgkggggggGkkkkGgk..",
        "..kyggkgggggggGkkkkGgk..",
        "..kggkgkggggggGkkkkGGk..",
        "..kgkgggkggggggGGGGgGk..",
        "..kggggggggggggggggGGk..",
        "..kggRgRgRgRgRgRgggGGk..",
        "..kggkgkgkgkgkgkggGGGk..",
        "..kgggkgkgkgkgkgkGGGGk..",
        "...kggRgRgRgRgRGGGGGk...",
        "....kGGGGGGGGGGGGGGk....",
        "...kRrrRRRRRRRRRRRRRk...",
        "...kygyygygyygygygGGk...",
        "...kyGkygkyGkygkyGGk....",
        "...kyk.kyk.kyk.kyk......",
    ]


def doll():
    c = C()
    for x0 in (0, 18):                                                                                              # gold ringlets
        for k in range(4):
            c.ell(x0 + 2.5 + (k & 1), 6 + k * 3.6, 2.3, 2.0, 'yggG')
    c.ell(11.5, 5.5, 10, 5.5, 'ygggG')
    c.ell(11.5, 11.5, 8.2, 8.0, 'WWWWwh')                                                                           # the round pale face
    c.hl(5, 4, 14, 'g'); c.pts('y', (7, 3), (12, 3), (16, 4))                                                       # the fringe
    for (x, y) in ((4, 14), (6, 14), (5, 15), (17, 14), (19, 14), (18, 15), (16, 15)):                             # dithered red cheeks
        c.px(x, y, 'r')
    c.rect(4, 8, 6, 6, 'h'); c.rect(13, 8, 6, 6, 'h')                                                               # the glass eyes' rims
    c.hl(4, 8, 6, 'k'); c.hl(13, 8, 6, 'k'); c.pts('k', (4, 7), (9, 7), (13, 7), (18, 7))                           # lashes
    c.rect(5, 9, 4, 4, 'W'); c.rect(14, 9, 4, 4, 'W')                                                               # (the eyes, redrawn by the game)
    c.pts('r', (10, 16), (13, 16), (11, 17), (12, 17)); c.pts('R', (11, 16), (12, 16))                             # a tiny bow mouth
    c.line(9, 3, 8, 7, 'k'); c.pts('k', (9, 13), (8, 14), (8, 15), (7, 16), (7, 17), (8, 18))                       # the crack, crown to jaw
    c.pts('r', (8, 19)); c.pts('R', (7, 19))                                                                        # a bead of blood
    c.pts('k', (3, 15), (3, 16), (4, 17)); c.pts('h', (4, 16))                                                      # a chip in the cheek
    for x in range(2, 22):                                                                                          # the scalloped lace collar
        c.px(x, 20, 'W' if x % 3 else 'h'); c.px(x, 21, 'w' if x % 3 != 1 else '.')
        if x % 3 == 2:
            c.px(x, 22, 'h')
    c.rect(9, 19, 6, 3, 'R'); c.pts('r', (10, 20), (13, 20)); c.pts('k', (11, 20), (12, 20))                       # the wine bow
    c.pts('R', (11, 22), (12, 23))
    c.outline()
    return c.rows()


HEADS = {'D0': beholder(), 'D1': mimic(), 'D2': dragon(), 'D3': flayer(), 'D4': cube(), 'D5': lich(),
         'D6': doctor(), 'D7': scarecrow(), 'D8': doll()}

if __name__ == '__main__':
    import sys
    from PIL import Image
    IDX = dict(f=1, w=2, r=3, k=4, g=5, e=6, d=7, W=8, b=9, s=10, y=11, l=12, h=13, G=14, R=15)
    PALR = [(8, 10, 8), (30, 54, 34), (226, 216, 192), (168, 26, 34), (18, 12, 22), (214, 146, 50), (120, 124, 132), (14, 26, 18),
            (234, 228, 210), (98, 66, 146), (178, 150, 150), (255, 212, 128), (44, 72, 46), (168, 158, 138), (106, 62, 24), (72, 14, 26)]
    S = 8
    img = Image.new('RGB', (len(HEADS) * (N + 2) * S, (N + 2) * S), PALR[1])
    for k in range(len(HEADS)):
        for y, r in enumerate(HEADS['D%d' % k]):
            for x, ch in enumerate(r):
                if ch != '.':
                    for dy in range(S):
                        for dx in range(S):
                            img.putpixel(((k * (N + 2) + 1 + x) * S + dx, (1 + y) * S + dy), PALR[IDX[ch]])
    img.save(sys.argv[1] if len(sys.argv) > 1 else 'heads.png')
