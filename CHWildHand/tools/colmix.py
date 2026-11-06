"""Writes tools/scripts/colmix.txt: mixed COLORS options (pause menu lines 7..12) at the table, in the shop, at a hex (boss) table and on the
title, a snap each; then makes the contact sheet from the snaps.
  python tools/colmix.py                 the script
  python tools/colmix.py sheet OUTDIR    OUTDIR/sheet.png from OUTDIR's snaps (after ./cl run ... colmix.txt OUTDIR --card ...)"""
import os, sys
import colours as C

HERE = os.path.dirname(os.path.abspath(__file__))
N = [len(o) for o in C.OPTS]          # THEME, FACE, RED, BLACK, HILITE, ACCENT
# FACE, RED, BLACK, HILITE, ACCENT
TABLE = [(0, 0, 0, 0, 0), (1, 1, 2, 2, 1), (2, 0, 0, 1, 1), (3, 2, 1, 3, 2), (4, 3, 2, 4, 3), (5, 4, 4, 0, 1),
         (6, 5, 3, 1, 2), (7, 1, 5, 2, 0), (8, 2, 0, 0, 0), (9, 3, 1, 1, 3), (10, 1, 4, 3, 0), (0, 4, 2, 2, 1), (1, 5, 3, 4, 2),
         (11, 0, 0, 0, 0), (12, 0, 1, 1, 2), (2, 0, 0, 0, 0), (5, 0, 1, 0, 0)]
SHOP = [(2, 0, 0, 1, 1), (8, 2, 0, 0, 0), (1, 1, 2, 2, 1)]
BOSS = [(6, 5, 3, 1, 2), (3, 2, 1, 3, 2)]
TITLE = (7, 1, 5, 2, 0)


def name(c):
    return '-'.join(C.OPTS[1 + k][v][0] for k, v in enumerate(c))


def script():
    out = ['say Z 99', 'wait 30', 'tap A', 'wait 12', 'tap A', 'wait 120']
    cur = [0] * 5

    def setc(c, menu=True):
        out.extend(['tap START', 'wait 4'] + ['tap DOWN'] * 8)               # (line 8: FACE)
        for k in range(5):
            out.extend(['tap RIGHT'] * ((c[k] - cur[k]) % N[1 + k]))
            cur[k] = c[k]
            out.append('tap DOWN')
        out.extend(['tap START', 'wait 6'])

    for i, c in enumerate(TABLE):
        setc(c)
        out.extend(['tap A', 'tap RIGHT', 'tap RIGHT', 'wait 8', 'snap t%02d' % i, 'tap LEFT', 'tap LEFT', 'tap A', 'wait 2'])
    for i, c in enumerate(SHOP):
        setc(c)
        out.extend(['say X', 'wait 40', 'snap s%02d' % i])
        if i < len(SHOP) - 1:
            out.extend(['say R 1'])
    for i, c in enumerate(BOSS):
        out.extend(['say R %d' % (1 + i), 'say Y %d' % (2 + i), 'say X', 'wait 10'] + ['tap RIGHT'] * 5 + ['wait 2', 'tap A', 'wait 120'])
        setc(c)
        out.extend(['tap A', 'wait 30', 'tap A', 'tap RIGHT', 'wait 8', 'snap b%02d' % i])
    setc(TITLE)
    out.extend(['tap START', 'wait 4'] + ['tap DOWN'] * 16 + ['tap A', 'wait 2', 'tap A', 'wait 60', 'snap title'])
    # the pause menu itself, at the top and scrolled to the COLORS lines
    out.extend(['tap A', 'wait 12', 'tap A', 'wait 120', 'tap START', 'wait 4', 'snap menu0'] + ['tap DOWN'] * 10 + ['wait 2', 'snap menu1'])
    # the THEME presets, one on each time (back at the table)
    out.extend(['tap START', 'wait 6'])
    for p in range(1, len(C.PRESETS)):
        out.extend(['tap START', 'wait 4'] + ['tap DOWN'] * 7 + ['tap RIGHT', 'tap START', 'wait 6', 'tap A', 'tap RIGHT', 'wait 8', 'snap p%02d' % p, 'tap LEFT', 'tap A', 'wait 2'])
    open(os.path.join(HERE, 'scripts', 'colmix.txt'), 'w', newline='').write('\n'.join(out) + '\n')
    return out


def sheet(d):
    from PIL import Image, ImageDraw
    shots = [('t%02d' % i, name(c)) for i, c in enumerate(TABLE)] + [('s%02d' % i, 'SHOP ' + name(c)) for i, c in enumerate(SHOP)] + \
            [('b%02d' % i, 'HEX ' + name(c)) for i, c in enumerate(BOSS)] + [('title', 'TITLE ' + name(TITLE)), ('menu0', 'MENU'), ('menu1', 'MENU, COLORS')] + \
            [('p%02d' % p, 'THEME ' + C.PRESETS[p][0]) for p in range(1, len(C.PRESETS))]
    cols, S = 5, 256
    rows = (len(shots) + cols - 1) // cols
    im = Image.new('RGB', (cols * (S + 8) + 8, rows * (S + 22) + 8), (36, 36, 36))
    dr = ImageDraw.Draw(im)
    for i, (f, label) in enumerate(shots):
        p = os.path.join(d, f + '.png')
        if not os.path.exists(p):
            continue
        x, y = 8 + (i % cols) * (S + 8), 8 + (i // cols) * (S + 22)
        im.paste(Image.open(p).convert('RGB').resize((S, S), 0), (x, y))
        dr.text((x, y + S + 4), label, fill=(230, 230, 230))
    im.save(os.path.join(d, 'sheet.png'))


if __name__ == '__main__':
    if len(sys.argv) > 2 and sys.argv[1] == 'sheet':
        sheet(sys.argv[2])
    else:
        print(len(script()), 'lines')
