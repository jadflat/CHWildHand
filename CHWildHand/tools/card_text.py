"""The game's text that lives on the SD card (WILDHAND.DAT, CARD_TEXT), read by tools/make_card.py.

Each group is one 512 B block of NUL-terminated strings; the game finds string i by skipping i NULs (cardText() in Gfx.h).
Only text drawn on calm screens goes here (the shop): the first read of a block costs one card read.
"""

# the wild cards (tools/jokers.py: names, descriptions, prices and rules), in id order
import jokers
JOKERS = [(c[0], c[1]) for c in jokers.CARDS]
JSPLIT = 20            # descriptions 0..19 sit in block 1 (with CALM), the rest in block 5

# the shop's own lines, after the names in block 0 (TX_SAY.. in Gfx.h, in this order)
SHOP = ["COME IN. STAY.", "IT IS YOURS NOW", "NO COIN, NO DEAL", "NO ROOM LEFT...", "GO... IF YOU CAN", "SHUFFLE BONES?",
        "NEW OFFERINGS", "COSTS $2", "DEEPER IN", "REROLL $2", "ONWARD >", "TAKEN", "GOAL %d"]

# the pause menu and the hand-levels table, after the descriptions in block 1 (TX_RESUME.. in Bonus.h, in this order)
CALM = ["RESUME", "SAVE GAME", "QUIT TO TITLE", "PAUSED", "SHAKES AND FLASHES OFF", "THE FULL HORROR",
        "HAND LEVELS", "LEVELS RISE IN PLAY"]

# the dealers' names and edges (Dealers.h order), the bonus stage's prizes, rules and outcomes (TX_DNAME.. in Bonus.h)
DEALERS = [("BEHOLDER", "SEES THE NEXT CARD"), ("MIMIC", "JAWS: +1 DISCARD"), ("DRAGON", "+$1 EVERY HAND"),
           ("MIND FLAYER", "DEALS YOU A PAIR"), ("CUBE", "DIGESTS: +$4 A WIN"), ("LICH", "+10 CHIPS EVERY HAND"),
]
DEALERS += [("PLAGUE DOCTOR", "TOSS PAYS $3, 1 LESS"), ("SCARECROW", "5 CARDS: +25 CHIPS"), ("PORCELAIN DOLL", "+1 HAND, COSTS $3")]
assert all(len(e) <= 20 for _, e in DEALERS)
# block 8 (TX_DNAME, TX_DEDGE = NDEAL in Bonus.h): the nine dealers' names, then their edges
DEAL2 = [n for n, _ in DEALERS] + [e for _, e in DEALERS]
BONUS = ([""] * 12 +                    # (where the six dealers' names and edges were: they are in block 8 now, with the newer three)
         ["CURSED GOLD", "FREE WILD CARD", "X2 NEXT ROUND",
          "", "PAIRS OR BETTER", "HIGHER OR LOWER?", "CALL 4 OF 5", "PICK HIGHER CARD", "BEST OF THREE", "6 PAIRS, 4 MISSES", "MISS SWAPS UNSEEN",
          "LOSE HALF $, 1 HAND", "THE GOLD IS YOURS", "+$12 (NO ROOM)", "A NEW WILD CARD", "ALL HANDS SCORE X2",
          "HALF YOUR $ IS GONE", "-1 HAND NEXT ROUND"])
# the bonus stages' names and lines (TX_MINI.. in Bonus.h)
MINI = ["THE CHASE", "HIGH OR LOW", "THE DUEL", "GRAVES",
          "UP: HIGHER", "DOWN: LOWER", "A TIE: YOURS!", "RIGHT!", "WRONG!", "YOU TAKE IT!", "IT TAKES IT",
          "LEFT/RIGHT, A: PICK", "ARROWS, A: FLIP",
          "NORMAL", "SMALL", "MEDIUM", "LARGE", "HUGE",   # the pause menu's CARD sizes
          "SOFT", "MUFFLED", "WARM", "DARK", "CRISP", "BASS", "TINNY", "RADIO", "CAVE", "AUTO",
          "COUNT THE HEARTS", "COUNT THE DIAMONDS", "COUNT THE CLUBS", "COUNT THE SPADES", "COUNT THE REDS", "COUNT THE BLACKS", "IT WAS %d", "UP/DOWN, A: LOCK",
          "SMALL", "SMALL+", "MEDIUM", "LARGE", "HUGE"]
          # the INDEX sizes (TX_C_LOCK + 1..5)   # THE COUNT: the six asks, IT WAS, the lock hint   # the EQ presets (Synth.cpp), AUTO for SONG

# the six newer bonus stages (block 7; TX_N_* in Bonus.h): names, rule line 1, rule line 2, score line (format: wins, losses), then WATCH, YOUR TURN, HIT, MISS
# (the score lines carry each stage's goal and limit: keep them equal to MG in Minis.h)
MINI2 = ["SEANCE", "GALLOWS", "DEAD WHEEL", "RAVEN RAIN", "GHOULS", "THE COUNT",
         "REPEAT THE CANDLES", "A: STOP THE SKULL", "A: STOP ON GOLD", "< > CATCH THE ACES", "D-PAD + A: WHACK", "COUNT THE NAMED SUIT",
         "5 ROUNDS, 3 SLIPS", "5 HITS, 3 MISSES", "3 OF 5 SPINS", "15 ACES, 3 KNIVES", "16 HITS, 5 ESCAPE", "3 RIGHT, 2 WRONG",
         "ROUNDS %d/5 SLIPS %d/3", "HITS %d/5  MISSES %d/3", "GOLD %d/3  MISSES %d/3", "ACES %d/15  CUTS %d/3", "GHOULS %d/16  LOST %d/5", "RIGHT %d/3  WRONG %d/2",
         "WATCH...", "YOUR TURN", "HIT!", "MISS!"]

# the title menu and the bonus stage select (TX_T_* in Bonus.h, in this order)
UI = ["CONTINUE  R%d", "NEW RUN", "BONUS", "TURBO TERROR", "BONUS STAGES", "A: PLAY", "B: BACK",
      "MUSIC  %s", "EFFECTS  %s", "CARD SIZE  %s", "CORNERS  %s", "SONG  %d", "SONG  %s", "TONE  %s", "OFF", "ON", "FULL", "CALM",   # the pause menu (a setting's value picks the line: 0 OFF, 1 ON)
      "TIME!", "SCORE %ld", "BEST %ld", "NEW BEST!", "SURVIVED", "CAUGHT!", "PRESS A",                                  # after a timed play
      "RIGHT %d  WRONG %d", "YOU %d  DEALER %d", "PAIRS %d  MISSES %d/4",                                              # the stages' scores
      "RED RITE", "BLACKOUT", "DAMNED", "DEAD KINGS",                                                                   # the combos (Rules.h)
      "NIGHTMARE!", "A: ENTER", "B: FLEE", "%ld IN 20 S",                                                               # the bonus stage's show
      "A AGAIN TO QUIT", "SAVING IS OFF", "GAME SAVED", "NOT IN A TIMED PLAY",                                          # the pause menu's notes
      "A: DEEPER  B: FLEE", "DAWN AT LAST", "YOU DIED", "NIGHTS LIVED %d", "BEST %d", "A POKER ROGUELIKE", "COMBO: %s"]                                   # the survived and died screens

# the table's lines (TX_M_* in CHWildHand.ino, in this order): the hand names, the hex twists, the suits, the table's hints. Not a text block: they follow the
# sound tables and the wild cards' table in the RAM block read when the card is opened (Sounds.cpp miscText()), so the table never reads the card for them
MISC = ["HIGH CARD", "PAIR", "TWO PAIR", "THREE KIND", "STRAIGHT", "FLUSH", "FULL HOUSE", "FOUR KIND", "STR FLUSH",
        "DEAD SUIT", "BLINDFOLD", "SHORT LIFE", "NO ESCAPE", "FOUR CARDS", "FADING HANDS", "NO REPEATS", "HEARTS", "DIAMONDS", "CLUBS", "SPADES",
        "DN: LEVELS", "UP TO %d", "RUN!", "PAIRS OR BETTER!", "%s ARE DEAD", "HEX: %s", "? ? ?",
        "HEX %d", "NIGHT %d", "NIGHTMARE", "TURBO BEST %ld", "TIME", "HANDS", "DISC"]

# the shop's selling and the live-trigger lines (TX_Y_* in Gfx.h, in this order), block 6
SELL = ["YOUR CARDS", "SELL OR THROW?", "NOTHING TO SELL", "SELL $%d", "THROW", "CANCEL", "HALF PRICE BACK", "NOTHING BACK", "KEEP IT", "SELL ONE FIRST"]

# block 0: the wild cards' names, then SHOP; block 1: what the first JSPLIT wild cards do, then CALM; block 2: BONUS; block 3: MINI; block 4: UI;
# block 5: what the rest do; block 6: SELL; block 7: MINI2; block 8: DEAL2
# block 9: the COLORS page's lines; block 10: the options' names; block 11: the colours themselves (bytes, not text: tools/colours.py)
import colours
COLORS, COLVALS = colours.strings()[:2]
GROUPS = [[n for n, _ in JOKERS] + SHOP, [d for _, d in JOKERS[:JSPLIT]] + CALM, BONUS, MINI, UI, [d for _, d in JOKERS[JSPLIT:]], SELL, MINI2, DEAL2, COLORS, COLVALS,
          colours.data()[0]]
