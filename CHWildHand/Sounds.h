// CHWildHand: the sound effects (piezo, library audio). Sounds.cpp plays them from the tables on the card.
#pragma once
#include <chgame/Audio.h>
#include "Synth.h"
#include "CardIndex.h"

enum class Sfx : uint8_t { Shuffle, Pick, Drop, Snap, Tick, Clear, Over, Buy, No, Reroll, Move, Flick, Win,
                        Deal, Whoosh, Alarm, Wah, Tada, Boing, Bleep, Blub, Whistle, Arr, Puff, Pop, Bwip, HeatUp, Beat, COUNT };
void playSfx(Sfx s, uint8_t semi);
void playSfx(Sfx s);                      // (semi 0: one argument less at every call)   // over the music (the synth's effect voice); the step tables are read from the card by openCard()
bool openCard();                                 // card::open(), then the effects into RAM
extern uint8_t wh_tabs[TAB_LEN];                 // the tables kept in RAM (tools/tables.py): T_RING, T_DL
const char *miscText(uint8_t i);                  // line i of the table's text (in RAM, with the sound tables)
const uint8_t *wildRec(uint8_t id);               // a wild card's record in RAM (see tools/jokers.py)
