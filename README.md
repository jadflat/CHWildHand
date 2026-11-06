# Wild Hand

A horror poker roguelike for [CHGame](https://github.com/bateske/CHGame). You sit at a table of dark, mouldy felt
and the house deals with something that is not human. Each night a monster dealer deals you eight cards and you get
four plays and three discards. Pick up to five cards and play a poker hand: its chips times its multiplier add to
your score, and the table is soaked a little redder. Reach the night's goal, then spend your winnings in the crypt
shop on wild cards that bend the scoring. Survive eight nights to win the run, then keep going in endless mode.

![Wild Hand: the title, a monster dealer, a flush that draws blood, a hex night, the crypt shop and the snake box](docs/gameplay.gif)

It is inspired only by the general genre of poker score-attack games. Every line of code, picture and sound in this
repository is original.

## Needs the SD card file

The pictures and the songs live in `WILDHAND.DAT` on the SD card (copy `WILDHAND.DAT` from the release to the card's root,
next to the GAMES folder). The game streams its original songs and reads its pictures from that file as it
plays. Without it the game shows "NEED WILDHAND.DAT". The DAT and the game are built together: a DAT from another
build is refused, so copy both whenever either changes.

The pause menu's settings (music, effects, card and index size, song, EQ; not the colours) are also kept in the DAT's last 512-byte
block, so they survive another game taking the save page; copying a fresh DAT over the old one resets them.
A DAT from before this block still works: the settings then live in the save page only.

## Controls

| Control | Action |
|---|---|
| LEFT / RIGHT | Move the cursor along your hand (UP / DOWN move the selection in the shop) |
| A | Pick up or drop the card under the cursor (up to five) |
| B | Play the picked cards |
| UP | Discard the picked cards |
| DOWN | The hand levels table |
| START | Pause menu: resume, music, effects, card size, song, card colours, save game, quit to title |
| START (hold 3 s) | Back to the CHGame menu |

## Rules

- Eight nights, goals 300, 450, 700, 1,000, 1,600, 2,500, 4,000 and 7,000. After night 8 the run can go on
  (ENDLESS): each goal is half as big again as the one before.
- Each night starts with four plays and three discards. A hand scores its chips times its multiplier, plus the chips
  of each scoring card.
- Poker hands, weakest to strongest: high card, pair, two pair, three of a kind, straight, flush, full house,
  four of a kind, straight flush.
- **Hand levels:** every time you play a hand type it gains a level, adding chips and multiplier. Press DOWN during a
  night to see the table.
- **Hex nights:** every third night is a hex with a twist: a dead suit that does not score, a blindfold (cards dealt
  face down), a short life (only three hands), no escape (no discards), or at most four cards a hand. Beating a hex
  pays $5 more.
- **Blood:** a big hand sprays the table, every discard leaves a stain, and crossing the goal throws gore (and bone,
  on a hex). The stains dry from red to wine and fade. The table edges drip, and so do the dealers when they are
  winning.
- The run ends (YOU DIED) when you run out of plays before reaching the goal.

## The dealers

A different monster deals each night, each with a shuffle, a deal, a way of watching you and an edge of its own:

| Dealer | Edge |
|---|---|
| BEHOLDER | sees the next card of the deck |
| MIMIC | jaws: one more discard |
| DRAGON | +$1 for every hand played |
| MIND FLAYER | deals you a pair in your first hand |
| CUBE | digests: +$4 when you win the night |
| LICH | +10 chips on every hand |

They blink, twitch, drool and look at you; the hotter the table, the more shifty they get.

## Bonus stages: the snake box

A bonus stage opens every run and follows every hex night. The dealer shuffles and throws the cards in arcs and
circles, then a trick reveals the prize: CURSED GOLD (big money), a free wild card, or x2 on every hand of the next
night. Then one of four stages, which you may take on (A) or flee (B):

- **THE CHASE:** score the goal in 20 seconds; only pairs or better score.
- **HIGH OR LOW:** call higher or lower on five blind cards; four right wins.
- **THE DUEL:** pick a card from the dealer's fan, the dealer picks too; the higher card wins, best of three.
- **GRAVES (the snake box):** you are inside a coffin, twelve cards face down in the dark, snakes slithering past at
  three depths. Find all six pairs before the fifth miss.

Lose and half your money is gone, and you play the next night one hand short.

## Wild cards and combos

The crypt shop offers three of 24 wild cards (DEMON, BONES, BLEEDER, SKULL KING, THE EYE, DEATH KNELL, REVENANT,
RAVEN, GARGOYLE ...); you can hold three. Some pairs make combos when you own both: BLEEDER + BLOOD GEM (RED RITE: x2
if every scoring card is red), SEXTON + CUDGEL (BLACKOUT: x2 if all black), DEMON + BONES (DAMNED: +40 chips and +4
mult), SKULL KING + THE EYE (DEAD KINGS: x2 when a face and an ace score).

## Turbo terror

TURBO TERROR on the title: 60 seconds, two random wild cards, unlimited plays and discards. Each milestone adds five
seconds, a straight or better two. The best score is kept.

## Look and sound

- Dark mouldy felt, bone-white cards with blood-wine backs, candle-amber text; each area has its own light: a rotten
  title with swinging chains, a candlelit crypt shop, a blood-and-black hex table with a heartbeat, a nightmare swirl
  for bonus stages, a cold grave for YOU DIED with falling ash and a raven.
- Parallax depth: the table has far fog in two bands, a mid lattice that eases away from the cursor and soaked near
  edges; the shop's pillars and candles slide at different speeds; the grave's ash falls at two depths.
- Five card sizes (pause menu, CARD): NORMAL, SMALL, MEDIUM (the default), LARGE and HUGE.
- Original songs, played from the SD card file: ambient pads, dungeon synth and upbeat jazz; the music gets faster and adds hats as the table heats up.
- More horror: extra gore and blood stains on the table.
- Card colours to mix and match in the pause menu: eight themes (three of them colour-blind friendly) or the face, suit, highlight and accent colours one by one.
- Animated backdrops that change with each night: falling code, embers, rain, fireflies, falling cards and fog (pause menu, BG).

## Settings and saving

The pause menu has MUSIC (on/off), EFFECTS (FULL or CALM: no shakes or flashes, shorter gore, the excitement effects
at half; with effects on, the cards jiggle at random), the CARD size, a song and equaliser choice, the card colours, BG, SAVE GAME and QUIT TO TITLE (it asks twice). The settings are
saved.

The card colours: THEME sets them all at once (BONE, the default; NOIR, NAVY, FOREST, WINE; RG SAFE for red-green colour
blindness, red against pure black on white; BY SAFE for blue-yellow colour blindness, red against white; HI-CON, red and white on
black; every theme has red and black suits). Then each on its own: FACE (BONE, WHITE, BLACK, GREY, SLATE, NAVY, GREEN, TEAL, WINE, PURPLE, BROWN, LT GREY, PALE TEAL), RED (the
hearts and diamonds: RED, ORANGE, PINK, GOLD, VIOLET, BLUE), BLACK (the clubs and spades: MONO, PURE, BLUE, GREEN, BROWN,
CYAN), HILITE (the picked card's frame, the cursor and the menus: GOLD, SILVER, CYAN, LIME, PINK) and ACCENT (card edges
and backs: BRASS, STEEL, JADE, VIOLET). Each suit colour is lightened or darkened to suit the face, so every mix reads at
4.5:1 contrast or better. THEME BONE puts them all back.

BG picks the backdrop's drifting layer: AUTO (a different one each night), SCENE (the area's own motes), MATRX, EMBER,
RAIN, FLIES, CARDS or FOG. BG HOLD STAGE keeps a picked one for the night only (then AUTO again); ALWAYS keeps it.
EFFECTS CALM or OFF keep the area's own motes. The picked cards and the cursor card never shake. The game saves when a shop opens and from the pause menu; the
title offers CONTINUE when a save exists (with its money, wild cards and hand levels). Saves from earlier versions
carry over.

## Building

You need the CHGame board package and a CHGame board (or the simulator in `chgame/`). From this folder:

    python CHWildHand/tools/make_card.py   # WILDHAND.DAT and the headers it pairs with
    ./cl build                             # build
    ./cl upload                            # flash it to the board

`make_card.py` reads `Art.h` (`tools/art.py`), `Art2.h` (`tools/art2.py`) and `Music.h`, and writes
`dist/public/WILDHAND.DAT` (and `CHWildHand/sdcard/WILDHAND.DAT` for the simulator) plus `ArtCard.h`, `MusicCard.h` and `CardIndex.h`. Rerun it after any art or music change.

## Developer notes

- `CHWildHand.ino`: state and input; `Rules.h`: wild cards, combos, levels, hexes, endless, the timed plays;
  `Gfx.h`: drawing, gore and the hidden excitement meter; `Scene.h`: the backdrops and parallax; `Dealers.h`: the
  monster dealers; `Bonus.h`: the deal, bonus show, pause menu and other screens; `Minis.h`: the three card bonus
  stages and the snake box; `Card.cpp`: the SD card reader and its block cache.
- Simulator scripts are in `CHWildHand/tools/scripts/`, for example
  `./cl run --card CHWildHand/sdcard/WILDHAND.DAT CHWildHand/tools/scripts/v1tour.txt out/v1tour`.
  `./cl check` finds the card by itself. `python CHWildHand/tools/tests/sim_save.py` tests the save.
- Debug commands (`say` in a script): H heat, W win, O over, X shop, R round, M money, J wild cards, G bonus show,
  Z bonus stage (0-3; 99 starts a run without the opening stage), U turbo, D dealer, Y hex twist, I hand levels,
  F effects, C card size, E time left, V power cycle, A state line.
- `docs/` holds the box art (`cart.png`) and the GIF above.

## Licence

The game's code, art and songs are MIT, by Jadflat (see `LICENSE`). Built on [CHGame](https://github.com/bateske/CHGame) by
Kevin Bates (bateske), Apache-2.0, vendored unchanged in `chgame/`. See `THIRD-PARTY.md` and `NOTICE`.
