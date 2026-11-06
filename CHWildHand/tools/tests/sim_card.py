"""Wild Hand's settings-on-the-card test in the simulator (Card.cpp putSettings/settings): a changed setting is written to
CARD_SET, the DAT's last block, and wins over the flash save at the next boot; a block that is not the fill or an earlier set
is never written; a bad block is ignored.

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
tmp = Path(tempfile.mkdtemp())
card = tmp / "WILDHAND.DAT"
os.environ["CHSD_CARD"] = str(card)
fails = 0


def check(ok, what):
    global fails
    print(("ok   " if ok else "FAIL ") + what, flush=True)
    fails += not ok


def block():
    with open(card, "rb") as f:
        f.seek(SET)
        return f.read(512)


def put(b):
    with open(card, "r+b") as f:
        f.seek(SET)
        f.write(b)


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
check(block() == b"\xff" * 512, "the fresh DAT's settings block is all 0xFF (block at %d)" % SET)

# 1. boot with no settings on the card: the defaults (music on); a pause-menu change is written to the card
s = session(True)
check(state(s)["music"] == "1", "no settings on the card: defaults")
newrun(s)
pause_toggle(s, 1, 2)                                      # MUSIC off, then not yet written (20 frame delay)
check(block() == b"\xff" * 512, "not written before the delay is over")
s.frames(30)
s.tap("START"); s.frames(4)
b = block()
check(b[0:1] == b"W" and b[1] & 1 == 0 and b[4] == (b[1] + b[2] + b[3] + 0xA5) & 255 and b[5:] == b"\xff" * 507, "MUSIC off written: %s" % b[:6].hex())
pause_toggle(s, 2)                                         # EFFECTS calm
b2 = block()
check(b2[2] & 1 == 1 and b2[1] & 1 == 0, "EFFECTS calm written too: %s" % b2[:6].hex())
check(card.stat().st_size == size0, "the DAT's size is unchanged")
s.result()

# 2. a new boot (the flash save is empty in a new process): the card's settings are loaded
s = session(False)
st = state(s)
check(st["music"] == "0" and st["fx"] == "1", "boot: settings come from the card with no flash save: %s" % st)

# 3. the card wins over the flash save: change a setting with the write refused, then reload
newrun(s)
pause_toggle(s, 1)                                         # MUSIC on again (flash only: the write is refused)
check(state(s)["music"] == "1" and block() == b2, "write refused: flash has MUSIC on, the card keeps the old set")
s.say("V"); s.frames(10)
st = state(s)
check(st["music"] == "0" and st["fx"] == "1", "reload: the card's settings win over the flash save: %s" % st)
s.result()

# 4. a bad checksum is ignored
bad = bytearray(b2)
bad[4] ^= 1
put(bytes(bad))
s = session(False)
st = state(s)
check(st["music"] == "1" and st["fx"] == "0", "a block with a bad check byte is ignored: %s" % st)
s.result()

# 5. a block that is neither the fill nor a set is never written over
foreign = bytes(range(256)) * 2
put(foreign)
s = session(True)
newrun(s)
pause_toggle(s, 1)
check(block() == foreign, "a foreign block is left alone")
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
