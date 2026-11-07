// CHWildHand: a poker roguelike for CHGame.
//
// Original C++ code, in the spirit of the "poker as a score attack" genre. Each round a dealer deals you 8 cards;
// you get four plays and three discards: pick up to five cards and play a poker hand. Chips x mult adds to your
// score; reach the round's target to win it, then spend your winnings on wild cards (up to 3) that bend the scoring.
// Every third round is a boss round with a twist, beaten bosses open a timed bonus stage, hands level up as you play
// them, and after round 8 the run can go on, endless. The title also offers a 60-second TURBO arcade.
#include <CHGame.h>
#pragma GCC optimize("no-jump-tables","no-caller-saves","reorder-blocks","reorder-blocks-algorithm=stc","no-tree-loop-im","no-schedule-insns2","no-move-loop-invariants","no-lra-remat","no-tree-forwprop","no-tree-dominator-opts","no-shrink-wrap","no-ira-hoist-pressure","no-ipa-pure-const","no-partial-inlining")   // (the last seven: measured smaller)

enum { HANDN = 8, JMAX = 5, ROUNDS = 8, PLAYS = 4, DISCS = 3 };
enum State : uint8_t { TITLE, PLAY, RESULT, SHOP, OVER, WON, PAUSED, DEAL, BONUS, TIMEUP, MINI, BSEL };

static const uint16_t TARGET[ROUNDS] = { 520, 900, 1000, 2000, 3000, 3800, 5500, 8000 };
#define HCHIPS ((const uint8_t *)(wh_tabs + T_HCHIPS))   // (in RAM, from the card: tools/tables.py)
#define HMULT ((const uint8_t *)(wh_tabs + T_HMULT))   // (in RAM, from the card: tools/tables.py)

// text block 4 (tools/card_text.py UI): the title's lines, the stage select, the pause menu, the end of a timed play, the scores, the combos, the bonus show
enum { TX_T_CONT, TX_T_NEW, TX_T_BONUS, TX_T_TURBO, TX_T_STAGES, TX_T_PLAY, TX_T_BACK,
       TX_P_MUSIC, TX_P_FX, TX_P_CARD, TX_P_INDEX, TX_P_SONGN, TX_P_SONG, TX_P_EQ, TX_POFF, TX_PON, TX_FULL2, TX_CALM2,
       TX_U_TIME, TX_U_SCORE, TX_U_BEST, TX_U_NEWBEST, TX_U_SURV, TX_U_CAUGHT, TX_U_PRESS,
       TX_S_HILO, TX_S_DUEL, TX_S_PAIRS, TX_COMBO, TX_B_NIGHT = TX_COMBO + 4, TX_B_ENTER, TX_B_FLEE, TX_B_SPEED,
       TX_N_QUIT, TX_N_OFF, TX_N_SAVED, TX_N_TIMED, TX_E_DEEPER, TX_E_DAWN, TX_E_DIED, TX_E_LIVED, TX_E_BEST, TX_T_TAG, TX_COMBOF };
enum { TX_Y_YOURS, TX_Y_SURE, TX_Y_NONE, TX_Y_SELL, TX_Y_THROW, TX_Y_CANCEL, TX_Y_HALF, TX_Y_NOTHING, TX_Y_KEEP, TX_Y_FIRST };   // text block 6 (card_text.py SELL): the shop's selling lines
// the table's lines (card_text.py MISC, kept in RAM with the sound tables: miscText()): the hand names, the hex twists, the suits, the table's hints, the top bar's words
enum { TX_M_HAND = 0, TX_M_BOSS = 9, TX_M_SUIT = 16, TX_M_LEVELS = 20, TX_M_PICK, TX_M_RUN, TX_M_PAIRS, TX_M_DEAD, TX_M_HEXC, TX_M_Q, TX_M_HEX, TX_M_NIGHT, TX_M_NM, TX_M_TURBO, TX_M_TIME, TX_M_HANDS, TX_M_DISC };
// the notes (a shop reaction, or a line under the pause menu): codes, not strings
enum { N_NONE, N_BOUGHT, N_NOCASH, N_FULL, N_QUIT, N_OFF, N_SAVED, N_TIMED };
static const char *cardText(uint8_t g, uint8_t i);

#include "Sounds.h"     // the piezo effects (Sounds.cpp)
#include "Fmt.h"        // snprintf stand-in
// PERF_BUILD: the board's debug build (the perf tool) has no room for the save code; the perf scripts never save or
// continue, so it is left out there (the simulator's debug build keeps it for the save tests).
#if CHGAME_DEBUG && !defined(CHSIM)
#define PERF_BUILD 1
// PERF_LEAN (defined: with everything in the board's debug build is about 51,930 B, over its 50,944; with the cuts it is about 50,850): the cosmetic cuts the debug build makes (a raven,
// the dark eyes, the plain rank/face stand-ins, the INDEX/SONG/EQ lines, the title's BONUS and its stage select). If later work pushes it over again, Synth.cpp's
// WH_NOEQ (the EQ presets) brings it down by another 120 B or so (define it for the debug build there).
#define PERF_LEAN 1
#endif
// the newer native bonus stages (Minis.h), one switch each (1 in, 0 out); NEW_STAGES: any of them in (the shared parts: the
// list's scroll, the score line, the result pause: NEW_SCAFFOLD). The board's debug build has no room for them and never picks them.
#ifdef PERF_BUILD
#define NEW_RAVEN 0
#define NEW_GHOULS 0
#define NEW_COUNT 0
#define NEW_VM 0
#endif
#ifndef NEW_RAVEN
#define NEW_RAVEN 0        // off: no room with the USB upload in (q1_out/report.md)
#endif
#ifndef NEW_GHOULS
#define NEW_GHOULS 0        // off: no room with the USB upload in (q1_out/report.md)
#endif
#ifndef NEW_COUNT
#define NEW_COUNT 1
#endif
// NEW_VM: the stages that are scripts on the SD card (StageVM.h, tools/stages: SEANCE, GALLOWS, DEAD WHEEL, THE WATCHER), all of them or none
#ifndef NEW_VM
#define NEW_VM 1
#endif
#define NEW_STAGES (NEW_RAVEN + NEW_GHOULS + NEW_COUNT + NEW_VM)
#ifndef NEW_SCAFFOLD
#define NEW_SCAFFOLD (NEW_STAGES > 0)                   // the shared parts (defined 1 with no stage only to measure them)
#endif

struct Card { uint8_t r, s; };                       // rank 0..12 (2..A), suit 0..3 (H D C S)
static Card deck[52], hand[HANDN];
static uint8_t deckPos, nHand;
static bool sel[HANDN];
static uint8_t cur;

static State state;
static uint8_t round_, playsLeft, discLeft, jokers[JMAX], nJok, shopOffer[4];
static uint8_t shopMode, ownSel, confSel;   // shopMode: 0 the offers, 1 your cards, 2 sell or throw
static int32_t score, money, hiRound;
static uint16_t timer;
static uint32_t rng = 2463534242u;
static State lastSt = TITLE;   // (the state the frame events last saw: Gfx.h stateEvents)
static uint8_t pauseSel, titleSel, shopSel, bselSel, pauseFrom;   // pauseFrom: the state the pause menu returns to (START works in every state)
static bool bonusPractice;                              // the title's BONUS option: a bonus stage played alone, no run (the stage select is state BSEL)
// the pause menu's settings (saved), in its order (lines 1..12): MUSIC, EFFECTS (0 FULL, 1 CALM, 2 OFF), CARD (0 NORMAL, 1 SMALL, 2 MEDIUM, 3 LARGE,
// 4 HUGE: Gfx.h drawFaceBig), INDEX (0 SMALL, 1 SMALL+, 2 MEDIUM, 3 LARGE, 4 HUGE: the rank and suit), SONG (0 AUTO, 1.. a fixed stage song), EQ (the
// preset), then the COLORS options cop[]: THEME (the preset last picked), FACE, RED, BLACK, HILITE, ACCENT (tools/colours.py; 0 = the bone default),
// then BG and BG HOLD (lines 13, 14).
// loadSave() sets them (an empty save reads MUSIC on, CARD MEDIUM)
#ifdef PERF_BUILD
static uint8_t sett[14] = { 1, 0, 2 };
#else
static uint8_t sett[14];
#endif
#define musicOn sett[0]
#define fxLow sett[1]
#define cardSz sett[2]
#define idxSz sett[3]
#define songSel sett[4]
#define eqSel sett[5]
#define cop (sett + 6)
#define bgSel sett[12]                                   // BG: 0 AUTO, 1 SCENE, 2.. a style (Scene.h sceneFg); BG HOLD: 0 STAGE (a pick lasts the night), 1 ALWAYS
#define bgHold sett[13]
static bool saveDue, saveFlag, hasSave;
static uint8_t cardDue;                                   // frames until the settings go to the card too (card::putSave; each change restarts the count)
static uint8_t note;
static uint8_t moodT;                                  // the shop reacted to this note (Gfx.h); cleared by a press

// result of the last play
static uint8_t lastType, lastN;
static int lastChips, lastMult;
static int32_t lastGain;
static bool lastScoring[5];
static Card lastCards[5];

// the save: version 2 adds the hand levels, the turbo best, the dealer order and the two settings (version 1 is read too). Five wild cards: the
// slots 4 and 5 sit in jok2 (the old struct's padding, so old saves load as they were), each slot's counter in the top nibble of lv[slot]
struct SaveData { int32_t money, hi, turboHi; uint8_t round_, nJok, jok[3], lv[9], music, fx, dealerBase, pad, jok2[2]; };
static_assert(sizeof(SaveData) == 32, "the save keeps its size");
static const uint32_t MAGIC = save::magic("WLDH");
static SaveData sv;

static uint32_t rnd() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

static void shuffleCards(Card *a, int n) { for (int i = n - 1; i > 0; i--) { int j = rnd() % (i + 1); Card t = a[i]; a[i] = a[j]; a[j] = t; } }   // (the deck, and PAIRS' twelve: Minis.h)
static void shuffleDeck() {
    for (int i = 0; i < 52; i++) { deck[i].r = i % 13; deck[i].s = i / 13; }
    shuffleCards(deck, 52);
    deckPos = 0;
}

// ---- scoring
static int rankChips(uint8_t r) { return r == 12 ? 11 : (r >= 9 ? 10 : r + 2); }

static void evaluate(const Card *c, int n, uint8_t &type, bool *scoring) {
    uint8_t sm[5];                                       // how many cards share each card's rank
    int d = -n, lo = 99, mask = 0, same = 0, b = 0;       // d: the sum of those counts less n: 0 none, 2 a pair, 4 two pairs, 6 trips, 8 a full house, 12 quads
    for (int i = 0; i < n; i++) {
        int k = 0;
        for (int j = 0; j < n; j++) k += c[j].r == c[i].r;
        sm[i] = (uint8_t)k;
        d += k;
        mask |= 1 << c[i].r;
        if (c[i].r < lo) lo = c[i].r;
        same += c[i].s == c[0].s;
        if (c[i].r > c[b].r) b = i;                      // (the highest card: c[b].r is the top rank)
    }
    const bool flush = n == 5 && same == 5;
    const bool straight = n == 5 && !d && (c[b].r - lo == 4 || mask == 0x100F);   // (0x100F: the wheel, A 2 3 4 5)
    d >>= 1;
    type = flush ? (straight ? 8 : 5) : straight ? 4 : d > 3 ? d / 2 + 4 : d;   // (d / 2: 1 pair, 2 two pairs, 3 trips, 4 full house -> 6, 6 quads -> 7)
    for (int i = 0; i < n; i++) scoring[i] = flush || straight || sm[i] >= 2;   // (a full house: every card is in a group)
    if (!type) scoring[b] = true;
}

static bool hasJoker(uint8_t j) { for (int i = 0; i < nJok; i++) if (jokers[i] == j) return true; return false; }

#include "Rules.h"      // wild cards, combos, levels, bosses, endless, dealers' edges, timed plays

#define kStageSong ((const uint8_t *)(wh_tabs + T_KSONG))      // the stage songs (in Music.h); a round plays kStageSong[(round + run) % NSTAGE], or the SONG option
#ifdef PERF_LEAN
static int stageSong() { return kStageSong[round_ % NSTAGE]; }
#else
static int stageSong() { return kStageSong[songSel ? songSel - 1 : (round_ + runOff) % NSTAGE]; }
#endif
#define PACKFX() ((uint8_t)((fxLow & 1) | (cardSz + 3) % 5 << 1 | runOff << 4))
#define PACKMU() ((uint8_t)(musicOn | idxSz << 1 | (fxLow >> 1) << 4 | cop[0] << 5))      // sv.music bits 1-3: the INDEX size (old saves: 0 = SMALL), bits 5-7: the colour THEME
#define HIPACK() (hiRound | colPack() << 8)          // sv.hi: the best night (bits 0-7), the five COLORS options a nibble each above it, BG and BG HOLD in bits 28-31 (old saves: 0, the defaults)
static uint32_t colPack() { uint32_t p = bgSel | bgHold << 3; for (int k = 5; k; k--) p = p << 4 | cop[k]; return p; }
#define PACKSE() ((uint8_t)(songSel | eqSel << 4))

#ifdef PERF_BUILD
static void fillSave() {}
static void saveRun() {}
static void endSave() {}
static void saveSettings() {}
#else
__attribute__((noinline)) static void packSettings() { sv.music = PACKMU(); sv.fx = PACKFX(); sv.pad = PACKSE(); sv.hi = HIPACK(); }
static void fillSave() {
    sv.money = money; sv.turboHi = turboHi; sv.round_ = round_; sv.nJok = nJok;
    for (int i = 0; i < JMAX; i++) (i < 3 ? sv.jok[i] : sv.jok2[i - 3]) = jokers[i];
    for (int i = 0; i < 9; i++) sv.lv[i] = (uint8_t)(hlv[i] | (i < JMAX ? cnt[i] << 4 : 0));
    packSettings(); sv.dealerBase = dealerBase;
}
__attribute__((noinline)) static void saveGo(bool f) { saveFlag = f; saveDue = true; cardDue = 20; }
static void saveRun() { fillSave(); saveGo(true); }
static void endSave() { fillSave(); saveGo(false); }
static void saveSettings() { packSettings(); sv.turboHi = turboHi; saveGo(hasSave); }
#endif

static void shopRoll() {                                 // three offers, four on the night after a hex is beaten: none owned, none twice
    shopN = round_ && round_ % 3 == 0 && mode == M_RUN ? 4 : 3;
    for (int i = 0; i < shopN; i++) {
        uint8_t id;
        bool dup;
        do { id = rnd() % NJOK; dup = hasJoker(id); for (int k = 0; k < i; k++) dup |= id == shopOffer[k]; } while (dup);
        shopOffer[i] = id;
    }
}
static void runOn() { mode = M_RUN; timed = bonusOn = false; }
static void openShop() {
    timed = bonusOn = false;
    shopRoll();
    shopMode = 0; if (shopSel > shopN + 1) shopSel = shopN + 1;   // the cursor stays where it was last visit
    state = SHOP; note = 0;
    saveRun();
}

static void removeSelected(int &count, Card *out) {
    count = 0;
    int w = 0;
    for (int i = 0; i < nHand; i++)                     // (one pass: w <= i, so hand[i] is read before anything is written over it)
        if (sel[i]) out[count++] = hand[i];
        else { hidden[w] = hidden[i]; hand[w++] = hand[i]; }
    pickSeen = 0;
    nHand = w;
    for (int i = 0; i < HANDN; i++) sel[i] = false;
    refill();
    if (cur >= nHand) cur = nHand ? nHand - 1 : 0;
}
static int selCount() { int n = 0; for (int i = 0; i < nHand; i++) if (sel[i]) n++; return n; }

static void continueRun() {
    runOn();
    round_ = sv.round_ > 99 ? 99 : sv.round_; money = sv.money; dealerBase = sv.dealerBase % NDEAL;
    clearJok(); nJok = sv.nJok > JMAX ? JMAX : sv.nJok;
    for (int i = 0; i < nJok; i++) { jokers[i] = (i < 3 ? sv.jok[i] : sv.jok2[i - 3]) % NJOK; cnt[i] = sv.lv[i] >> 4; }
    for (int i = 0; i < 9; i++) hlv[i] = (sv.lv[i] & 15) > LVMAX ? LVMAX : sv.lv[i] & 15;
    startRound();
}
// the title's lines (card text block 4): CONTINUE (with a save), NEW RUN, BONUS (not in the board's debug build), TURBO; the line's id k is 0..3
static int titleN() {
#ifdef PERF_LEAN
    return 2;
#else
    return 4 + hasSave;
#endif
}
static int titleK(int sel) {
    int k = sel + !hasSave;
#ifdef PERF_LEAN
    if (k > 1) k++;
#endif
    return k;
}
#ifndef PERF_LEAN
// the title's BONUS: the stage select (BSEL), then a bonus stage played alone as practice: no run, no save, no shop
static void practiceBack() { runOn(); boss = 255; x2Pend = penalty = false; note = 0; state = BSEL; if (bselSel == 255) { bonusPractice = 0; state = TITLE; } }   // (bselSel 255: the title's BLACKJACK, back to the title)
static void practiceStart(uint8_t k) {
    freshRun(M_RUN, 3, 4); dealer = rnd() % NDEAL;
    forceMini = k; startBonus(); forceMini = -1;
}
#endif
#ifdef PERF_BUILD
#define CONTINUE_RUN() ((void)0)                         // (the perf build has no saves, so no CONTINUE line)
#else
#define CONTINUE_RUN() continueRun()
#endif
static void dealerReact(bool good);
__attribute__((noinline)) static void toTitle() { state = TITLE; titleSel = 0; runOn(); }   // (a run or the turbo is over)
static void loadSave();
static void miniUpdate(bool up, bool dn, bool lf, bool rt, bool a);

// a menu cursor: left/right (or up/down) step a selection within 0..mx with a blip (wrapNav: round the ends of n lines)
static void nav(uint8_t &v, int mx, bool lo, bool hi) {
    const int n = v - (lo && v > 0) + (hi && (int)v < mx);
    if (n != v) { v = n; playSfx(Sfx::Move); }
}
static void wrapNav(uint8_t &v, int n, bool lo, bool hi) {
    if (lo) v = v ? v - 1 : n - 1;
    if (hi) v = v + 1 < n ? v + 1 : 0;
    if (lo || hi) playSfx(Sfx::Move);
}

// the pause menu's settings 1..6: MUSIC, EFFECTS, CARD, INDEX, SONG, EQ (a value and how many it has; left and right step it round)
#define OPTM (wh_tabs + T_OPTM)   // sett[]'s counts: { 2, 3, 5, 5, NSTAGE + 1, synth::EQS }, then the COLORS options' (tools/tables.py)
enum { PL_SAVE = 15, PL_QUIT, PL_N };   // the pause menu's lines: 0 RESUME, 1..14 sett[] (THEME BONE puts the colours back to the default), SAVE GAME, QUIT TO TITLE
static_assert(synth::EQS == 9, "OPTM in tools/tables.py");

static void update() {
    static uint8_t hc;                                       // frames LEFT or RIGHT has been held: after 12, a step every 4th (the hand and shop cursors)
    const uint8_t hd = chgame.buttons() & (LEFT_BUTTON | RIGHT_BUTTON);
    hc = hd ? hc + 1 : 0;
    const uint8_t hr = hc > 11 && !(hc & 3) ? hd : 0;
    const uint8_t p = chgame.justPressedMask() & 63;      // the buttons that went down this frame (not START or SELECT)
    const bool a = p & A_BUTTON, bb = p & B_BUTTON, up = p & UP_BUTTON, dn = p & DOWN_BUTTON, lf = p & LEFT_BUTTON, rt = p & RIGHT_BUTTON;
    rnd();
    if (state == PAUSED) {
        nav(pauseSel, PL_N - 1, up, dn);
        if (up || dn) note = 0;
        const uint8_t L = pauseSel;
        if (bb || (a && !L)) { state = lastSt = (State)pauseFrom; return; }    // B or RESUME: back to where it was
#ifdef PERF_LEAN
        if (L > 3 && L < 7) return;                                // (the board's debug build has no INDEX, SONG or EQ)
#endif
        if (L && L < PL_SAVE && (a || lf || rt)) {
            const uint8_t m = OPTM[L - 1]; sett[L - 1] = (uint8_t)((sett[L - 1] + (lf ? m - 1 : 1)) % m);
            colDue = 1 + (L == 7);                                      // (the palette again; a THEME brings its options)
            note = 0; saveSettings();
            if (L != 5) playSfx(Sfx::Move);                            // (a new SONG is heard instead)
        }
        else if (!a) {}
        else if (L == PL_SAVE) {
            if (timed) { note = N_TIMED; playSfx(Sfx::No); }
#ifdef PERF_BUILD
            else note = N_OFF;
#else
            else if ((pauseFrom | 2) != SHOP) { note = N_OFF; playSfx(Sfx::No); }   // (a run is saved at the table or in the shop)
            else { saveRun(); note = save::available() ? N_SAVED : N_OFF; playSfx(Sfx::Buy); }
#endif
        }
        else if (note != N_QUIT) { note = N_QUIT; playSfx(Sfx::No); }   // quitting asks twice
#ifndef PERF_LEAN
        else if (bonusPractice) practiceBack();
#endif
        else { toTitle(); note = 0; }
        return;
    }
    if (state == PLAY && showLv) { if (p) showLv = false; return; }    // the hand levels table: any button closes it
    if (timed && (state == PLAY || state == RESULT) && tLeft > 0) {
        tLeft--;
        if (tLeft <= 150 && tLeft % 30 == 0) playSfx(Sfx::Tick, 12);   // the last five seconds tick
        if (!tLeft && state == PLAY) { timeUp(false); return; }
    }
    if ((1 << DEAL | 1 << RESULT | 1 << BONUS | 1 << TIMEUP) >> state & 1 && timer < 255) timer++;   // these screens count their frames (MINI: miniUpdate)
    switch (state) {
    case TITLE: {
        int n = titleN();
        if (titleSel >= n) titleSel = 0;
        wrapNav(titleSel, n, up, dn);
        if (a) {
            int k = titleK(titleSel);
#ifdef PERF_LEAN
            if (k == 1) newRun(); else startTurbo();
#else
            if (k == 0) CONTINUE_RUN(); else if (k == 1) newRun(); else if (k == 2) { bonusPractice = true; bselSel = 0; state = BSEL; } else if (k == 4) { bonusPractice = true; bselSel = 255; practiceStart(MK_VM + 4); } else startTurbo();
#endif
        }
        break;
    }
    case DEAL:
        if ((a && timer > 8) || (timer >= 150 && boss == 255)) { state = PLAY; timer = 0; }
        break;
    case PLAY:
        nav(cur, nHand - 1, lf | hr & LEFT_BUTTON, rt | hr & RIGHT_BUTTON);
        if (dn && !timed) { showLv = true; playSfx(Sfx::Flick); }
        if (a && nHand) {
            if (sel[cur]) { sel[cur] = false; playSfx(Sfx::Drop); }
            else if (selCount() < maxPick) {
                sel[cur] = true; playSfx(Sfx::Pick);
                if (!(pickSeen >> cur & 1)) { pickSeen |= 1 << cur; cR = hand[cur].r; cS = hand[cur].s; jfire(EV_PICK); cR = cS = 255; }   // (a card counts once a deal)
            }
            else playSfx(Sfx::No);
        }
        if (bb && selCount() > 0 && playsLeft) {
            Card c[5]; int n; removeSelected(n, c);
            scoreHand(c, n); if (!timed) playsLeft--; timer = 0; state = RESULT; playSfx(Sfx::Snap);
            dealerReact(lastGain * 4 >= goal() || lastType >= 4);
        }
        if (up && discLeft && nHand > (selCount() ? selCount() : 1)) { if (!selCount()) sel[cur] = true;   // (never toss the last cards: an empty hand could not go on)
            Card c[5]; int n; removeSelected(n, c); if (!timed) discLeft--; playSfx(Sfx::Flick); jfire(EV_TOSS); }
        break;
    case RESULT:
        if (timer >= 6 && timer <= 20 && !(timer & 1)) playSfx(Sfx::Tick, (uint8_t)(timer - 6));   // the tally ticks up
        if (timer > resEnd() && (a || bb || (timed && timer > resEnd() + 10))) {
            score += lastGain; burnNow();
            if (timed) { timedResult(); if (state == PLAY && !tLeft) timeUp(false); break; }
            if (score >= goal()) {
                money += 3 + playsLeft + (boss != 255 ? 5 : 0) + (dealer == 4 ? 4 : 0);
                if (round_ + 1 > hiRound) hiRound = round_ + 1;
                boss = 255;
                if (round_ + 1 == ROUNDS) { state = WON; playSfx(Sfx::Win); endSave(); sv.round_ = 0; }
                else { round_++; startBonus(); }   // a bonus stage after every night (then the shop)
            } else if (!playsLeft || nHand == 0) { state = OVER; boss = 255; playSfx(Sfx::Over); endSave(); }
            else state = PLAY;
        }
        break;
    case BONUS:
        if (timer < 200) { if (a && timer > 10) timer = 200; }
        else if (a) { bonusGo(); playSfx(Sfx::Shuffle); }
#ifndef PERF_LEAN
        else if (bb && bonusPractice) { practiceBack(); playSfx(Sfx::Move); }
#endif
        else if (bb) { openShop(); playSfx(Sfx::Move); }
        break;
    case MINI: miniUpdate(up, dn, lf, rt, a); break;
#ifndef PERF_LEAN
    case BSEL:
        wrapNav(bselSel, NPICK, up, dn);
        if (a) practiceStart(pickK(bselSel));
        else if (bb) { bonusPractice = false; titleSel = 1 + hasSave; state = TITLE; }
        break;
#endif
    case TIMEUP:
        if (timer > 30 && a) {
#ifndef PERF_LEAN
            if (bonusPractice) practiceBack();
            else
#endif
            if (mode == M_TURBO) toTitle();                     // (bonusOn is already false)
            else openShop();
        }
        break;
    case SHOP: {
        const bool l2 = lf | up | hr & LEFT_BUTTON, r2 = rt | dn | hr & RIGHT_BUTTON;
        if (shopMode == 0) {
            nav(shopSel, shopN + 1, l2, r2);
            if (bb) { if (nJok) { shopMode = 1; ownSel = 0; note = 0; playSfx(Sfx::Flick); } else playSfx(Sfx::No); }   // B: your cards
            if (a) {
                moodT = 0;
                if (shopSel < shopN) {
                    if (shopOffer[shopSel] == 255) playSfx(Sfx::No);
                    else if (nJok >= JMAX) { note = N_FULL; playSfx(Sfx::No); shopMode = 1; ownSel = 0; }   // sell or throw one first
                    else if (money < jprice(shopOffer[shopSel])) { note = N_NOCASH; playSfx(Sfx::No); }
                    else { money -= jprice(shopOffer[shopSel]); addJok(shopOffer[shopSel]); shopOffer[shopSel] = 255; note = N_BOUGHT; saveRun(); playSfx(Sfx::Buy); }
                } else if (shopSel == shopN) {
                    if (money >= 2) { money -= 2; shopRoll(); note = 0; playSfx(Sfx::Reroll); }
                    else { note = N_NOCASH; playSfx(Sfx::No); }
                } else startRound();
            }
        } else if (shopMode == 1) {
            nav(ownSel, nJok - 1, l2, r2);
            if (a) { shopMode = 2; confSel = 0; playSfx(Sfx::Flick); }
            else if (bb) { shopMode = 0; note = 0; playSfx(Sfx::Move); }
        } else {                                                 // SELL (half the price back), THROW (nothing), CANCEL
            nav(confSel, 2, l2, r2);
            if (bb || (a && confSel == 2)) { shopMode = 1; playSfx(Sfx::Move); }
            else if (a) {
                if (!confSel) money += jprice(jokers[ownSel]) / 2;
                ashAt(tileX(ownSel, nJok), 72);
                jokRemove(ownSel); shopMode = 0; note = 0; saveRun(); playSfx(confSel ? Sfx::Puff : Sfx::Buy);
            }
        }
        break;
    }
    case WON:
        if (a) { round_++; openShop(); playSfx(Sfx::Clear); }           // endless: the run goes on
        else if (bb) toTitle();                       // (mode, timed and bonusOn are already M_RUN, false)
        break;
    case OVER:
        if (a || bb) toTitle();
        break;
    default: break;
    }
}

// ---- drawing: Gfx.h (art in Art.h, made by tools/art.py), the dealers (Dealers.h), the new screens (Bonus.h)
#include "TextDraw.h"   // the game's own font and text renderer (in place of CHGfx's gfx_text, gfx_textFx ...)
#include "Gfx.h"
#include "Dealers.h"
#include "Bonus.h"
#include "Minis.h"

// the TX_* enums against the text groups tools/make_card.py wrote (CardIndex.h CT_*, from tools/card_text.py): each enum's end is its group's count
static_assert(TX_STEP == NJOK && TX_GOALF + 1 == CT_G0, "text block 0 (card_text.py JOKERS + SHOP) vs TX_STEP.. in Gfx.h");
static_assert(TX_RESUME == CT_JSPLIT && TX_RISE + 1 == CT_G1, "text block 1 (card_text.py JSPLIT + CALM) vs TX_RESUME.. in Bonus.h");
static_assert(TX_MINUS1 + 1 == CT_G2, "text block 2 (card_text.py BONUS) vs TX_DNAME.. in Bonus.h");
static_assert(TX_C_IDXEND + 1 == CT_G3 && TX_C_IDX == 36, "text block 3 (card_text.py MINI) vs TX_MINI.. in Bonus.h");
static_assert(TX_COMBOF + 1 == CT_G4 && STAGE_TXG == COL_DATG + 1, "text block 4 (card_text.py UI) vs TX_T_CONT.. in CHWildHand.ino");
static_assert(NJOK - CT_JSPLIT == CT_G5, "text block 5 (the rest of the wild cards' descriptions) vs NJOK, JSPLIT");
static_assert(TX_Y_FIRST + 1 == CT_G6, "text block 6 (card_text.py SELL) vs TX_Y_*");
static_assert(TX_N_MISS + 1 == CT_G7, "text block 7 (card_text.py MINI2) vs TX_N_* in Bonus.h");
static_assert(TX_M_DISC + 1 == CT_MISC, "card_text.py MISC vs TX_M_*");

#ifdef CHSIM
void sim_sync();
void sim_advance(uint32_t us);
void sim_waitInput();
#endif

void setup() {
    chgame.boot();
#if CHGAME_DEBUG
    dbg::begin("WILDHAND 1.0");
#endif
    gfx_begin(GFX_DIV2, GFX_12BPP);
    gfxInit();   // palette and debug commands (Gfx.h)
    leadInit();  // more debug commands (Bonus.h)
    chgame.setFrameRate(30);
    synth::begin();
    openCard();
    loadSave();
    rng ^= millis();
    state = TITLE;
    titleSel = 0;
}

// the save: version 2 (older saves are not read)
static void loadSave() {
    uint8_t flag = 0;
    memset(&sv, 0, sizeof sv);
    sv.music = 1;
#ifndef PERF_BUILD
    const bool had = save::load(MAGIC, 2, sv, &flag);   // (flag stays 0 if there is none; an empty save decodes to the defaults)
    uint8_t m[sizeof sv + 1];                        // the card's copy: restores a save the flash page lost; its settings win either way (another game may have taken the page)
    if (card::getSave(m, sizeof m)) {
        SaveData c; memcpy(&c, m, sizeof c);
        if (had) { sv.music = c.music; sv.fx = (uint8_t)((sv.fx & 0xF0) | (c.fx & 15)); sv.pad = c.pad; sv.hi = (sv.hi & 255) | (c.hi & ~255); }
        else { memcpy(&sv, m, sizeof sv); flag = m[sizeof sv]; saveFlag = flag; saveDue = true; }
    }
    uint32_t cp = (uint32_t)sv.hi >> 8;
    for (int k = 1; k < 6; k++, cp >>= 4) cop[k] = cp & 15;   // (the save's check keeps them as written: in range)
    bgSel = cp & 7; bgHold = cp >> 3;
    colDue = 1;                                      // (their colours, at the next frame)
    musicOn = sv.music & 1; idxSz = (sv.music >> 1) & 7; cop[0] = sv.music >> 5; fxLow = (sv.fx & 1) | (sv.music >> 3 & 2); turboHi = sv.turboHi;
    cardSz = (uint8_t)((((sv.fx >> 1) & 7) + 2) % 5);
    runOff = (sv.fx >> 4) % NSTAGE; songSel = sv.pad & 15; if (songSel > NSTAGE) songSel = 0; eqSel = sv.pad >> 4; if (eqSel >= synth::EQS) eqSel = 0;   // sv.fx bits 1-3: the CARD size, stored so 0 = MEDIUM (old saves read MEDIUM)
#endif
    hiRound = sv.hi & 255;
    hasSave = flag == 1;
}

void loop() {
    if (chgame.justPressed(SELECT_BUTTON)) { cop[0] = (uint8_t)((cop[0] + 1) % OPTM[6]); colDue = 2; saveSettings(); playSfx(Sfx::Move); }   // SELECT: the next colour THEME
    if (chgame.justPressed(START_BUTTON)) {
        if (state == PAUSED) state = (State)pauseFrom;
        else if ((state | 1) != WON) { pauseFrom = state; state = PAUSED; pauseSel = 0; note = 0; }   // (the end screens count their frames to a prompt: no pause there)
        lastSt = state;   // (by hand: into the pause menu and back out nothing replays)
    }
    dbg::markRenderStart();
    update();
#ifndef PERF_LEAN
    synth::eq(eqSel);
#endif
    int song = stageSong();                                     // the song for each state (the stage's, unless one of these)
    if (state == PAUSED) { if (pauseSel < 4) song = SG_PAUSE; }
    else if (state == TITLE) song = SG_TITLE;
    else if (state == SHOP) song = (round_ & 1) ? SG_SHOP_B : SG_SHOP_A;
    else if (state == OVER || state == WON) song = SG_OVER;
    else if (mode == M_TURBO) song = SG_TURBO;
    else if (bonusOn || state == BONUS || state == MINI || bonusPractice) song = SG_BONUS;
    synth::music(musicOn ? song : -1);
    if (!card::ok() || (unsigned)(state - PLAY) > 1) gfx_wait();               // the table (the hand, the result) draws the felt first, top to bottom: it follows the last flush down the screen, which is still running; the card waits for it itself (Card.cpp), and the flash save never happens at the table
    card::frame();
    if (!card::ok() && (fc % 60) == 2) openCard();
    static bool busy;                                           // the last frame's drawing was long (board only): a music refill, a card read of unknown length, waits for a calm frame
#ifndef CHSIM
    const uint32_t t0 = micros();
#endif
    for (uint8_t k = 0; k < 1; k++) {                // the music's next 64 steps (or a new song's header and first and last chunks), from the card
        uint32_t off; uint8_t *dst;
        if (!card::ok() || !synth::refill(&off, &dst, busy) || !card::copyDirect(dst, CARD_SONG + off, 192)) break;
        synth::filled();
    }
#ifndef PERF_BUILD
    if (saveDue) {
        saveDue = false; gfx_wait();                              // (the flash page buffer is the flush scratch: at the table the wait above was skipped)
        const bool ok = save::store(MAGIC, 2, sv, saveFlag ? 1 : 0);   // (false exactly when save::available() is false after it)
        hasSave = saveFlag && ok;
    }
    if (cardDue && !--cardDue) {
        packSettings(); saveDue = true;                           // (again: a new THEME's options are read in the frame after the change, so the first pack missed them)
        uint8_t m[sizeof sv + 1]; memcpy(m, &sv, sizeof sv); m[sizeof sv] = saveFlag;
        card::putSave(m, sizeof m);                              // the whole save, alternating records (a torn write keeps the older)
    }                  // the settings, a little after the last change, also on the card (one block write; silent if it fails)
#endif
#if defined(PERF_LEAN) || defined(PERF_BUILD)
    draw();                                                     // (the perf tool runs with the card in)
#else
    if (card::ok()) draw(); else drawNeedCard();
#endif
#ifndef CHSIM
    busy = micros() - t0 > 12000;
#endif
    dbg::markRenderEnd();
    gfx_flushAsync();
#ifdef CHSIM
    uint8_t idle = 0;
    for (dbg::poll(); !chgame.nextFrame(); dbg::poll()) {
        sim_sync();
        sim_advance(1000);
        if (++idle > 2) { sim_waitInput(); idle = 0; }
    }
#else
    do { dbg::poll(); } while (!chgame.nextFrame());
#endif
    chgame.pollButtons();
    dbg::markUpdateStart();
    static uint32_t startDown;
    if (!chgame.pressed(START_BUTTON)) startDown = 0;
    else if (!startDown) startDown = millis() | 1;
    else if ((int32_t)(millis() - startDown) >= 3000) chgame.exitToMenu();   // (signed: startDown is millis() | 1, one ahead when millis() is even)
}
