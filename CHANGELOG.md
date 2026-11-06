# Changelog

## V1.3.13

- Hold LEFT or RIGHT to run along the hand and the shop: one step on the press, then a step every few frames.
- Room for more colour options (the colours' text is split over two blocks).

## V1.3.12

- The default highlight is now a dark red (the bone theme); the old gold is still in the HIGHLIGHT list.
- More colours: HIGHLIGHT gets CHAR, RED, ORANGE, WHITE, VIOLET and GOLD; ACCENT gets BLACK, RED, BLUE, ORANGE and GREY.
- BLK SUIT gets MONO ALL and PURE ALL: black (or near black) suits on every face, set by hand and not auto-lightened.
- Clearer pause-menu names: CARD SIZE, CORNERS, TONE, BLK SUIT, HIGHLIGHT, BACKDROP, BG SHOWN.

## V1.3.10 (first public release)

- The release build: the original songs only (ambient, dungeon synth and jazz), a smaller `WILDHAND.DAT`.
- Repository tidied for publishing; no change to the rules, art or backdrops.

## V1.3.9

- Animated backdrops: falling code, embers, rain, fireflies, falling cards and fog, a different one each night (pause menu
  BG: AUTO, SCENE or a fixed one; BG HOLD: STAGE, for the night only, or ALWAYS). EFFECTS CALM or OFF keep the area's motes.
- Two light card faces: LT GREY and PALE TEAL.
- Every THEME now has red and black suits (FOREST, WINE, HI-CON and RG SAFE changed).
- New upbeat jazz songs in the stage rotation and the pause menu's SONG choice.

## V1.3.8

- Card colours to mix and match (pause menu, scroll down): THEME (eight presets: BONE, NOIR, NAVY, FOREST, WINE and three
  colour-blind friendly ones, RG SAFE, BY SAFE, HI-CON), FACE (eleven card faces, light and dark), RED and BLACK (the suit
  colours), HILITE (the picked card's frame, the cursor glow, the menu bars) and ACCENT (card edges and backs).
  Every face and suit-colour mix stays readable (contrast 4.5:1 or better). THEME BONE puts everything back.
- The red suits on the bone face are a deeper red, for contrast.
- The pause menu has more room between lines and scrolls.

## V1.3.6

- Card colour themes: black with silver highlights plus several dark ones; bone stays the default.
- A pause-menu row to pick the card theme.
- Shop screen: UP / DOWN now move the selection.

## V1.3.5

- More horror: extra gore and blood stains, and faster, more intense music on a few tracks.
- An option to turn card effects off; with effects on, the card jiggle is randomised.
- Less lag moving left and right through the cards.
- Louder music and sound (synth gain).
- Settings are saved.

## 1.1 (horror)

- A new look: dark mouldy felt, bone cards, blood and candle amber instead of the casino colours. Rounds are NIGHTS
  and boss rounds are HEXES; the shop is a crypt, game over is YOU DIED, TURBO ARCADE is TURBO TERROR.
- Monster dealers replace the six cute ones: BEHOLDER, MIMIC, DRAGON, MIND FLAYER, CUBE and LICH, with the same edges
  (see the next card, +1 discard, +$1 a hand, a free pair, +$4 a win, +10 chips). They blink, drool and drip.
- Gore: blood stains on the table that dry and fade, blood sprays from big hands, discards and gibs, bone on a beaten
  hex, soaked table edges, blood running down the card backs, YOU DIED with creeping blood runs.
- Parallax depth on the table (fog in two bands, a lattice, near edges), the crypt shop (pillars and candles), the
  title (swinging chains) and the grave (ash at two depths).
- A bonus stage now opens each run and follows each hex night. The pairs stage became GRAVES, the snake box: the inside
  of a coffin with five snakes crawling at three depths. The other stages are THE CHASE, HIGH OR LOW and THE DUEL.
- Five card sizes (pause menu, CARD): NORMAL, SMALL, MEDIUM (default), LARGE, HUGE; old saves read MEDIUM.
- The 24 wild cards and the four combos have new names and pictures (effects unchanged); new dealer and shop lines.
- The original songs stream from `WILDHAND.DAT` on the SD card; the pause menu can pick a song and an
  equaliser setting.
- Fixes from review: the card cache, saves with out-of-range values, bonus prize text, one shop reaction per purchase,
  no 32-bit overflow with huge scores, QUIT TO TITLE asks twice.

## 1.0

- Six dealers (ROBO-7, OCTAVIA, DUSTY, MAGNUS, CAPN RUMBLE, PUFF), a different one each round, each with its own
  shuffle, deal, animations (blinks, head tilts, shifty looks) and an edge; they react to the excitement meter.
- Boss rounds every third round with a twist; hand levels (DOWN shows the table); 24 wild cards with four combos;
  endless mode after round 8; TURBO ARCADE on the title.
- Bonus stages after each boss: a card show and trick reveal the prize, then SPEED RUN, HIGH OR LOW, SHOWDOWN or
  PAIRS, with a real penalty for losing.
- New sounds for the gags, dealers and bonus stages; the music speeds up and adds hats as the table heats up.
- Pause menu: MUSIC and EFFECTS (FULL/CALM), both saved. The save is version 2; version 1 saves carry over.
- The pictures and music moved to WILDHAND.DAT on the SD card (the game shows NEED WILDHAND.DAT without it).
- Fixed: holding START could leave to the menu at once in the simulator (a millis() wrap in the hold timer).

## 0.1

First release: eight rounds with goals from 300 to 7,000, four plays and three discards a round, a shop with eight
wild cards (up to three owned at a time), a pause menu with save, and CONTINUE / NEW RUN on the title. Hold START for
three seconds to return to the CHGame menu.
