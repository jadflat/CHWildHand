"""The stage assembler: bonus stages as small scripts (tools/stages/*.stg, in file-name order) for the game's interpreter (StageVM.h).

make_card.py calls build() and puts the result in WILDHAND.DAT: the section CARD_STAGE (a directory of 4 bytes a stage: u16 offset
in the section, u16 length; then the blobs, none crossing a 512 B block) and one more text block (STAGE_TXG): every stage's name,
then the first rule lines, the second rule lines, then the .str strings (the stages share a string's text).

A script:
    ; a comment (also after a line)
    .stage THE WATCHER          the name (the stage list, the bonus show)
    .rule HOLD A: CREEP         two rule lines
    .rule FREEZE WHEN IT LOOKS
    .score DOOR %d  EYES %d/3   the score line: the string $SCORE (TEXT -1, 26, $SCORE, TEXT, a, b draws it centred with two values)
    WIN, LOSE                   end the stage (a pause, then the prize or the penalty); `say I 100/101` does the same
    .reg POS r10                a register's name
    .const CX 64                a constant (constants and numbers may be combined: CX+3, -CX, 2*CX)
    .str HIT HIT!               a string for TEXT: $HIT (TEXT x, y, $s, colour, a, b: a format with up to two %d; x < 0 centres it)
    .init / .update / .draw     the three programs (init once when the stage is loaded, update each frame, draw each frame)
    label:  OP arg, arg ...     arguments: numbers, constants, registers (r0..r31 or a .reg name), labels, $strings
Jumps are relative to the next instruction, biased by JB (1 byte from 64 back to 158 forward, else 3 bytes: keep loops and exits close).
Registers: W r0 (wins), L r1 (losses), F r2 (frames), LV r3 (level: night / 3, at most 5), EZ r8 (debug cheat), FC r9 (frame counter);
the rest (r4..r7, r10..r31) are the script's, all 0 at the start.
Names: the colours (GOLD, RED ... as in Gfx.h without P_), the buttons (BTN_A, BTN_B, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT), the
sound effects (SFX_TADA ..., Sounds.h order).

Run alone: python tools/stage_asm.py [file.stg ...]   (a listing and the sizes)
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
# name, destination register first?, value arguments (StageVM.h VMOPS: the same order)
OPS = [('END', 0, 0), ('SET', 1, 1), ('ADD', 1, 2), ('SUB', 1, 2), ('MUL', 1, 2), ('DIV', 1, 2), ('MOD', 1, 2), ('AND', 1, 2),
       ('DRAW', 1, 0), ('SHR', 1, 2), ('MIN', 1, 2), ('MAX', 1, 2), ('RND', 1, 1), ('SIN', 1, 2), ('ABS', 1, 1), ('LD', 1, 1), ('ST', 0, 2),
       ('JMP', 0, 1), ('JEQ', 0, 3), ('JNE', 0, 3), ('JLT', 0, 3), ('JGE', 0, 3), ('DJNZ', 1, 1), ('IN', 1, 1), ('EDGE', 1, 1),
       ('SFX', 0, 2), ('FLASH', 0, 1), ('BURST', 0, 6), ('WIN', 0, 0), ('LOSE', 0, 0),
       ('FRECT', 0, 5), ('RECT', 0, 5), ('FLIP', 0, 5), ('FCIRC', 0, 4), ('CIRC', 0, 4), ('LINE', 0, 5), ('HLINE', 0, 4),
       ('TEXT', 0, 6), ('CARD', 0, 5), ('SUIT', 0, 5), ('HEAD', 0, 3), ('HAND', 0, 2), ('BJROW', 0, 4)]
OPN = {n: (i, d, k) for i, (n, d, k) in enumerate(OPS)}
JUMPS = ('JMP', 'JEQ', 'JNE', 'JLT', 'JGE', 'DJNZ')
COLOURS = ['BG', 'FELT', 'CARD', 'RED', 'INK', 'GOLD', 'GREY', 'DARK', 'TEXT', 'CHIP', 'SKIN', 'GLOW', 'FELTLT', 'SHADE', 'GOLDDK', 'WINE']
BASEREG = {'W': 0, 'L': 1, 'F': 2, 'LV': 3, 'EZ': 8, 'FC': 9}
BASECONST = dict({c: i for i, c in enumerate(COLOURS)}, BTN_A=1, BTN_B=2, BTN_UP=4, BTN_DOWN=8, BTN_LEFT=16, BTN_RIGHT=32)
JB = 64                                  # (StageVM.h VM_JB: the same)
HDR = 6
MAXLEN = 448                                  # (StageVM.h VM_CODE)


def sfx_names():
    src = open(os.path.join(HERE, '..', 'Sounds.h')).read()
    body = re.search(r'enum class Sfx : uint8_t \{(.*?)\};', src, re.S).group(1)
    return {'SFX_' + n.strip().upper(): i for i, n in enumerate(body.replace('\n', ' ').split(',')) if n.strip() and n.strip() != 'COUNT'}


class Stage:
    def __init__(self, path):
        self.path = path
        self.name, self.rules = None, []
        self.regs, self.consts, self.strs = dict(BASEREG), dict(BASECONST, **sfx_names()), []
        self.prog = {'init': [], 'update': [], 'draw': []}
        cur = None
        for ln, raw in enumerate(open(path), 1):
            line = raw.split(';')[0].strip()
            if not line:
                continue
            where = '%s:%d' % (os.path.basename(path), ln)
            if line.startswith('.'):
                key, _, rest = line.partition(' ')
                rest = rest.strip()
                if key in ('.init', '.update', '.draw'):
                    cur = key[1:]
                elif key == '.stage':
                    self.name = rest
                elif key == '.rule':
                    self.rules.append(rest)
                elif key == '.score':
                    self.strs.append(('SCORE', rest))
                elif key == '.reg':
                    n, r = rest.split()
                    self.regs[n] = self.reg(r, where)
                elif key == '.const':
                    n, v = rest.split(None, 1)
                    self.consts[n] = self.value(v, where)
                elif key == '.str':
                    n, _, t = rest.partition(' ')
                    self.strs.append((n, t))
                else:
                    raise SystemExit('%s: unknown %s' % (where, key))
                continue
            assert cur, '%s: an instruction before .init/.update/.draw' % where
            while ':' in line.split()[0]:
                lab, _, line = line.partition(':')
                self.prog[cur].append(('label', lab.strip(), where))
                line = line.strip()
                if not line:
                    break
            if line:
                op, _, args = line.partition(' ')
                op = op.upper()
                if op not in OPN:
                    raise SystemExit('%s: unknown op %s' % (where, op))
                a = [x.strip() for x in args.split(',')] if args.strip() else []
                if len(a) != OPN[op][1] + OPN[op][2]:
                    raise SystemExit('%s: %s takes %d arguments' % (where, op, OPN[op][1] + OPN[op][2]))
                self.prog[cur].append(('op', op, a, where))
        assert self.name and len(self.rules) == 2, '%s: .stage and two .rule lines' % path

    def isreg(self, x):
        return x in self.regs or re.fullmatch(r'[rR]\d+', x) is not None

    def reg(self, x, where):
        if x in self.regs:
            return self.regs[x]
        m = re.fullmatch(r'[rR](\d+)', x)
        if m and int(m.group(1)) < 32:
            return int(m.group(1))
        raise SystemExit('%s: not a register: %s' % (where, x))

    def value(self, x, where, labels=None):
        names = dict(self.consts, **(labels or {}))
        if not re.fullmatch(r'[\w\s+\-*/()]+', x):
            raise SystemExit('%s: bad value %s' % (where, x))
        try:
            v = eval(re.sub(r'[A-Za-z_]\w*', lambda m: str(names[m.group(0)]) if m.group(0) in names else m.group(0), x).replace('/', '//'), {'__builtins__': {}})
        except Exception:
            raise SystemExit('%s: bad value %s' % (where, x))
        return int(v)

    def operand(self, x, where, labels, sidx):
        if x.startswith('$'):
            if x[1:] not in sidx:
                raise SystemExit('%s: no string %s' % (where, x))
            v = sidx[x[1:]]
        elif self.isreg(x):
            return bytes([224 + self.reg(x, where)])
        else:
            v = self.value(x, where, labels)
        if 0 <= v <= 222:
            return bytes([v])
        assert -32768 <= v < 32768, '%s: %d is out of range' % (where, v)
        return bytes([223, v & 255, (v >> 8) & 255])

    def assemble(self, sidx):
        labels = {it[1]: 0 for sec in self.prog.values() for it in sec if it[0] == 'label'}
        size = None
        for _ in range(8):                                  # (a label's value can change an operand's size: until it settles)
            out, newl, segs = bytearray(), {}, []
            for sec in ('init', 'update', 'draw'):
                s0 = len(out)
                for it in self.prog[sec]:
                    if it[0] == 'label':
                        newl[it[1]] = HDR + len(out)
                        continue
                    _, op, args, where = it
                    i, d, k = OPN[op]
                    vals = args[d:]
                    same = bool(d and k and self.isreg(vals[0]) and self.reg(vals[0], where) == self.reg(args[0], where))
                    out.append(i | (128 if same else 0))
                    if d:
                        out.append(self.reg(args[0], where))
                    ops = vals[1:] if same else vals
                    for n, x in enumerate(ops):
                        if op in JUMPS and n == len(ops) - 1:           # (a jump is relative to the next instruction)
                            t = self.value(x, where, labels)
                            rel = t - (HDR + len(out) + 1) + JB
                            out += bytes([rel]) if 0 <= rel <= 222 else self.operand(str(t - (HDR + len(out) + 3) + JB), where, labels, sidx)
                        else:
                            out += self.operand(x, where, labels, sidx)
                segs.append(len(out) - s0)
            if newl == labels and size == len(out):
                break
            labels, size = newl, len(out)
        self.labels = labels
        upd = HDR + segs[0]
        drw = upd + segs[1]
        end = drw + segs[2]
        hdr = b''.join(bytes([x & 255, x >> 8]) for x in (upd, drw, end))
        blob = hdr + bytes(out)
        assert len(blob) <= MAXLEN, '%s: %d bytes (the most is %d)' % (self.path, len(blob), MAXLEN)
        return blob


def load(paths=None):
    if paths is None:
        d = os.path.join(HERE, 'stages')
        paths = [os.path.join(d, f) for f in sorted(os.listdir(d)) if f.endswith('.stg')]
    return [Stage(p) for p in paths]


def build(paths=None):
    """(the section's bytes, the text block's strings, the number of stages, the blobs)"""
    st = load(paths)
    n = len(st)
    texts = [s.name for s in st] + [s.rules[0] for s in st] + [s.rules[1] for s in st]
    blobs = []
    for s in st:                                            # (a string's text is stored once, whichever stages use it)
        sidx = {}
        for name, t in s.strs:
            if t not in texts[3 * n:]:
                texts.append(t)
            sidx[name] = texts.index(t, 3 * n)
        blobs.append(s.assemble(sidx))
    assert len(texts) < 256
    sec = bytearray(4 * n)
    for i, b in enumerate(blobs):
        if (len(sec) % 512) + len(b) > 512:
            sec += bytes(-len(sec) % 512)
        sec[4 * i:4 * i + 4] = bytes([len(sec) & 255, len(sec) >> 8, len(b) & 255, len(b) >> 8])
        sec += b
    return bytes(sec), texts, n, [(s.name, b) for s, b in zip(st, blobs)]


if __name__ == '__main__':
    sec, texts, n, blobs = build(sys.argv[1:] or None)
    for name, b in blobs:
        u, d, e = (b[k] | b[k + 1] << 8 for k in (0, 2, 4))
        print('%-12s %3d bytes (header %d, init %d, update %d, draw %d)' % (name, len(b), HDR, u - HDR, d - u, e - d))
    print('section %d bytes, %d strings (%d bytes of text)' % (len(sec), len(texts), sum(len(t) + 1 for t in texts)))
