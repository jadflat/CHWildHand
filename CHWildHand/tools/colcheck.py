"""Checks the card colour options (tools/colours.py): every FACE x RED and FACE x BLACK pair's WCAG contrast, the menu bar,
how far apart the faces are (CIELAB, also as the colour blind see them), and the red/black suits in the colour-blind presets.
  python tools/colcheck.py              the report (exit 1 on a failure)
  python tools/colcheck.py cvd OUT.png  the presets' suits and the faces as seen with protanopia, deuteranopia, tritanopia (Machado 2009)"""
import sys, itertools
import colours as C

MACHADO = {'protan': [[0.152286, 1.052583, -0.204868], [0.114503, 0.786281, 0.099216], [-0.003882, -0.048116, 1.051998]],
           'deutan': [[0.367322, 0.860646, -0.227968], [0.280085, 0.672501, 0.047413], [-0.011820, 0.042940, 0.968881]],
           'tritan': [[1.255528, -0.076749, -0.178779], [-0.078411, 0.930809, 0.147602], [0.004733, 0.691367, 0.303900]]}


def unlin(v):
    v = min(1.0, max(0.0, v))
    return round(255 * (v * 12.92 if v <= 0.0031308 else 1.055 * v ** (1 / 2.4) - 0.055))


def cvd(rgb, kind):
    l = [C.lin(v) for v in rgb]
    m = MACHADO[kind]
    return tuple(unlin(sum(m[i][j] * l[j] for j in range(3))) for i in range(3))


def lab(rgb):
    r, g, b = (C.lin(v) for v in rgb)
    x, y, z = (0.4124 * r + 0.3576 * g + 0.1805 * b) / 0.95047, 0.2126 * r + 0.7152 * g + 0.0722 * b, (0.0193 * r + 0.1192 * g + 0.9505 * b) / 1.08883
    f = lambda t: t ** (1 / 3) if t > 0.008856 else 7.787 * t + 16 / 116
    return 116 * f(y) - 16, 500 * (f(x) - f(y)), 200 * (f(y) - f(z))


def de(a, b):
    return sum((p - q) ** 2 for p, q in zip(lab(a), lab(b))) ** 0.5


def suits(face, r, b):
    """the face colour, the red suit's and the black suit's, as the game shows them"""
    _, k, fc = C.FACES[face]
    red, blk, _ = C.variants()
    return fc, red[r][k], blk[b][k]


def report():
    red, blk, hil = C.variants()
    bad = 0
    lo = {'red': 99, 'black': 99, 'bar': 99}
    for f, (fn, k, fc) in enumerate(C.FACES):
        for r in range(len(C.REDS)):
            lo['red'] = min(lo['red'], C.contrast(red[r][k], fc))
        for b in range(len(C.BLACKS)):
            lo['black'] = min(lo['black'], C.contrast(blk[b][k], fc))
            ink = blk[b][0] if k == 0 else fc
            for h in range(len(C.HILITES)):
                lo['bar'] = min(lo['bar'], C.contrast(hil[h][k][0], ink))
    print('lowest contrast over every combination: red suit %.2f, black suit %.2f, menu bar %.2f' % (lo['red'], lo['black'], lo['bar']))
    bad += lo['red'] < 4.5 or lo['black'] < 4.5 or lo['bar'] < 4.5
    print('red suits (light / near black / dark faces):')
    for (n, _), v in zip(C.REDS, red):
        print('  %-7s %s' % (n, '  '.join('%3d,%3d,%3d' % c for c in v)))
    print('black suits:')
    for (n, _, _), v in zip(C.BLACKS, blk):
        print('  %-7s %s' % (n, '  '.join('%3d,%3d,%3d' % c for c in v)))
    print('highlights (gold per class):')
    for (n, _, _), v in zip(C.HILITES, hil):
        print('  %-7s %s' % (n, '  '.join('%3d,%3d,%3d' % c[0] for c in v)))
    # the faces: the closest pair, normal and colour blind
    for kind in (None, 'protan', 'deutan', 'tritan'):
        m = min(((de(*(c if not kind else cvd(c, kind) for c in (a[2], b[2]))), a[0], b[0]) for a, b in itertools.combinations(C.FACES, 2)))
        print('closest faces %-7s dE %.1f (%s, %s)' % (kind or 'normal', *m))
        bad += kind is None and m[0] < 10
    # the presets: red vs black suits as each kind of colour blindness sees them
    print('presets: red suit vs black suit, dE / dL (normal, protan, deutan, tritan)')
    for p in C.PRESETS:
        fc, rc, bc = suits(p[1], p[2], p[3])
        row = []
        for kind in (None, 'protan', 'deutan', 'tritan'):
            a, b = (rc, bc) if not kind else (cvd(rc, kind), cvd(bc, kind))
            row.append('%5.1f/%4.1f' % (de(a, b), abs(lab(a)[0] - lab(b)[0])))
        print('  %-8s %s' % (p[0], '  '.join(row)))
    return bad


def sheet(path):
    from PIL import Image, ImageDraw
    kinds = [None, 'protan', 'deutan', 'tritan']
    cw, ch = 60, 40
    rows = len(C.PRESETS) + 2
    im = Image.new('RGB', (90 + cw * 4, 20 + ch * rows), (40, 40, 40))
    d = ImageDraw.Draw(im)
    for j, k in enumerate(kinds):
        d.text((90 + j * cw + 4, 4), k or 'normal', fill=(230, 230, 230))
    for i, p in enumerate(C.PRESETS):
        fc, rc, bc = suits(p[1], p[2], p[3])
        y = 20 + i * ch
        d.text((4, y + 14), p[0], fill=(230, 230, 230))
        for j, k in enumerate(kinds):
            t = (lambda c: c) if not k else (lambda c, k=k: cvd(c, k))
            x = 90 + j * cw
            d.rectangle([x + 2, y + 2, x + cw - 3, y + ch - 3], fill=t(fc))
            d.text((x + 8, y + 8), 'A', fill=t(rc)); d.rectangle([x + 8, y + 22, x + 22, y + 32], fill=t(rc))
            d.text((x + 34, y + 8), 'K', fill=t(bc)); d.rectangle([x + 34, y + 22, x + 48, y + 32], fill=t(bc))
    y = 20 + len(C.PRESETS) * ch
    d.text((4, y + 14), 'FACES', fill=(230, 230, 230))
    for j, k in enumerate(kinds):
        for f, (_, _, fc) in enumerate(C.FACES):
            c = fc if not k else cvd(fc, k)
            x = 90 + j * cw + 2 + (f % 6) * 9
            d.rectangle([x, y + 2 + f // 6 * 18, x + 7, y + 17 + f // 6 * 18], fill=c)
    im = im.resize((im.width * 2, im.height * 2), 0)
    im.save(path)


if __name__ == '__main__':
    if len(sys.argv) > 2 and sys.argv[1] == 'cvd':
        sheet(sys.argv[2])
    else:
        sys.exit(1 if report() else 0)
