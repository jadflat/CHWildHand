"""Which songs a build has and which one plays where.

PUBLIC builds (the repository, the release) have only the songs in make_music.py; a local build can add more
from tools/songs_private.py (not in the repository). PUBLIC=1 in the environment forces the public build; without
tools/songs_private.py the build is public anyway. Song numbers are rows of the SD card file (make_music.py order)."""
import os

PRIVATE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'songs_private.py')
PUBLIC = os.environ.get('PUBLIC') == '1' or not os.path.exists(PRIVATE)
VERSION = '1.3.33'

if PUBLIC:   # 0 AMBER, 1 AMBNIGHT, 2 CRYPT, 3 CATACOMB, 4 ASHEN, 5 VIGIL, 6 BLUFF, 7 HIGHCARD, 8 ALLIN
    ID = dict(TITLE=2, SHOP_A=6, SHOP_B=0, OVER=5, TURBO=8, BONUS=4, PAUSE=1)
    KSONG = [2, 6, 3, 7, 4, 8, 5]            # the stage songs
else:
    ID = dict(TITLE=0, SHOP_A=2, SHOP_B=10, OVER=3, TURBO=4, BONUS=8, PAUSE=11)
    KSONG = [12, 1, 13, 5, 14, 6, 15, 7, 9, 4, 16, 17, 18]
