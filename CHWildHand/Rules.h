// Rules.h: Wild Hand's depth (v0.2). 40 wild cards (a rule table on the SD card, run by jrun/jeff below) and four combos, hand levels, boss rounds, endless goals,
// the dealers' edges (the newer three and the hexes 5-6 as rule records), and the timed plays (the turbo arcade and the bonus stage). Integer maths only.
// Included by CHWildHand.ino after the game state; Dealers.h and Bonus.h draw what this decides.
#pragma once

enum { NJOK = 40, NDEAL = 9, NBOSS = 7, LVMAX = 9, BONUS_FRAMES = 600, TURBO_FRAMES = 1800 };
enum Mode : uint8_t { M_RUN, M_TURBO };

// combos: owning both wild cards adds an effect
#define COMBO ((const uint8_t (*)[2])(wh_tabs + T_COMBO))
// hand levels: what each level adds to a hand's base
#define LVCH ((const uint8_t *)(wh_tabs + T_LVCH))   // (in RAM, from the card: tools/tables.py)

static uint8_t mode, hlv[9], boss = 255, deadSuit, dealer, dealerBase, maxPick = 5, bonusPrize, playsDone, turboStage, prevT;   // prevT: the last hand's type (NO REPEATS)
static bool hidden[HANDN], timed, bonusOn, x2Pend, x2Now, penalty, showLv, bonusWon, prizeFull;
static uint8_t runOff;                                   // the run counter (the AUTO rotation)
static uint8_t colDue = 1;   // 1: read the COLORS options' colours (Gfx.h applyCol), 2: the THEME preset's options first
// (SEANCE, GALLOWS and DEAD WHEEL are scripts, MK_VM + 0, 1, 2: their ids here only keep text block 7 in place)
enum { MK_SPEED, MK_HILO, MK_DUEL, MK_PAIRS, MK_SEANCE, MK_GALLOWS, MK_WHEEL, MK_RAVEN, MK_GHOULS, MK_COUNT, NMINI, NNEW = NMINI - MK_SEANCE, MK_VM = NMINI };
// the stages that can come up (the NEW_* switches in the .ino): the first four, or a list of the ones in, then the scripts (StageVM.h)
#if !NEW_STAGES
#define NPICK MK_SEANCE
#define PICK_ID 1                                        // (pickK is the identity)
static uint8_t pickK(uint8_t i) { return i; }
#else
static const uint8_t PICKS[] = { MK_SPEED, MK_HILO, MK_DUEL, MK_PAIRS
#if NEW_RAVEN
    , MK_RAVEN
#endif
#if NEW_GHOULS
    , MK_GHOULS
#endif
#if NEW_COUNT
    , MK_COUNT
#endif
};
#define NPICK ((int)sizeof PICKS + NEW_VM * STAGE_N)
static uint8_t pickK(uint8_t i) { return i < sizeof PICKS ? PICKS[i] : (uint8_t)(MK_VM + i - sizeof PICKS); }
#endif
static uint8_t miniK, lastMiniK;                          // which bonus stage
static void miniStart();
static bool skipOpen;                                    // debug: no opening bonus stage
static int8_t forceBoss = -1, forceMini = -1;             // debug: the next boss twist and bonus stage                            // debug: the next round's boss twist
static int16_t tLeft;                                    // frames left in a timed play
static int32_t bonusGoal, turboHi, milestone;

static int32_t goal() {
    if (bonusOn) return bonusGoal;
    if (mode == M_TURBO) return milestone;
    if (round_ < ROUNDS) return TARGET[round_];
    int32_t t = TARGET[ROUNDS - 1];                      // endless: half as much again each round
    for (int i = ROUNDS - 1; i < round_ && t < 400000000; i++) t = t * 3 / 2;
    return t;
}
static bool bossRound() { return mode == M_RUN && round_ % 3 == 2; }
__attribute__((noinline)) static int handChips(int t) { return HCHIPS[t] + LVCH[t] * hlv[t]; }
__attribute__((noinline)) static int handMult(int t) { return HMULT[t] + (t ? hlv[t] : hlv[t] / 2); }
static bool comboOn(int k) { return hasJoker(COMBO[k][0]) && hasJoker(COMBO[k][1]); }
static const char *comboHint(uint8_t id) {             // the shop: this card would complete a combo
    for (int k = 0; k < 4; k++)
        for (int e = 0; e < 2; e++)
            if (COMBO[k][e] == id && hasJoker(COMBO[k][!e])) return strf(cardText(4, TX_COMBOF), cardText(4, TX_COMBO + k));
    return nullptr;
}
static int jprice(uint8_t id) { return wildRec(id)[0] & 15; }
static void ashAt(int x, int y);                         // Gfx.h: a puff of ash
static int tileStride(int n) { return n <= 3 ? 42 : n == 4 ? 31 : 25; }          // the shop's cards: three big, four or five narrow
static int tileX(int i, int n) { return 3 + i * tileStride(n) + (n <= 3 ? 19 : tileStride(n) / 2 - 1); }   // a card's centre
static bool isRed(uint8_t s) { return s < 2; }

// ---- the wild cards' engine. Each card is a few rules (tools/jokers.py, on the SD card, in RAM at wildRec()): when (an event), if (a condition),
// what (an effect and an amount). Events: the hand scored (and each scoring card), a card picked, a discard, the night's start, the night won.
enum { EV_HAND, EV_CARD, EV_PICK, EV_TOSS, EV_START, EV_WIN, NFL = 10, NEV = 8 };
struct Fl { int8_t t; uint8_t slot, kind; int16_t val; };     // a number floating up from a wild card's slot: t < 0 frames to wait; kind is the effect's target + 1 (0: free)
static Fl fl[NFL];
static int16_t evC[NEV + 1], evM[NEV + 1];                    // the tally after each wild card that fired (0: the hand's base)
static uint8_t slotPop[JMAX], nEv;                            // each slot's pop (frames), the events of this hand
#define FLCLR() memset(fl, 0, sizeof fl)
static uint8_t cnt[JMAX], burnMask, shopN = 3, pickSeen, cR = 255, cS = 255, cSuits;   // each slot's counter (saved), the slots to burn, the cards offered, the picks counted
static int cFirstQ;                                           // the first scoring card's chips
static bool scoring;                                          // jeff's feedback goes into the tally (else floats up at once)

static int jq(uint8_t s, uint8_t j) {                         // what a SCALED rule multiplies its amount by
    return s == 0 ? cnt[j] : s == 1 ? (timed ? 0 : discLeft) : s == 2 ? round_ : s == 3 ? (money > 50 ? 50 : (int)money) : cFirstQ;
}
__attribute__((optimize("jump-tables"))) static bool jcond(uint8_t c, uint8_t a, uint8_t j) {
    switch (c) {
    case 0: case 15: return true;
    case 1: return cS == a;
    case 2: return cR >= (a >> 4) && cR <= (a & 15);
    case 3: return !(cR & 1) && cR <= a;
    case 4: return lastType && (a >> (lastType - 1) & 1);
    case 5: return lastType == a;
    case 6: return lastN >= a;
    case 7: return playsLeft == a && !timed;
    case 8: return playsDone == a;
    case 9: return cSuits == a;
    case 10: return lastType == prevT;
    case 11: return cnt[j] >= a;
    case 12: return (rnd() & 255) < a;
    case 13: return boss != 255;
    default: return timed;
    }
}
static void fb(uint8_t j, uint8_t k, int v) {                // a wild card did something: queue it (a hand's tally) or float it at once
    if (scoring) {
        if (k == 7) { slotPop[j] = 6; return; }
        Fl *f = nEv ? &fl[nEv - 1] : nullptr;
        if (f && f->slot == j && f->kind == k + 1 && k != 6) f->val += v;
        else if (nEv < NEV) { f = &fl[nEv]; *f = { (int8_t)-(23 + nEv * (timed ? 2 : 6)), j, (uint8_t)(k + 1), (int16_t)v }; nEv++; }
        else return;
        evC[nEv] = lastChips; evM[nEv] = lastMult;
    } else for (Fl &f : fl) if (!f.kind) { f = { -1, j, (uint8_t)(k + 1), (int16_t)v }; break; }
}
static void jeff(uint8_t j, const uint8_t *h, const uint8_t *r, uint8_t cd) {
    uint8_t e = r[2] >> 4;
    int v = r[3];
    if (cd == 15) v *= jq(r[1], j);
    if (e & 1) v = -v;
    int k = e >> 1;
    switch (k) {
    case 0: lastChips += v; if (lastChips < 0) lastChips = 0; break;
    case 1: if (lastMult + v < 1) v = 1 - lastMult; lastMult += v; break;
    case 2: if (money + v < 0) v = -(int)money; money += v; break;
    case 3: if (playsLeft + v < 1) v = 1 - playsLeft; playsLeft += v; break;
    case 4: if (discLeft + v < 0) v = -discLeft; discLeft += v; break;
    case 5: tLeft += v * 30; break;
    case 6: lastMult = v < 0 ? (lastMult + 1) >> 1 : lastMult * v; break;   // (negative: halved, rounded up)
    default: if (cnt[j] < h[1] >> 4) cnt[j]++; else v = 0;
    }
    if (r[2] & 1) cnt[j] = 0;
    if (r[2] & 2) burnMask |= 1 << j;
    if (v && j < JMAX) fb(j, k, v);                         // (a dealer's or a hex's rules: no slot to float from)
}
static void jrun(uint8_t j, uint8_t ev, unsigned id) {        // slot j's rules (record id) for ev (EV_HAND: the scoring pass, with its per-card rules, in the card's order)
    const uint8_t *h = wildRec(id);
    for (uint8_t k = 0, n = (h[0] >> 4) & 3; k < n; k++) {
        const uint8_t *r = h + 4 + 4 * k;
        uint8_t e = r[0] >> 5, cd = r[0] & 31;
        if (ev == EV_HAND ? e > EV_CARD : e != ev) continue;
        const bool card = e == EV_CARD;                  // (EV_CARD: once per scoring card, that card in cR, cS; else once)
        for (uint8_t i = 0; i < (card ? lastN : 1); i++) {
            if (card) { if (!lastScoring[i]) continue; cR = lastCards[i].r; cS = lastCards[i].s; }
            if (jcond(cd, r[1], j)) jeff(j, h, r, cd);
        }
        if (card) cR = cS = 255;
    }
}
// the later dealers' edges and hexes' twists are records after the wild cards' (tools/jokers.py EXTRA): dealers 6-8, hexes 5-6; not in a timed play
static void xrun(uint8_t ev) {
    if (timed) return;
    unsigned d = dealer - 6u, b = boss - 5u;
    if (d < 3) jrun(JMAX, ev, NJOK + d);
    if (b < NBOSS - 5) jrun(JMAX, ev, NJOK + 3 + b);
}
static void jfire(uint8_t ev) { for (uint8_t j = 0; j < nJok; j++) jrun(j, ev, jokers[j]); xrun(ev); }
__attribute__((noinline)) static void addJok(uint8_t id) { cnt[nJok] = 0; jokers[nJok++] = id; }
static void clearJok() { nJok = burnMask = 0; memset(cnt, 0, sizeof cnt); FLCLR(); }
static void jokRemove(uint8_t i) { for (; i + 1 < nJok; i++) { jokers[i] = jokers[i + 1]; cnt[i] = cnt[i + 1]; } nJok--; }
static void burnNow() { for (int i = nJok - 1; i >= 0; i--) if (burnMask >> i & 1) jokRemove((uint8_t)i); burnMask = 0; }
static int resEnd() { return nEv ? 22 + nEv * (timed ? 2 : 6) : 20; }   // the frame the tally has finished (the result screen)

__attribute__((noinline)) static void scoreHand(const Card *c, int n) {
    evaluate(c, n, lastType, lastScoring);
    lastN = n;
    for (int i = 0; i < n; i++) { lastCards[i] = c[i]; if (boss == 0 && c[i].s == deadSuit) lastScoring[i] = false; }
    int t = lastType;
    lastChips = handChips(t); lastMult = handMult(t);
    int first = -1, suits = 0, red = 0, blk = 0, face = 0, ace = 0;
    for (int i = 0; i < n; i++) if (lastScoring[i]) {
        lastChips += rankChips(c[i].r);
        if (first < 0) first = i;
        suits |= 1 << c[i].s;
        if (isRed(c[i].s)) red++; else blk++;
        if (c[i].r >= 9 && c[i].r <= 11) face++;
        if (c[i].r == 12) ace++;
    }
    if (dealer == 5 && !timed) lastChips += 10;           // PUFF's tailwind
    cSuits = (uint8_t)suits; cFirstQ = first >= 0 ? rankChips(c[first].r) : 0;
    scoring = true;
    FLCLR(); nEv = 0; evC[0] = lastChips; evM[0] = lastMult;
    for (uint8_t j = 0; j < nJok; j++) jrun(j, EV_HAND, jokers[j]);   // (not jfire: xrun's EV_HAND comes after the combos, below)
    for (int k = 0; k < 4; k++) {
        if (!comboOn(k)) continue;
        bool on = k == 0 ? (red && !blk) : k == 1 ? (blk && !red) : k == 2 ? true : (face && ace);
        if (!on) continue;
        uint8_t s = 0; while (jokers[s] != COMBO[k][0]) s++;
        if (k == 2) { lastChips += 40; lastMult += 4; fb(s, 1, 4); } else { lastMult *= 2; fb(s, 6, 2); }
    }
    xrun(EV_HAND);                                       // the newer dealers' and hexes' (SCARECROW, FADING HANDS, NO REPEATS)
    if (bonusOn && t == 0) lastChips = 0;                // the bonus stage: pairs or better
    if (x2Now) lastMult *= 2;
    if (dealer == 2 && !timed) money++;                  // DUSTY's pickpocket
    lastGain = lastChips * lastMult;
    if (!timed && score + lastGain >= goal()) jfire(EV_WIN);   // this hand wins the night
    scoring = false;
    if (!bonusOn && hlv[t] < LVMAX) hlv[t]++;
    playsDone++; prevT = (uint8_t)t;
}

// ---- the hand, arranged on every refill: the made hands left to right, best first (straight flush, four, full house, flush, straight,
// three, two pair, pair: evaluate()'s types, each the best of its kind left in the hand), then the loose cards; each high to low, ace high
// (a full house: the three, then the pair). Face-down cards (boss: hidden) take no part (their rank must not show): last, as they were.
// Rank masks by count and suit counts, a pass a group (at most four), no subsets tried: under 3,000 instructions at worst, once per refill.
struct HS { Card o[HANDN]; uint8_t k, used; };
__attribute__((noinline)) static void hsEmit(HS &h, int w, int s, int per, int n) {   // n cards of the ranks in w (suit s; 4: any), high first, up to per a rank (bit 13: any rank, in slot order)
    for (int r = 13; r >= 0; r--) if (w >> r & 1)
        for (int i = 0, t = per; i < nHand && t && n; i++)
            if (!(h.used >> i & 1) && (hand[i].r == r || r > 12) && (s > 3 || hand[i].s == s)) { h.used |= 1 << i; h.o[h.k++] = hand[i]; t--; n--; }
}
static int hiStraight(int m) {                           // the best straight in a rank mask, as a mask (0: none)
    int w = m << 1 | (m >> 12 & 1);                      // (bit 0: the ace again, low)
    for (int t = 12; t >= 3; t--) if ((w >> (t - 3) & 31) == 31) return t > 3 ? 31 << (t - 4) : 0x100F;
    return 0;
}
static void sortHand() {
    HS h; h.k = h.used = 0;
    int hm = 0;                                          // the face-down cards
    for (;;) {
        uint8_t sc[4] = {};
        int sm[4] = {}, m1 = 0, m2 = 0, m3 = 0, m4 = 0, f = 4, w, s = 4, per = 1, n = 5;   // the ranks held once, twice, three times, four times; each suit's ranks; a suit of five
        for (int i = 0; i < nHand; i++) if (!((h.used |= hm |= hidden[i] << i) >> i & 1)) {
            int b = 1 << hand[i].r;
            m4 |= m3 & b; m3 |= m2 & b; m2 |= m1 & b; m1 |= b; sm[hand[i].s] |= b;
            if (++sc[hand[i].s] >= 5) f = hand[i].s;
        }
        if (f < 4 && (w = hiStraight(sm[f]))) s = f;
        else if (m4) { w = m4; per = n = 4; }
        else if (m3 && (m2 & (m2 - 1))) { hsEmit(h, m3, 4, 3, 3); w = m2; per = n = 2; }   // (the best three, then the best pair left)
        else if (f < 4) w = sm[s = f];
        else if ((w = hiStraight(m1))) {}
        else if (m3) { w = m3; per = n = 3; }
        else if (m2) { w = m2; per = 2; n = 4; }
        else break;
        hsEmit(h, w, s, per, n);
    }
    hsEmit(h, 0x1FFF, 4, 4, HANDN);
    int up = h.k;
    h.used = ~hm;                                        // (now only the face-down cards are left)
    hsEmit(h, 1 << 13, 4, HANDN, HANDN);
    for (int i = 0; i < nHand; i++) { hand[i] = h.o[i]; hidden[i] = i >= up; }
}

// ---- dealing
static void refill() {
    while (nHand < HANDN) {
        if (deckPos >= 52) {
            if (!timed) break;
            // a timed play never runs dry: a fresh deck, less the cards in hand
            shuffleDeck();
            for (int i = 0; i < nHand; i++)
                for (int j = deckPos; j < 52; j++)
                    if (deck[j].r == hand[i].r && deck[j].s == hand[i].s) { Card t = deck[j]; deck[j] = deck[deckPos]; deck[deckPos++] = t; break; }
        }
        hidden[nHand] = boss == 1 && rnd() % 3 == 0;
        hand[nHand++] = deck[deckPos++];
    }
    sortHand();
}
static void startRound() {
    shuffleDeck(); nHand = 0;
    if (!bgHold) bgSel = 0;                              // BG HOLD STAGE: a picked BG lasts the night
    boss = forceBoss >= 0 ? forceBoss : (bossRound() && !timed) ? rnd() % NBOSS : 255;
    deadSuit = rnd() & 3;
    if (!timed) dealer = (dealerBase + round_) % NDEAL;
    if (dealer == 3 && !timed)                           // MAGNUS's lucky draw: the first hand holds a pair
        for (int j = 2; j < 52; j++) if (deck[j].r == deck[0].r) { Card t = deck[1]; deck[1] = deck[j]; deck[j] = t; break; }
    refill();
    for (int i = 0; i < HANDN; i++) sel[i] = false;
    cur = 0; score = 0; playsLeft = PLAYS; discLeft = DISCS; maxPick = 5; playsDone = 0; showLv = false; prevT = 255;
    if (boss == 2) playsLeft = 3;
    if (boss == 3) discLeft = 0;
    if (boss == 4) maxPick = 4;
    if (dealer == 1 && !timed) discLeft++;               // OCTAVIA: one more discard
    pickSeen = 0; FLCLR();
    jfire(EV_START);                                     // the wild cards' night-start rules (BONE DICE: one hand fewer), the newer dealers' (PLAGUE DOCTOR, PORCELAIN DOLL)
    if (penalty && playsLeft > 1) playsLeft--;
    penalty = false;
    x2Now = x2Pend; x2Pend = false;
    if (timed) { playsLeft = 99; discLeft = 99; }
    timer = 0;
    state = timed ? PLAY : DEAL;
    playSfx(Sfx::Shuffle);
}
static void startBonus();
__attribute__((noinline)) static void freshRun(uint8_t m, uint8_t r, uint8_t cash) {   // a new run, turbo or practice: no wild cards, no levels, no twists
    mode = m; timed = m == M_TURBO; bonusOn = false; round_ = r; money = cash; clearJok();
    memset(hlv, 0, sizeof hlv); x2Pend = penalty = false;
}
static void newRun() {
    freshRun(M_RUN, 0, 4);
    for (int i = 0; i < 9; i++) hlv[i] = (sv.lv[i] & 15) / 2 > LVMAX ? LVMAX : (sv.lv[i] & 15) / 2;   // what a death (or a win) leaves: half of each hand's level, rounded down,
    if (sv.nJok) addJok(sv.jok[0] % NJOK);                // and the first wild card, kept (the save still holds the last run)
#ifndef PERF_LEAN
    runOff = (uint8_t)((runOff + 1) % NSTAGE);           // each run starts the stage songs one on
#endif
    dealerBase = rnd() % NDEAL;
    if (skipOpen) startRound(); else startBonus();           // levels 1, 4, 7, 10...: a bonus stage comes first (after a boss for the rest)
}
static void randomWild() {
    if (nJok >= JMAX) return;
    uint8_t id;
    do id = rnd() % NJOK; while (hasJoker(id));
    addJok(id);
}
static void startTurbo() {
    freshRun(M_TURBO, 0, 0);
    randomWild(); randomWild();
    dealer = rnd() % NDEAL;
    tLeft = TURBO_FRAMES; milestone = 500; turboStage = 0;
    startRound();
}
// the bonus stage, after a boss round is beaten (round_ already counts the next round)
static void startBonus() {
    state = BONUS; timer = 0;
    bonusPrize = rnd() % 3;
#ifdef PICK_ID
    miniK = lastMiniK = (uint8_t)((lastMiniK + 1 + rnd() % (NPICK - 1)) % NPICK);   // never the same stage twice running
#else
    lastMiniK = (uint8_t)((lastMiniK + 1 + rnd() % (NPICK - 1)) % NPICK); miniK = pickK(lastMiniK);   // (lastMiniK: the place in PICKS)
#endif
    if (forceMini >= 0) miniK = (uint8_t)forceMini;
    uint8_t r = round_; if (r) round_--;
    int32_t g = goal(); round_ = r;                          // the beaten round's goal (endless too); the opener uses level 1's
    bonusGoal = g * 3 / 4;
    playSfx(Sfx::Whoosh);
}
__attribute__((noinline)) static void bonusGo() {
    if (miniK != MK_SPEED) { miniStart(); return; }
    timed = true; bonusOn = true; tLeft = BONUS_FRAMES;
    startRound();
}
static void timeUp(bool won) {
    bonusWon = won; timer = 0; state = TIMEUP;
    if (mode == M_TURBO) {
        bonusWon = score > turboHi;
        if (score > turboHi) { turboHi = score; sv.turboHi = turboHi; saveFlag = hasSave; saveDue = true; }
        playSfx(Sfx::Alarm);
        return;
    }
    if (won) {
        if (bonusPrize == 0) money += 15 + round_ * 2;
        else if (bonusPrize == 1) { prizeFull = nJok >= JMAX; if (prizeFull) money += 12; else randomWild(); }
        else x2Pend = true;
        playSfx(Sfx::Win);
    } else { money /= 2; penalty = true; playSfx(Sfx::Wah); }
}
// after a timed play's result: the turbo's milestones add time; the bonus ends the moment its goal is met
static void timedResult() {
    if (bonusOn) { if (score >= bonusGoal) timeUp(true); else state = PLAY; return; }
    if (lastType >= 4) tLeft += 60;
    while (score >= milestone && milestone < (1L << 30)) { milestone = milestone * 2; tLeft += 150; turboStage++; playSfx(Sfx::Clear); }
    if (tLeft > 5400) tLeft = 5400;
    state = PLAY;
}
