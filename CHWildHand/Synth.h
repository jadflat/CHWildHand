// CHWildHand's sound (from CHix): a small software synth. The music is two FM voices (a lead and a bass, plus an
// optional echo of the lead) and a drum voice, read from step songs (Music.h, tools/make_music.py); the sound
// effects are a square voice playing Sounds.cpp's step tables over the music. Mixed in a timer interrupt and
// sent to the piezo as PWM: replaces the library's audio::begin/update (same timers).
#pragma once
#include <chgame/Audio.h>

namespace synth {

void begin();                                           // device: starts the sample interrupt (the simulator is silent)
void music(int song);                                   // 0 title, 1 round, 2 shop, 3 end; -1 stops (fades out)
enum { EQS = 9 };                                       // the EQ presets (Synth.cpp), 0 = SOFT
void eq(uint8_t preset);                                // the output filter
void heat(uint8_t level);                              // the excitement meter (0-255): the music speeds up (to 1.25x) and adds hats
// The songs' notes are on the SD card, streamed 64 steps at a time: once a frame, after gfx_wait(), the game asks
// refill(); when true it reads 192 bytes at CARD_SONG + *off into *dst and calls filled(). A new song starts (fading
// in) after its first three reads (its header, chunk 0 and its last chunk).
bool refill(uint32_t *off, uint8_t **dst, bool busy = false);   // busy: the last frame was heavy; a refill that is not urgent waits
void filled();
uint8_t beat();                                         // once a frame: nonzero for a few frames after each kick
void sfx(const audio::Effect &e, uint8_t semi = 0);     // an effect, raised 0-12 semitones; a higher or equal priority replaces it
uint8_t sample();                                       // one 8-bit sample (the interrupt; the PC renderer)

}  // namespace synth
