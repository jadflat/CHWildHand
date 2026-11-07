// CHWildHand's sound (Synth.h), copied from CHix: a small software synth mixed in the sample interrupt and sent to the piezo as PWM.
//
// The hardware setup follows chgame-soundtracks (ESPboy-edu/ESPboy_CHCore, LIBS/chgame-soundtracks, BSD 3-Clause: see THIRD-PARTY.md).
// The hardware is the playbook's PWM-as-DAC recipe: TIM1 CH2 on PB10 is an 8-bit PWM carrier (187.5 kHz), and
// TIM3's update interrupt writes one sample to it SYN_RATE times a second. The music is a step sequencer over
// Music.h's streams: three 2-operator FM voices (a modulator at a whole-number ratio of the carrier, with its
// own decaying index: a bell, a pluck and a pad) and one drum voice (a sine that drops for the kick, noise for
// the snare and hats). The game's sound effects are one more voice, a square wave that plays its step tables.
#include "Synth.h"
#include <string.h>
#include "MusicCard.h"     // the songs' instruments and tempi; their notes are on the SD card (tools/make_card.py)
#if defined(__riscv) && !defined(CHSIM)
#include <Arduino.h>
#include <chgame/RamFunc.h>
#define SYN_DEVICE 1
#else
#define SYN_DEVICE 0
#define RAMFUNC(name)
#endif

namespace {

// the sine from its quarter (SINEQ, 0..64): the table is mirror-symmetric, so it reproduces the full 256-entry table exactly
static inline int sine(unsigned i) {
    i &= 255;
    int j = i & 127;
    if (j > 64) j = 128 - j;
    return (i & 128) ? -SINEQ[j] : SINEQ[j];
}

#ifndef SYN_GAIN
#define SYN_GAIN 28                                       // output gain: two voices at a steady level, peaks squashed by a soft knee
#endif
constexpr uint16_t ENV_DIV = SYN_RATE / 122;            // samples between envelope ticks (about 8 ms)

struct Voice { uint16_t ph, mph, inc, minc; uint8_t idx, eff, vol; bool gate; };       // eff: idx plus the sweep
__attribute__((section(".sbss.syn_v"))) Voice v[2];   // (the hot variables go first in RAM, inside the global pointer's reach: smaller, faster code)                                              // the tune and the bass (the third, an echo, is gone)

struct Drum { uint16_t ph, inc, lfsr; uint8_t vol, kind, decay; };      // kind 0 kick, 1 snare, 2 hat
__attribute__((section(".sdata.syn_dr"))) Drum dr = { 0, 0, 0xACE1, 0, 0, 0 };   // (small data: inside the global pointer's reach)
volatile uint8_t kickFlash = 0;
#ifdef WH_NOEQ
#define SYN_NOEQ 1                                       // (-DWH_NOEQ: no EQ, SOFT only, if the board's debug build runs out of flash)
#else
#define SYN_NOEQ 0
#endif
#if !SYN_NOEQ
struct Eq { uint8_t a; int8_t g; uint8_t s; };           // low-pass alpha /16, low shelf gain /8, level /16
const Eq EQ[synth::EQS] = {
    { 12, 0, 16 },   // SOFT: the original
    { 5, 0, 17 },    // MUFFLED
    { 9, 3, 13 },    // WARM
    { 7, 0, 17 },    // DARK
    { 14, 0, 16 },   // CRISP
    { 10, 6, 10 },   // BASS
    { 16, -7, 18 },  // TINNY
    { 7, -6, 20 },   // RADIO
    { 3, 2, 14 },    // CAVE
};
volatile uint8_t eqSel = 0;
#endif                          // set on each kick: the picture can pulse with the beat

struct Sfx { const audio::Step *st; uint8_t n, i, ticks, prio, amp, semi; bool soft; uint16_t inc, ph; int16_t dinc; };
__attribute__((section(".sbss.syn_s"))) Sfx sf = {};

// The song playing: its header comes from the card (make_card.py: the instruments, the step length in 2 ms, the echo delay, the chunks).
struct Hdr { Inst ins[3]; uint8_t step2, delay, chunks; };
__attribute__((section(".sbss.syn_h"))) Hdr hdr;
bool cur = false;                                        // a song is playing
// The song playing is streamed from the card: two 64-step chunks (lead, bass, drums: 64 bytes each), the one playing and the
// next. The game's main loop refills the other half (refill/filled) once the playhead is 16 steps into a chunk, so the arp,
// which reads the lead up to 15 steps back, still finds the end of the last chunk there. The interrupt never reads the card.
alignas(4) uint8_t songBuf[2][192];
uint8_t chunkIn[2];                                      // the chunk each half holds
enum { SONG_STRIDE = 8 };                                  // blocks a song takes on the card (make_card.py)
uint8_t denied;                                          // frames a refill has been put off
volatile int8_t needId = -1;                             // a song the game should load (its first and last chunk) and start
int8_t loadSong = -1;                                    // main loop: the song being loaded, its chunks loaded so far (0, 1)
uint8_t loadStage = 0;
uint8_t reqKind = 0, reqHalf, reqChunk;                  // main loop: the read in progress (0 none, 1 the next chunk, 2 a new song's)
int8_t reqSong;
const Inst *const ins = hdr.ins;                       // the instruments of the song playing (or the last one, for the release)
int8_t curId = -1, pendingId = -2;                       // the song playing, and the one fading in after it (-2: none)
uint8_t fade = 255;                                      // the music's master level
int8_t fadeDir = 0;                                      // -1 fading out, +1 fading in
volatile uint16_t pos = 0;                               // the step in the song, 0 to steps-1
uint16_t steps = 64;
uint16_t stepCnt = 0, stepLen = 0, envCnt = 0;
volatile uint8_t heatLv = 0;                             // the game's excitement: tempo and extra hats

inline uint16_t noteInc(uint8_t n) {
    const int o = n / 12 - 5;                           // C4 is octave 0
    const uint32_t b = NOTE_INC16[n % 12];
    return (uint16_t)(o >= 0 ? (b << o) >> 4 : b >> (4 - o));
}

void noteOn(int i, uint8_t n) {
    Voice &c = v[i];
    const Inst &in = ins[i];
    c.inc = noteInc(n);
    c.minc = (uint16_t)(c.inc * in.ratio);
    c.mph = 0;
    c.idx = in.idx0;
    c.eff = in.idx0;
    c.vol = in.amp;
    c.gate = true;
}

void drumOn(uint8_t bits) {
    if (bits & 1) { kickFlash = 3; dr.kind = 0; dr.inc = 1310; dr.vol = 96; dr.decay = 12; }              // 160 Hz, dropping: a soft thump
    else if (bits & 2) { dr.kind = 1; dr.inc = 1556; dr.vol = 150; dr.decay = 18; }
    else if (bits & 8) { dr.kind = 2; dr.vol = 80; dr.decay = 7; }
    else if (bits & 4) { dr.kind = 2; dr.vol = 84; dr.decay = 26; }
}

void startSong(int song) {
    for (int i = 0; i < 2; i++) { v[i].vol = 0; v[i].gate = false; }     // the last song's notes and drums do not carry into this one
    dr.vol = 0; kickFlash = 0;
    if (song < 0 || song >= SYN_SONGS) { cur = false; curId = -1; return; }
    cur = true;                                          // (hdr and songBuf hold its header and notes)
    curId = (int8_t)song;
    stepLen = (uint16_t)(hdr.step2 * (2 * SYN_RATE / 1000));
    stepCnt = 1;
    pos = 0;
    steps = (uint16_t)(hdr.chunks * 64);
}

void nextStep() {
    const uint16_t p = pos;
    const uint8_t *h = songBuf[(p >> 6) & 1] + (p & 63);
    for (int i = 0; i < 2; i++) {
        const uint8_t b = h[i * 64];
        if (b == 0) v[i].gate = false;
        else if (b > 1) noteOn(i, b);
    }
    if (h[128]) drumOn(h[128]);
    pos = p + 1 < steps ? p + 1 : 0;
}

void sfxStep() {
    if (sf.i >= sf.n) { sf.n = 0; sf.amp = 0; sf.prio = 0; return; }
    const audio::Step &s = sf.st[sf.i];
    const uint16_t inc = (uint16_t)((s.hz * 164u * SEMI_Q8[sf.semi]) >> 8);       // 20 Hz units to a phase increment at SYN_RATE, raised
    sf.ticks = (uint8_t)((s.ms * 62u >> 8) + 1);
    sf.dinc = s.endHz ? (int16_t)(((int)((s.endHz * 164u * SEMI_Q8[sf.semi]) >> 8) - (int)inc) / sf.ticks) : 0;
    sf.inc = inc;
    sf.amp = s.hz ? (sf.soft ? 44 : 104) : 0;
}

void envTick() {
    if (fadeDir < 0) {
        if (fade > 4) fade -= 4;
        else {
            fade = 0;
            if (pendingId != -2) { startSong(-1); needId = pendingId; pendingId = -2; } fadeDir = 0;   // the game loads the next one
        }
    } else if (fadeDir > 0) {
        if (fade < 251) fade += 4; else { fade = 255; fadeDir = 0; }
    }
    for (int i = 0; i < 2; i++) {
        Voice &c = v[i];
        const Inst &in = ins[i];
        if (!c.vol) continue;
        if (c.idx > in.idxFloor + in.idxDecay) c.idx -= in.idxDecay; else c.idx = in.idxFloor;
        c.eff = c.idx;
        if (c.gate) {
            if (c.vol > in.volFloor + in.volDecay) c.vol -= in.volDecay; else c.vol = in.volFloor;
        } else {
            c.vol = c.vol > 14 ? c.vol - 14 : 0;                                              // released
        }
    }
    if (dr.vol) {
        if (dr.kind == 0 && dr.inc > 360) dr.inc -= dr.inc >> 2;                              // the kick's pitch drops
        dr.vol = dr.vol > dr.decay ? dr.vol - dr.decay : 0;
    }
    if (sf.n) {
        sf.inc += sf.dinc;
        if (--sf.ticks == 0) { sf.i++; sfxStep(); }
    }
}

}  // namespace

namespace synth {

uint8_t sample() {                                       // runs from flash: the RAM for a RAM function is not there
#if SYN_DEVICE
    TIM1->CCER = (cur || sf.n || dr.vol || v[0].vol || v[1].vol) ? 0x10 : 0;   // the carrier is off while nothing sounds (CHStarlift's idle-whine fix: a running carrier at mid duty whines in the piezo and in the emulators)
#endif
    if (++envCnt >= ENV_DIV) { envCnt = 0; envTick(); }
    if (cur && --stepCnt == 0) { stepCnt = (uint16_t)(stepLen - ((stepLen * heatLv) >> 10)); nextStep(); }   // up to 25% faster
    int mix = 0;
    for (int i = 0; i < 2; i++) {
        Voice &c = v[i];
        if (!c.vol) continue;
        c.ph += c.inc;
        c.mph += c.minc;
        const int m = (sine(c.mph >> 8) * c.eff) >> 5;                  // the modulator bends the carrier's phase
        mix += (sine((uint8_t)((c.ph >> 8) + m)) * c.vol) >> 7;
    }
    if (dr.vol) {
        dr.ph += dr.inc;
        if (dr.kind) {
            uint16_t x = dr.lfsr;
            x ^= x << 7; x ^= x >> 9; x ^= x << 8;
            dr.lfsr = x;
            const int nz = (int)(x >> 8) - 128;
            mix += (nz * dr.vol) >> 7;
            if (dr.kind == 1) mix += (sine(dr.ph >> 8) * dr.vol) >> 7;
        } else {
            mix += (sine(dr.ph >> 8) * dr.vol) >> 8;
        }
    }
    if (fade < 255) mix = (mix * fade) >> 8;                           // the crossfade (the effects stay at full level)
    if (sf.amp) {
        sf.ph += sf.inc;
        mix += (sf.ph & 0x8000) ? sf.amp : -sf.amp;
    }
    mix = (mix * SYN_GAIN) >> 4;                                          // the master volume (16 = as mixed)
    if (mix > 100) mix = 100 + ((mix - 100) >> 1); else if (mix < -100) mix = -100 + ((mix + 100) >> 1);   // a soft knee (CHix's): peaks squash, they do not clip
    // The EQ (EQ table): a one-pole low-pass (alpha a/16, 8 kHz; SOFT's 12/16 takes the edge off the top, the clip's harmonics and the
    // noise hiss), a slow one-pole (about 170 Hz) added back g/8 times (g > 0 lifts the lows, g < 0 cuts them), and a level s/16.
    static int lp;
#if SYN_NOEQ
    lp += (((mix > 127 ? 127 : mix < -128 ? -128 : mix) << 4) - lp) * 3 >> 2;
    return (uint8_t)(128 + (lp >> 4));
#else
    static int lo;
    const Eq &q = EQ[eqSel];
    lp += (((mix > 127 ? 127 : mix < -128 ? -128 : mix) << 4) - lp) * q.a >> 4;
    lo += (lp - lo) >> 3;
    int o = (lp + (lo * q.g >> 3)) * q.s >> 4;
    return (uint8_t)(128 + ((o > 2047 ? 2047 : o < -2048 ? -2048 : o) >> 4));
#endif
}

#if SYN_DEVICE
#define SYN_LOCK() __disable_irq()
#define SYN_UNLOCK() __enable_irq()
#else
#define SYN_LOCK()
#define SYN_UNLOCK()
#endif

void music(int song) {
    const int id = (song < 0 || song >= SYN_SONGS) ? -1 : song;
    SYN_LOCK();
    if (!cur) {                                          // nothing playing: the game loads it from the card, then it fades in
        needId = (int8_t)id;
        pendingId = -2;
    } else if (id == curId) {                            // asked for the song that is playing: keep it (and cancel a fade away from it)
        pendingId = -2;
        if (fadeDir < 0) fadeDir = 1;
    } else {                                             // another song (or silence): fade this one out first
        pendingId = (int8_t)id;
        fadeDir = -1;
    }
    SYN_UNLOCK();
}

void heat(uint8_t level) { heatLv = level; }
#if !SYN_NOEQ
void eq(uint8_t preset) { eqSel = preset < EQS ? preset : 0; }
#else
void eq(uint8_t) {}
#endif

bool refill(uint32_t *off, uint8_t **dst, bool busy) {
    SYN_LOCK();
    reqKind = 0;
    const int8_t want = needId;
    if (want >= 0) {                                     // a new song: its header (the block's last 192 B, into half 1), chunk 0 into half 0, its last chunk into half 1 (the arp's first notes)
        if (loadSong != want) { loadSong = want; loadStage = 0; }
        reqKind = 2; reqSong = want; reqHalf = loadStage != 1; reqChunk = loadStage ? loadStage == 1 ? 0 : (uint8_t)(hdr.chunks - 1) : 0xFF;
    } else if (cur) {                                    // playing: the next chunk into the other half (when the frame has the time: it has 48+ steps)
        const uint16_t p = pos;
        const uint8_t k = (uint8_t)(p >> 6), nx = k + 1 < hdr.chunks ? k + 1 : 0, other = (k & 1) ^ 1;
        if ((p & 63) >= 16 && chunkIn[other] != nx) {
            if (busy && denied < 40) denied++;           // a busy frame waits (up to 40 frames, about 1.3 s: the other half is needed 48 steps later at least)
            else { denied = 0; reqKind = 1; reqSong = curId; reqHalf = other; reqChunk = nx; }
        }
    }
    SYN_UNLOCK();
    if (!reqKind) return false;
    *off = reqChunk == 0xFF ? (uint32_t)reqSong * (SONG_STRIDE * 512u) + 320u    // (the header: 320..512 of the song's first block)
         : ((uint32_t)reqSong * SONG_STRIDE + (reqChunk >> 1)) * 512u + (reqChunk & 1) * 192u;
    *dst = songBuf[reqHalf];
    return true;
}

void filled() {
    SYN_LOCK();
    if (reqKind == 1 && cur && curId == reqSong) chunkIn[reqHalf] = reqChunk;
    else if (reqKind == 2 && needId == reqSong && loadSong == reqSong) {
        if (loadStage == 0) { memcpy(&hdr, songBuf[1] + 64, sizeof hdr); loadStage = 1; }
        else if (loadStage == 1) loadStage = 2;
        else {
            startSong(reqSong);
            chunkIn[0] = 0; chunkIn[1] = reqChunk;
            fade = 0; fadeDir = 1;
            needId = -1; loadSong = -1;
        }
    }
    reqKind = 0;
    SYN_UNLOCK();
}

uint8_t beat() {                                          // once a frame: nonzero for a few frames after each kick
    const uint8_t k = kickFlash;
    if (k) kickFlash = k - 1;
    return k;
}

void sfx(const audio::Effect &e, uint8_t semi) {
    SYN_LOCK();
    const uint8_t prio = e.flags & 15;
    if (!sf.n || prio >= sf.prio) {
        sf.st = e.steps; sf.n = e.n; sf.i = 0; sf.prio = prio; sf.soft = (e.flags & audio::SOFT) != 0; sf.semi = semi > 12 ? 12 : semi;
        sf.ph = 0;
        sfxStep();
    }
    SYN_UNLOCK();
}

}  // namespace synth

#if SYN_DEVICE
extern "C" volatile uint32_t CFGHR_tmpB;
extern "C" void TIM3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
extern "C" void TIM3_IRQHandler(void) {
    TIM3->INTFR = ~1u;
    TIM1->CH2CVR = (uint16_t)(synth::sample() * 125u >> 7);   // 0-255 onto the 250-tick carrier (0-249)
}

void synth::begin() {
    RCC->APB2PCENR |= RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB | RCC_APB2Periph_TIM1;
    RCC->APB1PCENR |= RCC_APB1Periph_TIM3;
    AFIO->PCFR1 = (AFIO->PCFR1 & ~(7u << 15)) | (1u << 15);          // TIM1 partial remap: CH2 on PB10
    CFGHR_tmpB = (CFGHR_tmpB & ~(15u << 8)) | (11u << 8);            // alternate-function push-pull
    GPIOB->CFGHR = CFGHR_tmpB;
    TIM1->CTLR1 = 0; TIM1->DMAINTENR = 0; TIM1->CCER = 0;
    TIM1->PSC = 0; TIM1->ATRLR = 249; TIM1->CNT = 0;   // 250 ticks: 192 kHz, exactly 4 a 48 kHz audio sample (256 ticks beat to a 4.5 kHz whine in the emulators) and 24 a synth sample (SYN_RATE 8000)
    TIM1->CHCTLR1 = 0x6800; TIM1->CH2CVR = 125;
    TIM1->RPTCR = 0; TIM1->BDTR = 0x8000; TIM1->CCER = 0;            // carrier off until a sound starts
    TIM1->SWEVGR = 1; TIM1->CTLR1 = 0x81;                            // 192 kHz carrier
    TIM3->CTLR1 = 0; TIM3->DMAINTENR = 0;
    TIM3->PSC = 0; TIM3->ATRLR = 48000000u / SYN_RATE - 1; TIM3->CNT = 0;
    TIM3->SWEVGR = 1; TIM3->INTFR = 0; TIM3->DMAINTENR = 1;
    NVIC_SetPriority(TIM3_IRQn, 0);
    NVIC_EnableIRQ(TIM3_IRQn);
    TIM3->CTLR1 = 1;
}
#else
void synth::begin() {}
#endif
