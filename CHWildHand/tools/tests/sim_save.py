"""Wild Hand's save test in the simulator: settings, a run and the turbo best survive a power cycle.

python tools/tests/sim_save.py   (from CHWildHand/CHWildHand; CHSD_CARD must name sdcard/WILDHAND.DAT)
"""
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
os.environ.setdefault("CHSD_CARD", str(ROOT / "sdcard" / "WILDHAND.DAT"))
sys.path.insert(0, str(ROOT.parent / "chgame" / "tools" / "chsim"))
from simsave import Session  # noqa: E402


def state(s):
    words = (s.say("A") or "").split()
    return dict(w.split("=", 1) for w in words if "=" in w)


s = Session(ROOT)
s.say("Z 99")                                              # no opening bonus stage
s.frames(30)
s.tap("A"); s.frames(12); s.tap("A"); s.frames(30)          # a new run, past the deal
s.say("M 23"); s.say("I 3")
s.tap("START"); s.frames(4)
s.tap("DOWN"); s.tap("A"); s.frames(2)                      # MUSIC off
s.tap("DOWN"); s.tap("A"); s.frames(2)                      # EFFECTS calm
for _ in range(5): s.tap("DOWN")                          # CARD, INDEX, SONG, EQ, THEME
s.tap("RIGHT")                                              # THEME NOIR (its options)
s.tap("DOWN"); s.tap("RIGHT"); s.tap("RIGHT")              # then FACE two on
for _ in range(5): s.tap("DOWN")                          # RED, BLACK, HILITE, ACCENT, BG
for _ in range(3): s.tap("RIGHT")                         # BG EMBER
s.tap("DOWN"); s.tap("RIGHT")                               # BG HOLD ALWAYS
s.tap("DOWN")                                               # SAVE GAME
s.tap("A"); s.frames(4)
st = state(s)
s.check(st["music"] == "0" and st["fx"] == "1" and st["save"] == "1", "settings set and the run saved: %s" % st)
s.power_cycle()
s.frames(10)
st = state(s)
s.check(st["music"] == "0" and st["fx"] == "1", "settings survive a power cycle: %s" % st)
s.check(st["th"] == "1" and st["col"] == str(0xB11004), "the COLORS and BG options survive (THEME NOIR, FACE two on, BG EMBER, HOLD ALWAYS): %s" % st)
s.check(st["save"] == "1", "the run survives")
s.tap("A"); s.frames(20)                                    # CONTINUE
st = state(s)
s.check(st["money"] == "23" and st["lv1"] == "3", "CONTINUE restores money and hand levels: %s" % st)
# the title's BONUS (practice) leaves the saved run alone: lose a stage, back to the title, power cycle, CONTINUE
s.say("V"); s.frames(10)                                    # back to the title (the power cycle's state, no new load)
s.tap("DOWN"); s.tap("DOWN"); s.frames(4)                   # CONTINUE, NEW RUN, BONUS
s.tap("A"); s.frames(12)                                    # the stage select
s.tap("A"); s.frames(215); s.tap("A"); s.frames(12)         # THE CHASE: through the show, into the play
s.say("E 20"); s.frames(70)                                 # time up: CAUGHT (money halves in practice only)
st = state(s)
s.check(st["money"] != "23", "the practice stage was lost (money halved): %s" % st)
s.tap("A"); s.frames(8); s.tap("B"); s.frames(8)            # the list, then the title
s.power_cycle()
s.frames(10)
st = state(s)
s.check(st["save"] == "1" and st["music"] == "0", "the save survives a practice: %s" % st)
s.tap("A"); s.frames(20)                                    # CONTINUE
st = state(s)
s.check(st["money"] == "23" and st["lv1"] == "3" and st["nj"] == "0", "CONTINUE after a practice: money and levels as saved: %s" % st)
raise SystemExit(s.result())
