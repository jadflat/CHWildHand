"""Wild Hand's save mirror in the simulator (Card.cpp putSave/getSave): every save is also written to the DAT's two last blocks
(CARD_SET, +512) as alternating records (generation, checksum); the newest good record restores a lost flash save and its
settings win over the flash save at the next boot; a block that is neither the fill nor a record is never written; a bad record
is ignored.

python tools/tests/sim_card.py   (from CHWildHand/CHWildHand; works on a copy of sdcard/WILDHAND.DAT)
The simulator's card is read-only unless $WH_CARDW is set; this test sets it for the sessions that should write.
"""
import os
import re
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT.parent / "chgame" / "tools" / "chsim"))
from simsave import Session  # noqa: E402

SET = int(re.search(r"CARD_SET = (\d+)", (ROOT / "CardIndex.h").read_text()).group(1))
FILL = bytes([255]) * 512
tmp = Path(tempfile.mkdtemp())
card = tmp / "WILDHAND.DAT"
os.environ["CHSD_CARD"] = str(card)
fails = 0


def check(ok, what):
    global fails
    print(("ok   " if ok else "FAIL ") + what, flush=True)
    fails += not ok


def block(k=0):
    with open(card, "rb") as f:
        f.seek(SET + 512 * k)
        return f.read(512)


def put(b, k=0):
    with open(card, "r+b") as f:
        f.seek(SET + 512 * k)
        f.write(b)


def rec(b):                                                # (gen, payload) of a good record, else None
    if b[4:5] != b"S": return None
    n = b[5]; t = 0
    for x in b[:n + 6]: t = (t * 31 + x + 1) & 0xFFFF
    return (int.from_bytes(b[:4], "little"), b[6:6 + n]) if t == b[6 + n] | b[7 + n] << 8 and b[8 + n:] == bytes([255]) * (504 - n) else None


def settings(b):                                           # (music, fx, pad) bytes of the record (SaveData: 12 B of ints, round_ nJok jok[3] lv[9], music fx dealerBase pad)
    r = rec(b)
    return None if not r else (r[1][26] & 1, r[1][27] & 1)


def state(s):
    return dict(w.split("=", 1) for w in (s.say("A") or "").split() if "=" in w)


def session(write):
    if write:
        os.environ["WH_CARDW"] = "1"
    else:
        os.environ.pop("WH_CARDW", None)
    s = Session(ROOT)
    s.say("Z 99")
    s.frames(10)
    return s


def pause_toggle(s, line, frames=40):                      # START, down to a line (1 MUSIC, 2 EFFECTS), A, wait; the write comes 20 frames after the A
    s.tap("START"); s.frames(4)
    for _ in range(line): s.tap("DOWN")
    s.tap("A"); s.frames(frames)
    if frames > 10: s.tap("START"); s.frames(4)            # and back to the table


def newrun(s):
    s.tap("A"); s.frames(12); s.tap("A"); s.frames(30)


def reset():
    shutil.copyfile(ROOT / "sdcard" / "WILDHAND.DAT", card)


reset()
size0 = card.stat().st_size
check(block(0) == FILL and block(1) == FILL, "the fresh DAT save blocks are all 0xFF (block at %d)" % SET)

# 1. boot with no settings on the card: the defaults (music on); a pause-menu change is written to the card
s = session(True)
check(state(s)["music"] == "1", "no settings on the card: defaults")
newrun(s)
pause_toggle(s, 1, 2)                                      # MUSIC off, then not yet written (20 frame delay)
check(block(0) == FILL and block(1) == FILL, "not written before the delay is over")
s.frames(30)
s.tap("START"); s.frames(4)
r = rec(block())
check(r and r[0] == 1 and block(1) == FILL and r[1][26] & 1 == 0, "MUSIC off: record gen 1 in the first block (%s)" % (r and r[0]))
pause_toggle(s, 2)                                         # EFFECTS calm
r2 = rec(block(1))
check(r2 and r2[0] == 2 and r2[1][26] & 1 == 0 and rec(block())[0] == 1, "EFFECTS calm: gen 2 in the second block, gen 1 kept")
b2 = block(1)
check(card.stat().st_size == size0, "the DAT's size is unchanged")
s.result()

# 2. a new boot (the flash save is empty in a new process): the card's settings are loaded
s = session(False)
st = state(s)
check(st["music"] == "0" and st["fx"] == "1", "boot: the newest record restores the save with no flash save: %s" % st)

# 3. the card wins over the flash save: change a setting with the write refused, then reload
newrun(s)
pause_toggle(s, 1)                                         # MUSIC on again (flash only: the write is refused)
check(state(s)["music"] == "1" and block(1) == b2, "write refused: flash has MUSIC on, the card keeps the old set")
s.say("V"); s.frames(10)
st = state(s)
check(st["music"] == "0" and st["fx"] == "1", "reload: the card's settings win over the flash save: %s" % st)
s.result()

# 4. a bad checksum is ignored
bad = bytearray(b2)
bad[6] ^= 1
put(bytes(bad), 1)                                         # the newest is torn: the older record (MUSIC off, EFFECTS not yet calm) is used
s = session(False)
st = state(s)
check(st["music"] == "0" and st["fx"] == "0", "a torn newest record: the older one is used: %s" % st)
s.result()
put(bytes(bad), 0)
s = session(False)
st = state(s)
check(st["music"] == "1" and st["fx"] == "0", "both records bad: ignored, defaults: %s" % st)
s.result()

# 5. a block that is neither the fill nor a set is never written over
foreign = bytes(range(256)) * 2
put(foreign); put(foreign, 1)
s = session(True)
newrun(s)
pause_toggle(s, 1)
check(block() == foreign and block(1) == foreign, "foreign blocks are left alone")
s.result()
reset()

# 6. an old DAT without the block (CARD_SET past its end): nothing is written, nothing breaks
with open(card, "r+b") as f:
    f.truncate(SET)
s = session(True)
newrun(s)
pause_toggle(s, 1)
check(card.stat().st_size == SET and state(s)["music"] == "0", "a DAT without the block: size unchanged, the game goes on")
s.result()

shutil.rmtree(tmp, ignore_errors=True)
raise SystemExit(1 if fails else 0)
