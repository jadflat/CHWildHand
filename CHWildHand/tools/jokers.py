"""The wild cards' data: name, description, price, slot colour, HUD meter and the rules the game's engine runs (Rules.h jrun/jeff).

tools/make_card.py packs RULES into WILDHAND.DAT (loaded into RAM with the sound tables); tools/card_text.py takes the names and
descriptions. One card = a 4-byte header and 4 bytes a rule:
  header  b0: price (4 bits) | rules (2 bits) << 4 | meter (2 bits) << 6      meter: 0 none, 1 growth bar, 2 charge segments, 3 flame
          b1: slot colour (palette index, 4 bits) | param (4 bits): the counter's cap (a charge meter's segments)
          b2, b3: the picture's offset in the DAT's picture section
  rule    b0: event (3 bits) << 5 | condition (5 bits)     b1: the condition's argument (or, condition SCALED: what multiplies the amount)
          b2: effect (4 bits) << 4 | flags (4 bits)         b3: the amount
"""
# events: when the rule runs
HAND, CARD, PICK, TOSS, START, WIN = range(6)
# conditions (cond, arg)
ALWAYS, SUIT, RANK, EVEN, TYPES, TYPE, NCARDS, PLAYS, DONE, SUITS, SAMEPREV, CNT, CHANCE, BOSS, TIMED, SCALED = range(16)   # SAMEPREV: the same hand type as the last
# effects: target << 1 | negative; MULX multiplies the mult (negative: halves it, rounding up), CNTUP counts the card's counter up (to its cap)
CHIPS, MULT, MONEY, PLAYSLEFT, DISC, TIME, MULX, CNTUP = range(8)
NOP = (CHIPS, 0)
# flags
RESET, BURN = 1, 2
# SCALED's multipliers
Q_CNT, Q_DISC, Q_ROUND, Q_MONEY, Q_FIRST = range(5)


def eff(t, neg=0):
    return t << 1 | neg


def R(ev, cond, arg, e, amt, flags=0, neg=0):
    return (ev, cond, arg, eff(e, neg), amt, flags)


def rk(lo, hi):
    return lo << 4 | hi


FACE, ACE = rk(9, 11), rk(12, 12)
PAIRISH, STRAIGHTISH = 0x67, 0x98             # hand types 1,2,3,6,7 / 4,5,8 as a mask (bit k: type k+1)

P_BG, P_FELT, P_CARD, P_RED, P_INK, P_GOLD, P_GREY, P_DARK, P_TEXT, P_CHIP, P_SKIN, P_GLOW, P_FELTLT, P_SHADE, P_GOLDDK, P_WINE = range(16)

# (name, description, price, slot colour, meter, param, rules): the first 24 keep their scoring to the point (tools/regress.py, 'K' in the simulator)
CARDS = [
    ("DEMON", "+4 MULT", 4, P_INK, 0, 0, [R(HAND, ALWAYS, 0, MULT, 4)]),
    ("BONES", "+40 CHIPS", 4, P_DARK, 0, 0, [R(HAND, ALWAYS, 0, CHIPS, 40)]),
    ("BLEEDER", "+2 MULT/HEART", 5, P_WINE, 0, 0, [R(CARD, SUIT, 0, MULT, 2)]),
    ("SEXTON", "+15 CHIPS/SPADE", 5, P_SHADE, 0, 0, [R(CARD, SUIT, 3, CHIPS, 15)]),
    ("TWO FACES", "+8 MULT ON PAIRS", 6, P_SHADE, 0, 0, [R(HAND, TYPES, PAIRISH, MULT, 8)]),
    ("SKULL KING", "+3 MULT/FACE", 6, P_WINE, 0, 0, [R(CARD, RANK, FACE, MULT, 3)]),
    ("CLAWS", "X2 MULT STR/FLUSH", 7, P_INK, 0, 0, [R(HAND, TYPES, STRAIGHTISH, MULX, 2)]),
    ("THE EYE", "+30 CHIPS/ACE", 6, P_DARK, 0, 0, [R(CARD, RANK, ACE, CHIPS, 30)]),
    ("BLOOD GEM", "+$1 PER DIAMOND", 5, P_DARK, 0, 0, [R(CARD, SUIT, 1, MONEY, 1)]),
    ("CUDGEL", "+3 MULT/CLUB", 5, P_WINE, 0, 0, [R(CARD, SUIT, 2, MULT, 3)]),
    ("TOMBSTONE", "+4 MULT PER 2-5", 5, P_CHIP, 0, 0, [R(CARD, RANK, rk(0, 3), MULT, 4)]),
    ("SCALES", "+12 CHIPS/EVEN", 5, P_INK, 0, 0, [R(CARD, EVEN, 8, CHIPS, 12)]),
    ("THE HAND", "+6 MULT ON 5 CARDS", 5, P_CHIP, 0, 0, [R(HAND, NCARDS, 5, MULT, 6)]),
    ("CANDLE", "+15 MULT HIGH CARD", 4, P_INK, 0, 0, [R(HAND, TYPE, 0, MULT, 15)]),
    ("DEATH KNELL", "X3 MULT LAST HAND", 7, P_WINE, 0, 0, [R(HAND, PLAYS, 1, MULX, 3)]),
    ("BONE KEY", "+60 CHIPS 1ST HAND", 5, P_DARK, 0, 0, [R(HAND, DONE, 0, CHIPS, 60)]),
    ("CIRCLE", "X2 MULT 4 SUITS", 6, P_INK, 0, 0, [R(HAND, SUITS, 15, MULX, 2)]),
    ("SHROUD", "+3 MULT/DISCARD", 6, P_WINE, 0, 0, [R(HAND, SCALED, Q_DISC, MULT, 3)]),
    ("COFFIN", "+2 CHIPS PER $", 6, P_DARK, 0, 0, [R(HAND, SCALED, Q_MONEY, CHIPS, 2)]),
    ("REVENANT", "+1 MULT PER ROUND", 6, P_WINE, 0, 0, [R(HAND, SCALED, Q_ROUND, MULT, 1)]),
    ("RAVEN", "1ST CARD TWICE", 5, P_SHADE, 0, 0, [R(HAND, SCALED, Q_FIRST, CHIPS, 1)]),
    ("BONE DICE", "X2 MULT, -1 HAND", 8, P_WINE, 0, 0, [R(HAND, ALWAYS, 0, MULX, 2), R(START, ALWAYS, 0, PLAYSLEFT, 1, 0, 1)]),
    ("SEVENTH SON", "+7 MULT PER 7", 5, P_INK, 0, 0, [R(CARD, RANK, rk(5, 5), MULT, 7)]),
    ("GARGOYLE", "+80 CHIPS -1 MULT", 5, P_DARK, 0, 0, [R(HAND, ALWAYS, 0, CHIPS, 80), R(HAND, ALWAYS, 0, MULT, 1, 0, 1)]),
    # ---- growing: the card's counter climbs, the bonus is the counter
    ("LEECH", "+2 MULT PER TOSS", 4, P_WINE, 1, 15,
     [R(START, ALWAYS, 0, CHIPS, 0, RESET), R(TOSS, ALWAYS, 0, CNTUP, 1), R(HAND, SCALED, Q_CNT, MULT, 2)]),
    ("MOLD", "+1 MULT/HAND, GROWS", 5, P_DARK, 1, 15, [R(HAND, ALWAYS, 0, CNTUP, 1), R(HAND, SCALED, Q_CNT, MULT, 1)]),
    ("TROPHY", "+5 MULT PER HEX WON", 4, P_INK, 1, 15, [R(WIN, BOSS, 0, CNTUP, 1), R(HAND, SCALED, Q_CNT, MULT, 5)]),
    # ---- charging: every Nth time, a big effect, then the meter empties
    ("TOLL BELL", "EVERY 3RD HAND X3", 7, P_SHADE, 2, 3, [R(HAND, ALWAYS, 0, CNTUP, 1), R(HAND, CNT, 3, MULX, 3, RESET)]),
    ("MAGGOT JAR", "EVERY 3RD HEART: $1", 4, P_DARK, 2, 3, [R(PICK, SUIT, 0, CNTUP, 1), R(PICK, CNT, 3, MONEY, 1, RESET)]),
    ("WHISPER", "+30 CHIPS PER TOSS", 4, P_CHIP, 1, 4, [R(TOSS, ALWAYS, 0, CNTUP, 1), R(HAND, SCALED, Q_CNT, CHIPS, 30, RESET)]),
    # ---- burning: one use, then ash and a free slot
    ("WICK", "NEXT HAND X3, BURNS", 3, P_INK, 3, 0, [R(HAND, ALWAYS, 0, MULX, 3, BURN)]),
    ("ASH URN", "WIN: +$8, THEN ASH", 4, P_SHADE, 3, 0, [R(WIN, ALWAYS, 0, MONEY, 8, BURN)]),
    # ---- lucky: rnd()
    ("GHOUL LUCK", "1 IN 4: X3 MULT", 5, P_DARK, 0, 0, [R(HAND, CHANCE, 64, MULX, 3)]),
    ("MOTHS", "1/5 CARDS: +10 MULT", 5, P_INK, 0, 0, [R(CARD, CHANCE, 51, MULT, 10)]),
    ("WISHBONE", "1 IN 3 TOSSES FREE", 4, P_WINE, 0, 0, [R(TOSS, CHANCE, 85, DISC, 1)]),
    # ---- risk: a cost for a lot
    ("BLOOD PACT", "X2 MULT, -$3 A HAND", 6, P_WINE, 0, 0, [R(HAND, ALWAYS, 0, MULX, 2), R(HAND, ALWAYS, 0, MONEY, 3, 0, 1)]),
    ("NOOSE", "+10 MULT, -1 TOSS", 6, P_INK, 0, 0, [R(HAND, ALWAYS, 0, MULT, 10), R(START, ALWAYS, 0, DISC, 1, 0, 1)]),
    # ---- play-changers
    ("SHOVEL", "+1 TOSS EACH NIGHT", 4, P_DARK, 0, 0, [R(START, ALWAYS, 0, DISC, 1)]),
    ("SEER BONE", "SEE THE NEXT CARD", 3, P_CHIP, 0, 0, []),
    ("SAND CLOCK", "+3 MULT, +2S TIMED", 4, P_SHADE, 0, 0, [R(HAND, ALWAYS, 0, MULT, 3), R(HAND, TIMED, 0, TIME, 2)]),
]
NCARDS_ = len(CARDS)

# the newer dealers' edges and hexes' twists, as records after the wild cards' (ids NJOK.., Rules.h xrun): dealers 6-8, then hexes 5-6
EXTRA = [
    ("PLAGUE DOCTOR", [R(TOSS, ALWAYS, 0, MONEY, 3), R(START, ALWAYS, 0, DISC, 1, 0, 1)]),                      # a toss pays $3, one toss fewer
    ("SCARECROW", [R(HAND, NCARDS, 5, CHIPS, 25)]),                                                             # +25 chips on 5 cards
    ("PORCELAIN DOLL", [R(START, ALWAYS, 0, PLAYSLEFT, 1), R(START, ALWAYS, 0, MONEY, 3, 0, 1)]),              # +1 hand, costs $3
    ("FADING HANDS", [R(HAND, DONE, 1, CHIPS, 10, 0, 1), R(HAND, DONE, 2, CHIPS, 20, 0, 1), R(HAND, DONE, 3, CHIPS, 30, 0, 1)]),
    ("NO REPEATS", [R(HAND, SAMEPREV, 0, MULX, 2, 0, 1)]),                                                      # the same hand again: mult halved
]
NEW = 24
SEER = 38


def table(art_off):
    """RULES as bytes; art_off(i): wild card i's picture offset in the picture section (a 16-bit number)."""
    out = bytearray()
    for i, (name, desc, price, bg, meter, param, rules) in enumerate(CARDS):
        assert price < 16 and len(rules) < 4 and param < 16 and meter < 4 and len(desc) <= 20, name
        a = art_off(i)
        out += bytes([price | len(rules) << 4 | meter << 6, bg | param << 4, a & 255, a >> 8])
        for ev, cond, arg, e, amt, flags in rules:
            assert 0 <= amt < 256 and 0 <= arg < 256 and e < 16 and flags < 16
            out += bytes([ev << 5 | cond, arg, e << 4 | flags, amt])
    for name, rules in EXTRA:
        assert len(rules) < 4, name
        out += bytes([len(rules) << 4, 0, 0, 0])
        for ev, cond, arg, e, amt, flags in rules:
            out += bytes([ev << 5 | cond, arg, e << 4 | flags, amt])
    return bytes(out)
