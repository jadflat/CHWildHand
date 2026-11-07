"""Roguelike carry-over: a death keeps the first joker and half of each hand's level (rounded down) for the next run.
python tools/tests/sim_carry.py   (from CHWildHand/CHWildHand)"""
import os, shutil, sys, tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT.parent / "chgame" / "tools" / "chsim"))
from simsave import Session  # noqa: E402
tmp = Path(tempfile.mkdtemp()); card = tmp / "WILDHAND.DAT"
shutil.copyfile(ROOT / "sdcard" / "WILDHAND.DAT", card)
os.environ["CHSD_CARD"] = str(card)
s = Session(ROOT)
def st(): return dict(w.split("=", 1) for w in (s.say("A") or "").split() if "=" in w)
def jok(): return (s.say("j") or "").strip()
s.say("Z 99"); s.frames(10)
s.tap("A"); s.frames(12); s.tap("A"); s.frames(30)
s.say("J 7 3"); s.say("I 5"); s.say("R 4"); s.frames(2)
s.say("X"); s.frames(5); s.say("O"); s.frames(5)   # the shop saves (fillSave), then the title
s.tap("A"); s.frames(30)                      # back to the title
print("after death:", st(), jok())
s.tap("DOWN"); s.frames(3); s.tap("A"); s.frames(12); s.tap("A"); s.frames(30)
print("new run:", st(), jok())
bad = 0
bad += st()["lv1"] != "2"; bad += st()["nj"] != "1" or st()["round"] != "0"
s.result()
shutil.rmtree(tmp, ignore_errors=True)
print("FAIL" if bad else "ok"); raise SystemExit(bad)
