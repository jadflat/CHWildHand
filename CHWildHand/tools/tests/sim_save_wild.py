"""Wild Hand's wild-card save test in the simulator: five wild cards and their counters survive a power cycle (the counters ride in the
top nibble of the hand levels, the 4th and 5th card in the old struct's padding).

python tools/tests/sim_save_wild.py   (from CHWildHand/CHWildHand; CHSD_CARD must name sdcard/WILDHAND.DAT)
"""
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
os.environ.setdefault("CHSD_CARD", str(ROOT / "sdcard" / "WILDHAND.DAT"))
sys.path.insert(0, str(ROOT.parent / "chgame" / "tools" / "chsim"))
from simsave import Session  # noqa: E402


def jok(s):
    words = (s.say("j") or "").split()
    return " ".join(w for w in words if ":" in w)


s = Session(ROOT, prefix="J ")
s.say("Z 99")
s.frames(30)
s.tap("A"); s.frames(12); s.tap("A"); s.frames(30)          # a new run, past the deal
s.say("J 25 27 24 33 38")                                   # MOLD, TOLL BELL, LEECH, MOTHS, SEER BONE
s.say("x 0 0 1 0 2 0 3 0 4 0 9 1 7 2 5 3"); s.frames(3)
for _ in range(5):
    s.tap("A"); s.tap("RIGHT")
s.tap("B"); s.frames(60); s.tap("A"); s.frames(10)          # play a hand: MOLD and TOLL BELL count
before = jok(s)
s.check(before == "25:1 27:1 24:0 33:0 38:0", "the hand counted: %r" % before)
s.say("X"); s.frames(20)                                    # the shop saves the run
s.power_cycle(); s.frames(10)
s.tap("A"); s.frames(20)                                    # CONTINUE
after = jok(s)
s.check(after == before, "five-slot save: cards and counters survive: %r -> %r" % (before, after))
