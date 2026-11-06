"""CHWildHand's music: looping chiptune arrangements (melody + bass; each with a second section, a
bridge or breakdown and a lifted last section), as step
sequences for the FM synth (Synth.cpp; the CHix pattern).

  The original songs: ambient pads, dungeon synth and upbeat jazz (the stage rotation, the title, the shop, the end).
  tools/songs.py says which plays where.

A song is 512 to 1024 steps (sixteenths, a multiple of 128: the game streams it from the SD card 64 steps at a
time); a phrase shorter than that repeats to fill it. Streams of one byte a step:
  lead, bass: 0 = rest (released), 1 = hold, 2..127 = a MIDI note (a new note);
  drums: bit mask, 1 kick, 2 snare, 4 closed hat, 8 open hat.
Notes are written "E5:4" (name, octave, length in sixteenths), "R:4" a rest.

Writes CHWildHand/Music.h. Run: python tools/make_music.py
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import songs

RATE = 8000
NAMES = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7,
         'G#': 8, 'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11}


def notes(text):
    out = []
    for tok in text.split():
        n, ln = tok.split(':')
        ln = int(ln)
        if n == 'R':
            out += [0] * ln
        else:
            m = 12 * (int(n[-1]) + 1) + NAMES[n[:-1]]
            out += [m] + [1] * (ln - 1)
    return out


def fill(seq, steps):
    assert steps % len(seq) == 0, len(seq)
    return (seq * (steps // len(seq)))[:steps]


def drums(pat):                     # one bar of 16 characters: k kick, s snare, h hat, o open hat, b kick+hat, . none
    m = {'.': 0, 'k': 1, 's': 2, 'h': 4, 'o': 8, 'b': 5, 'x': 6}
    return [m[c] for c in pat]


def bars(*bs):
    return notes(" ".join(bs))


SONGS = []

# ---- the calm pass (v1.1): the music was busy and noisy. Every older song is thinned when it is added: one voice of
# tune and one of bass (the echo voice is gone), no repeated-note stutter, no one-step ornaments, a bass that moves
# at most twice a bar, a soft kick once a bar (no snare or hats), shrill notes folded down, FM tones purer, and the
# tempo 12% slower.
CALM_SLOW = 1.12
LEAD_TOP = 86                                   # D6, about 1.2 kHz: nothing shriller


def to_notes(seq):                              # step stream -> [[start, pitch, length]], gaps are rests
    out = []
    for i, b in enumerate(seq):
        if b > 1:
            out.append([i, b, 1])
        elif b == 1 and out and out[-1][0] + out[-1][2] == i:
            out[-1][2] += 1
    return out


def from_notes(ns, n):
    seq = [0] * n
    for st, p, ln in ns:
        seq[st] = p
        for k in range(1, ln):
            seq[st + k] = 1
    return seq


def calm_lead(seq):
    ns = [n for n in to_notes(seq) if n[2] > 1]     # drop one-step ornaments
    out = []
    for n in ns:
        while n[1] > LEAD_TOP:
            n[1] -= 12
        if out and out[-1][1] == n[1] and out[-1][0] + out[-1][2] >= n[0]:      # a repeat that touches: one long note
            out[-1][2] = n[0] + n[2] - out[-1][0]
        else:
            out.append(n)
    return from_notes(out, len(seq))


def calm_bass(seq):
    kept = [n for n in to_notes(seq) if n[0] % 8 == 0]      # a bass note only on the half bar, held to the next one
    out = []
    for n in kept:
        if not (out and out[-1][1] == n[1] and n[0] % 16):   # the same root again is held, and struck again each bar
            out.append(n)
    for k, n in enumerate(out):
        end = out[k + 1][0] if k + 1 < len(out) else len(seq)
        n[2] = min(end - n[0], 16)
    return from_notes(out, len(seq))


def calm_drums(seq):                            # a soft kick on the first beat of each bar (the picture still pulses)
    return [1 if (b & 1) and i % 16 == 0 else 0 for i, b in enumerate(seq)]


def calm_inst(inst):                            # purer FM: a lower modulation index, a gentler bass; no echo voice
    (r0, i0, d0, f0, a0, vd0, vf0), (r1, i1, d1, f1, a1, vd1, vf1), e = inst
    return [(1, min(i0, 18), 1, min(f0, 4), min(a0, 80), 1, min(vf0, 44)),
            (1, min(i1, 10), 1, min(f1, 3), min(a1, 82), 1, min(vf1, 52)), (2, 20, 2, 3, 0, 2, 0)]


def add(name, ms, inst, lead, bass, dr, delay=3, steps=512, calm=True):
    assert steps % 64 == 0 and steps <= 1024 and delay < 16
    lead, bass, dr = fill(lead, steps), fill(bass, steps), fill(dr, steps)
    if calm:                                    # the older, busier arrangements are thinned (see calm_* below)
        lead, bass, dr = calm_lead(lead), calm_bass(bass), calm_drums(dr)
        inst = calm_inst(inst)
        ms = int(ms * CALM_SLOW + 0.5)
    SONGS.append(dict(name=name, ms=ms, inst=inst, lead=lead, bass=bass, drums=dr, delay=delay, steps=steps))


def up(seq, k):                     # transposed by k semitones (rests and holds stay)
    return [n + k if n > 1 else n for n in seq]


def sec(seq, bars_):                # a section: checks its length in bars
    assert len(seq) == 16 * bars_, (len(seq), bars_)
    return seq


# ---- optional extra songs come first when tools/songs_private.py exists (not in the repository); a public build skips it
if not songs.PUBLIC:
    exec(compile(open(songs.PRIVATE).read(), songs.PRIVATE, 'exec'), globals())

# ---- 10, 11 ambient: slow pads, no drums (original)
# 1024 steps each (64 bars, about 4.5 and 5 minutes), four 16-bar sections that drift: the home progression,
# a relative-minor turn, a modal shift, and home again with a quiet close into the loop.
AMB1_LEAD = notes("E5:8 G5:8 D5:8 B4:8 C5:8 E5:8 D5:12 R:4")
AMB1_BASS = notes("C3:16 G2:16 A2:16 F2:16")
AMB1 = [                                                                   # C major: C G Am F
    (AMB1_LEAD, AMB1_BASS), (notes("G5:8 E5:8 D5:8 B4:8 C5:8 A4:8 C5:12 R:4"), AMB1_BASS),
    (AMB1_LEAD, AMB1_BASS), (notes("E5:16 D5:16 C5:16 A4:12 R:4"), AMB1_BASS),
    # Am Em F C, then Dm G Em Am
    (notes("A4:8 C5:8 B4:8 G4:8 A4:8 C5:8 G4:12 R:4"), notes("A2:16 E2:16 F2:16 C3:16")),
    (notes("E5:8 C5:8 B4:8 E5:8 F5:8 E5:8 C5:12 R:4"), notes("A2:16 E2:16 F2:16 C3:16")),
    (notes("F5:8 D5:8 D5:8 B4:8 B4:8 G4:8 A4:12 R:4"), notes("D3:16 G2:16 E2:16 A2:16")),
    (notes("D5:8 F5:8 G5:8 D5:8 E5:16 R:16"), notes("D3:16 G2:16 E2:16 G2:16")),
    # C mixolydian: C Bb F C, the flat seventh
    (notes("E5:8 G5:8 F5:8 D5:8 C5:8 A4:8 G4:12 R:4"), notes("C3:16 Bb2:16 F2:16 C3:16")),
    (notes("Bb4:8 D5:8 F5:8 D5:8 C5:16 R:16"), notes("C3:16 Bb2:16 F2:16 C3:16")),
    (notes("G5:16 F5:8 D5:8 Bb4:16 C5:12 R:4"), notes("C3:16 Bb2:16 F2:16 C3:16")),
    (notes("E5:8 D5:8 C5:8 Bb4:8 A4:16 G4:12 R:4"), notes("C3:16 Bb2:16 F2:16 G2:16")),
    # home, the tune an octave up once, then a close that settles on G into the loop
    (AMB1_LEAD, AMB1_BASS), (up(AMB1_LEAD, 12), AMB1_BASS),
    (notes("E5:8 G5:8 D5:8 B4:8 C5:8 A4:8 F4:12 R:4"), AMB1_BASS),
    (notes("E5:16 D5:16 C5:16 R:16"), notes("C3:16 G2:16 A2:16 G2:16")),
]
add('AMBER', 260, [(1, 12, 1, 3, 72, 0, 60), (1, 8, 1, 2, 70, 0, 60), (2, 20, 2, 3, 0, 2, 0)],
    sum((sec(l, 4) for l, _ in AMB1), []), sum((sec(b, 4) for _, b in AMB1), []), [0] * 16, delay=6, steps=1024, calm=False)
AMB2_LEAD = notes("A4:6 C5:2 E5:8 D5:8 B4:4 G4:4 E5:6 D5:2 C5:8 A4:16")
AMB2_BASS = notes("A2:16 F2:16 C3:16 E2:16")
AMB2 = [                                                                   # A minor: Am F C E
    (AMB2_LEAD, AMB2_BASS), (notes("E5:6 D5:2 C5:8 B4:8 G4:8 C5:6 B4:2 A4:8 E4:16"), AMB2_BASS),
    (AMB2_LEAD, AMB2_BASS), (notes("A4:16 C5:8 B4:8 G4:16 A4:12 R:4"), AMB2_BASS),
    # A dorian (the raised sixth): Am D Am D, then Em G D Am
    (notes("A4:8 B4:8 C5:8 F#5:8 E5:16 D5:12 R:4"), notes("A2:16 D3:16 A2:16 D3:16")),
    (notes("E5:8 F#5:8 G5:8 E5:8 D5:8 B4:8 A4:12 R:4"), notes("A2:16 D3:16 A2:16 D3:16")),
    (notes("B4:8 E5:8 D5:8 B4:8 F#4:8 A4:8 A4:12 R:4"), notes("E2:16 G2:16 D3:16 A2:16")),
    (notes("G4:8 B4:8 D5:8 F#5:8 E5:16 R:16"), notes("E2:16 G2:16 D3:16 E2:16")),
    # darker: Am Bb (phrygian), then F G E E
    (notes("E5:8 F5:8 E5:8 C5:8 D5:8 C5:8 Bb4:12 R:4"), notes("A2:16 Bb2:16 A2:16 Bb2:16")),
    (notes("A4:8 C5:8 E5:16 D5:8 C5:8 B4:12 R:4"), notes("F2:16 G2:16 E2:16 E2:16")),
    (notes("E5:8 F5:8 A5:8 F5:8 D5:8 Bb4:8 A4:12 R:4"), notes("A2:16 Bb2:16 A2:16 Bb2:16")),
    (notes("C5:8 A4:8 D5:8 B4:8 G#4:16 E4:12 R:4"), notes("F2:16 G2:16 E2:16 E2:16")),
    # home: the tune an octave up, then down, then a close on E into the loop
    (up(AMB2_LEAD, 12), AMB2_BASS), (AMB2_LEAD, AMB2_BASS),
    (notes("E5:6 D5:2 C5:8 B4:8 G4:8 C5:6 B4:2 A4:8 E4:16"), AMB2_BASS),
    (notes("A4:16 E4:16 A4:16 R:16"), notes("A2:16 F2:16 E2:16 E2:16")),
]
add('AMBNIGHT', 300, [(1, 10, 1, 2, 72, 0, 60), (1, 8, 1, 2, 70, 0, 60), (2, 20, 2, 3, 0, 2, 0)],
    sum((sec(l, 4) for l, _ in AMB2), []), sum((sec(b, 4) for _, b in AMB2), []), [0] * 16, delay=8, steps=1024, calm=False)

# horror pass (v1.3.4): the dungeon songs are faster and harsher, with a heartbeat under them (kick = lub-dub, snare = the thud)
HEART = [1, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0]
HEART2 = [1, 0, 1, 0, 4, 0, 4, 0, 2, 0, 1, 0, 4, 0, 4, 8]

# ---- 12..15 dungeon-synth, original (v1.1): slow minor modes, a bell or a soft flute, a drone that moves every two
# bars, no drums. 80 to 92 seconds a loop; each phrase is four bars. They join the stage songs.
def phrases(*ps):
    return sum((sec(p, 4) for p in ps), [])


# 12 CRYPT: D minor with a flat second, a tolling bell (FM ratio 3) over a low D drone. 384 steps at 240 ms = 92 s.
CR = phrases(bars("A5:12 R:4", "F5:8 G5:4 F5:4", "E5:12 R:4", "D5:16"),
             bars("A5:8 Bb5:4 A5:4", "G5:8 F5:8", "E5:4 F5:4 G5:8", "A5:16"),
             bars("D6:12 R:4", "C6:8 Bb5:4 A5:4", "Bb5:12 R:4", "A5:16"),
             bars("R:16", "F5:12 R:4", "E5:8 D5:8", "Eb5:16"),
             bars("D5:8 F5:8", "A5:12 G5:4", "F5:8 E5:8", "D5:16"),
             bars("A4:12 R:4", "D5:12 R:4", "C5:8 Bb4:4 A4:4", "D5:12 R:4"))
CRB = phrases(bars("D3:32", "Bb2:32"), bars("D3:32", "C3:16 A2:16"), bars("Bb2:32", "F3:32"),
              bars("Eb3:32", "D3:32"), bars("D3:32", "G2:32"), bars("D3:48", "A2:16"))
add('CRYPT', 170, [(3, 36, 2, 6, 80, 1, 40), (2, 18, 1, 4, 84, 0, 54), (2, 20, 2, 3, 0, 2, 0)], CR, CRB, HEART, steps=384, calm=False)

# 13 CATACOMB: A minor, a soft flute (a pure sine) with long notes over a slow bass. 384 steps at 220 ms = 84 s.
CT = phrases(bars("E5:8 C5:4 D5:4", "E5:16", "A4:8 B4:4 C5:4", "B4:16"),
             bars("C5:8 D5:8", "E5:12 R:4", "G5:8 F#5:8", "E5:16"),
             bars("A5:12 G5:4", "E5:8 D5:8", "C5:8 D5:4 E5:4", "D5:16"),
             bars("R:8 E5:4 G5:4", "A5:16", "G5:8 E5:8", "D5:12 R:4"),
             bars("C5:8 B4:8", "A4:12 R:4", "B4:8 C5:4 D5:4", "E5:16"),
             bars("A4:16", "C5:8 B4:8", "A4:16", "R:16"))
CTB = phrases(bars("A3:32", "F3:32"), bars("A3:32", "G3:32"), bars("F3:32", "C3:32"),
              bars("D3:32", "E3:32"), bars("A3:32", "F3:32"), bars("A3:48", "E3:16"))
add('CATACOMB', 155, [(1, 22, 1, 5, 80, 0, 58), (2, 16, 1, 4, 84, 0, 54), (2, 20, 2, 3, 0, 2, 0)], CT, CTB, HEART2, steps=384, calm=False)

# 14 ASHEN: E phrygian, a glassy bell (ratio 2) that rings out, the flat second leaning on the drone. 320 steps at 250 ms = 80 s.
AS = phrases(bars("B5:8 R:8", "C6:8 B5:8", "G5:12 R:4", "F5:8 E5:8"),
             bars("E5:12 F5:4", "G5:8 A5:8", "B5:12 R:4", "A5:16"),
             bars("B5:4 C6:4 B5:8", "A5:8 G5:8", "F5:12 R:4", "E5:16"),
             bars("R:16", "G5:8 F5:8", "E5:8 F5:4 G5:4", "B5:16"),
             bars("A5:8 G5:8", "F5:8 E5:8", "F5:8 G5:4 F5:4", "E5:16"))
ASB = phrases(bars("E3:32", "F3:32"), bars("E3:32", "G3:16 A3:16"), bars("C3:32", "D3:32"),
              bars("E3:32", "F3:32"), bars("C3:32", "E3:32"))
add('ASHEN', 175, [(2, 30, 1, 5, 78, 1, 42), (2, 20, 1, 4, 84, 0, 54), (2, 20, 2, 3, 0, 2, 0)], AS, ASB, HEART, steps=320, calm=False)

# 15 VIGIL: C minor, a lullaby for the dead: a near-pure tone, the lift to Ab and a settle back. 384 steps at 230 ms = 88 s.
VG = phrases(bars("G5:8 Eb5:8", "F5:12 R:4", "G5:8 Bb5:8", "Ab5:16"),
             bars("G5:8 F5:4 Eb5:4", "D5:16", "Eb5:8 F5:8", "G5:16"),
             bars("C6:12 R:4", "Bb5:8 Ab5:8", "G5:12 F5:4", "Eb5:16"),
             bars("R:8 D5:4 Eb5:4", "F5:16", "Eb5:8 D5:8", "C5:16"),
             bars("G5:8 Ab5:4 G5:4", "F5:8 Eb5:8", "D5:8 Eb5:4 F5:4", "G5:16"),
             bars("Eb5:12 R:4", "D5:12 R:4", "C5:16", "R:16"))
VGB = phrases(bars("C3:32", "Ab2:32"), bars("Bb2:32", "G2:32"), bars("C3:32", "F3:32"),
              bars("G3:32", "C3:32"), bars("Ab2:32", "Bb2:32"), bars("C3:48", "G2:16"))
add('VIGIL', 185, [(1, 22, 1, 4, 78, 1, 46), (1, 12, 1, 3, 84, 0, 54), (2, 20, 2, 3, 0, 2, 0)], VG, VGB, HEART, steps=384, calm=False)


# ---- 16..18 upbeat jazz, original (v1.3.9): swing on a triplet grid (a bar is 12 steps, a beat 3: a swung eighth pair is
# 2 + 1), a walking bass in quarters, a ride pattern with a feathered kick, bright syncopated tunes. They join the stage songs.
def sw(*bs):                        # bars of 12 steps
    out = []
    for b in bs:
        n = notes(b)
        assert len(n) == 12, (b, len(n))
        out += n
    return out


def walkq(*bs):                     # a walking bass: four quarter notes a bar ("F2 A2 C3 D3")
    return sw(*[" ".join(x + ":3" for x in b.split()) for b in bs])


def ride(*pats):
    out = []
    for p in pats:
        assert len(p) == 12, p
        out += drums(p)
    return out


RIDE, RIDEC, RIDEX = 'b..h.hk..h.h', 'b..h.hk..h.x', 'b..x.hb..x.h'    # the ride; a snare comp; snare on 2 and 4


def jazz(name, ms, inst, lead, bass, dr):
    add(name, ms, inst, lead, bass, dr, steps=len(lead), calm=False)


# 16 BLUFF: F major, AABA, 152 bpm (a triplet 132 ms), a muted-trumpet lead. 384 steps = 32 bars, 51 s.
BL_A = sw("R:2 C5:1 D5:2 F5:1 A5:3 G5:2 F5:1", "F#5:2 D5:1 R:3 C5:2 D5:1 A4:3", "R:2 Bb4:1 D5:2 F5:1 G5:3 Bb5:3",
          "A5:2 G5:1 E5:2 C5:1 Bb4:5 R:1")
BL_A1 = BL_A + sw("C5:2 E5:1 A5:3 R:2 F#5:1 A5:3", "G5:6 R:2 E5:1 G5:3", "F5:2 A5:1 C6:2 A5:1 R:2 F#5:1 A5:3", "G5:3 F5:2 E5:1 R:6")
BL_A2 = BL_A + sw("C5:2 E5:1 A5:2 G5:1 F#5:3 D5:3", "Bb4:2 D5:1 G5:3 E5:2 C5:1 Bb4:3", "A4:2 Bb4:1 B4:2 C5:1 E5:3 G5:3", "F5:6 R:6")
BL_B = sw("E5:2 C#5:1 E5:2 G5:1 A5:3 R:3", "R:2 G5:1 E5:2 C#5:1 A4:6", "F#5:2 A5:1 C6:3 A5:2 F#5:1 D5:3", "R:6 C5:2 C#5:1 D5:3",
          "B4:2 D5:1 F5:2 G5:1 B5:3 A5:3", "G5:2 F5:1 D5:3 B4:3 R:3", "Bb4:2 C5:1 E5:2 G5:1 Bb5:3 A5:2 G5:1", "E5:3 C5:3 R:6")
BLB_A = walkq("F2 A2 C3 D3", "D3 C3 A2 F#2", "G2 Bb2 D3 F3", "C3 E3 G3 Ab2")
BLB_A1 = BLB_A + walkq("A2 E3 D3 F#2", "G2 D3 C3 E2", "F2 C3 D3 A2", "G2 Bb2 C3 E2")
BLB_A2 = BLB_A + walkq("A2 E3 D3 F#2", "G2 D3 C3 E2", "G2 D3 C3 E2", "F2 A2 C3 E2")
BLB_B = walkq("A2 C#3 E3 G3", "A3 G3 E3 C#3", "D3 F#3 A2 C3", "D3 C3 A2 F#2", "G2 B2 D3 F3", "G3 F3 D3 B2",
              "C3 E3 G2 Bb2", "C3 Bb2 G2 E2")
jazz('BLUFF', 132, [(1, 28, 2, 8, 86, 1, 50), (1, 14, 2, 4, 88, 2, 44), (2, 20, 2, 3, 0, 2, 0)],
     BL_A1 + BL_A2 + BL_B + BL_A2, BLB_A1 + BLB_A2 + BLB_B + BLB_A2,
     ride(*[RIDE] * 7 + [RIDEC] + [RIDE] * 7 + [RIDEC] + [RIDEX] * 7 + ['b..x.xb.xx.x'] + [RIDE] * 6 + [RIDEC, 'b..x.hb.x.xx']))

# 17 HIGHCARD: a Bb jazz blues, 168 bpm (a triplet 119 ms), a reedy lead: the head, a solo chorus, a shout chorus of
# stabs, the head again. 576 steps = 48 bars, 69 s.
HC_HEAD = sw("R:2 F5:1 Ab5:2 F5:1 Bb5:3 R:3", "G5:2 F5:1 Db5:3 R:6", "R:2 F5:1 Ab5:2 F5:1 Bb5:3 D6:3",
             "C6:2 Bb5:1 Ab5:3 F5:2 D5:1 R:3", "R:2 G5:1 Bb5:2 G5:1 Db6:3 R:3", "C#6:2 Bb5:1 G5:3 E5:3 R:3",
             "F5:2 D5:1 F5:2 Bb5:1 Ab5:6", "R:2 B4:1 D5:2 F5:1 Ab5:3 G5:3", "R:2 Eb5:1 G5:2 Bb5:1 C6:3 Bb5:3",
             "A5:2 F5:1 Eb5:3 C5:2 A4:1 R:3", "Bb4:2 D5:1 F5:3 B4:2 D5:1 F5:3", "Eb5:3 D5:2 C5:1 A4:3 R:3")
HC_SOLO = sw("D5:2 F5:1 Bb5:2 D6:1 C6:2 Bb5:1 Ab5:3", "G5:2 Bb5:1 Db6:3 R:2 Bb5:1 G5:3", "F5:3 R:2 D5:1 F5:2 G5:1 Ab5:3",
             "Ab5:2 G5:1 F5:2 Eb5:1 D5:3 F5:3", "Eb5:2 G5:1 Bb5:2 Db6:1 C6:6", "Bb5:2 G5:1 E5:2 Db5:1 R:3 E5:3",
             "F5:6 R:2 D5:1 F5:3", "B5:2 Ab5:1 F5:2 D5:1 B4:3 R:3", "C5:2 Eb5:1 G5:2 Bb5:1 D6:3 C6:3",
             "A5:2 C6:1 A5:2 F5:1 Eb5:3 R:3", "D5:2 F5:1 Bb5:3 Ab5:2 F5:1 D5:3", "C5:2 Eb5:1 G5:3 F5:2 Eb5:1 C5:3")
HC_SHOUT = sw("R:2 Bb5:1 R:2 Ab5:1 R:3 F5:3", "R:2 G5:1 R:2 Bb5:1 R:3 Db6:3", "R:2 Bb5:1 R:2 Ab5:1 R:3 F5:3",
              "Ab5:2 F5:1 D5:3 R:6", "R:2 G5:1 R:2 Bb5:1 R:3 Db6:3", "C#6:2 R:1 Bb5:2 R:1 G5:3 E5:3",
              "F5:2 Ab5:1 F5:2 D5:1 Bb4:6", "R:2 D5:1 F5:2 Ab5:1 B5:3 R:3", "C6:3 R:2 Bb5:1 G5:3 Eb5:3",
              "F5:2 A5:1 C6:3 A5:2 F5:1 Eb5:3", "D5:3 F5:2 Bb5:1 B5:3 R:3", "C6:2 Bb5:1 G5:2 Eb5:1 C5:3 A4:3")
HC_BASS = walkq("Bb2 D3 F3 E3", "Eb3 G3 Db3 B2", "Bb2 C3 D3 E3", "F2 Ab2 Bb2 D3", "Eb3 G3 Bb2 Db3", "E3 G3 Db3 C3",
                "Bb2 D3 F3 F#2", "G2 B2 D3 Db3", "C3 Eb3 G2 Gb2", "F2 A2 C3 B2", "Bb2 D3 G2 B2", "C3 Eb3 F2 A2")
jazz('HIGHCARD', 119, [(2, 24, 1, 10, 84, 1, 54), (1, 14, 2, 4, 88, 2, 44), (2, 20, 2, 3, 0, 2, 0)],
     HC_HEAD + HC_SOLO + HC_SHOUT + HC_HEAD, HC_BASS * 4,
     ride(*[RIDE] * 11 + [RIDEC] + [RIDE, RIDEC] * 5 + [RIDE, 'b..x.xb.xx.x'] + [RIDEX] * 11 + ['b..x.xb.xx.x']
          + [RIDE] * 11 + ['b..x.hb.x.xx']))

# 18 ALLIN: C major, AABA, 136 bpm (a triplet 147 ms), a vibraphone lead (a bell that fades). 384 steps = 32 bars, 56 s.
AI_A = sw("R:2 E5:1 G5:2 B5:1 C6:3 B5:2 G5:1", "A5:3 R:2 E5:1 G5:2 E5:1 C5:3", "R:2 D5:1 F5:2 A5:1 C6:3 A5:3",
          "B5:2 G5:1 F5:2 D5:1 B4:3 R:3", "R:2 E5:1 G5:2 B5:1 D6:3 B5:3", "C#6:2 A5:1 G5:2 E5:1 C#5:3 R:3")
AI_A1 = AI_A + sw("D5:2 F5:1 A5:3 G5:2 F5:1 B4:3", "C5:6 R:6")
AI_A2 = AI_A + sw("F5:2 E5:1 D5:3 B4:2 D5:1 G5:3", "E5:3 G5:2 C6:1 R:6")
AI_B = sw("A5:3 C6:2 A5:1 E5:3 F5:3", "Ab5:2 F5:1 C5:3 D5:2 F5:1 Ab5:3", "G5:3 B5:2 G5:1 E5:3 D5:3", "C#5:2 E5:1 G5:2 Bb5:1 A5:6",
          "R:2 A5:1 C6:2 A5:1 F5:3 D5:3", "B4:2 D5:1 F5:2 G5:1 B5:6", "A5:2 F5:1 D5:3 E5:2 F5:1 F#5:3", "G5:3 R:2 B5:1 D6:3 R:3")
AIB_A = walkq("C3 E3 G3 Bb2", "A2 C3 E3 Eb3", "D3 F3 A2 Ab2", "G2 B2 D3 F3", "E3 G3 B2 Bb2", "A2 C#3 E3 Eb3", "D3 F3 G2 B2")
AIB_A1 = AIB_A + walkq("C3 E3 G2 B2")
AIB_A2 = AIB_A + walkq("C3 E3 G3 E3")
AIB_B = walkq("F2 A2 C3 E3", "F2 Ab2 Bb2 D3", "E2 G2 B2 Bb2", "A2 C#3 E3 Eb3", "D3 F3 A3 F3", "G2 B2 D3 F3", "D3 E3 F3 F#3", "G3 F3 D3 B2")
jazz('ALLIN', 147, [(4, 20, 2, 2, 84, 2, 30), (1, 14, 2, 4, 88, 2, 44), (2, 20, 2, 3, 0, 2, 0)],
     AI_A1 + AI_A2 + AI_B + AI_A2, AIB_A1 + AIB_A2 + AIB_B + AIB_A2,
     ride(*[RIDE, RIDEC] * 4 + [RIDE] * 7 + ['b..x.xb.xx.x'] + [RIDEC, RIDE] * 4 + [RIDE, RIDEC] * 3 + [RIDE, 'b..x.hb.x.xx']))


for sg in SONGS:
    for part in ('lead', 'bass', 'drums'):
        assert len(sg[part]) == sg['steps'], (sg['name'], part, len(sg[part]))
        assert max(sg[part]) < 128

out = ["// Generated by tools/make_music.py: step songs for the FM synth, the sine table and the pitch table.",
       "#pragma once", "#include <stdint.h>", "", "#define SYN_RATE %d" % RATE, "#define SYN_SONGS %d" % len(SONGS), ""]
out.append("static const int8_t SINEQ[65] = {   // a quarter of the sine (Synth.cpp mirrors it into 256 steps)")
sine = [int(round(127 * math.sin(i * 2 * math.pi / 256))) for i in range(256)]
assert all(sine[(256 - i) % 256] == -sine[i] and sine[128 - i] == sine[i] for i in range(1, 128))
out.append("    " + ",".join(str(v) for v in sine[:65]) + ",")
out.append("};")
inc16 = [int(round(440 * 2 ** ((60 + k - 69) / 12) * 65536 / RATE * 16)) for k in range(12)]
assert max(inc16) < 65536
out.append("static const uint16_t NOTE_INC16[12] = { " + ", ".join(str(v) for v in inc16) + " };")
semi = [int(round(256 * 2 ** (k / 12))) for k in range(13)]
out.append("static const uint16_t SEMI_Q8[13] = { " + ", ".join(str(v) for v in semi) + " };   // 2^(k/12) in 1/256ths: effects raised in pitch")
out.append("// an FM voice: modulator ratio, index at the start, its fall and floor; volume at the start, its fall while held and floor")
out.append("struct Inst { uint8_t ratio, idx0, idxDecay, idxFloor, amp, volDecay, volFloor; };")
out.append("struct SongHdr { const uint8_t *lead, *bass, *drums; const Inst *inst; uint8_t stepMs, delay; uint16_t steps; };   // voice 2 echoes the lead `delay` steps later")
for sg in SONGS:
    nm = sg['name']
    out.append("static const Inst %s_INST[3] = { %s };" % (nm, ", ".join("{ %s }" % ", ".join(str(x) for x in i) for i in (list(sg['inst']) + [sg['inst'][-1]] * 3)[:3])))
    for part in ('lead', 'bass', 'drums'):
        out.append("static const uint8_t %s_%s[%d] = {" % (nm, part.upper(), sg['steps']))
        for i in range(0, sg['steps'], 32):
            out.append("    " + ",".join(str(v) for v in sg[part][i:i + 32]) + ",")
        out.append("};")
out.append("static const SongHdr SONGS[SYN_SONGS] = {")
for sg in SONGS:
    nm = sg['name']
    out.append("    { %s_LEAD, %s_BASS, %s_DRUMS, %s_INST, %d, %d, %d }," % (nm, nm, nm, nm, sg['ms'], sg['delay'], sg['steps']))
out.append("};")
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Music.h'), 'w', newline='').write("\n".join(out) + "\n")
print("wrote Music.h")
