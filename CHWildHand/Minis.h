// Minis.h: three more bonus stages after the card show (the speed run is the fourth, in Rules.h):
//   HIGH OR LOW  call higher or lower on five blind cards; four right wins
//   SHOWDOWN     pick a card from the dealer's fan, the dealer picks too: the higher card takes it, best of three
//   PAIRS        twelve cards face down, six pairs: find them all before the fifth miss
//   RAVEN RAIN, GHOULS, THE COUNT: three more, one NEW_* switch each in the .ino (newStart/newUpdate/drawNew below; their text is block 7)
//   SEANCE, GALLOWS, DEAD WHEEL, THE WATCHER ...: scripts on the SD card (StageVM.h, tools/stages), NEW_VM in the .ino
// Each ends in timeUp(won): the same prize or penalty as the speed run. Included after Bonus.h.
#pragma once

static uint8_t miniStep, miniWin, miniLoss, miniSel, miniT, miniA, miniB;
static uint16_t miniUp, miniDone, miniSeen;          // PAIRS: face up, matched, ever turned
static Card miniC[12];
__attribute__((noinline)) static void miniHit(bool ok, Sfx yes, Sfx no) { if (ok) { miniWin++; playSfx(yes); } else { miniLoss++; playSfx(no); } }   // a call right or wrong
#if NEW_VM
#include "StageVM.h"
#endif
#if NEW_SCAFFOLD
#if NEW_STAGES > 1
#define NEWFN __attribute__((noinline)) static           // (out of line: smaller with several stages in, bigger with one)
#else
#define NEWFN static
#endif
static uint8_t miniP, miniEnd;                          // the newer stages: a phase or timer, the pause before the result
static uint16_t miniF;                                  // the newer stages: frames since the start of a phase
#define MG ((const uint8_t (*)[2])(wh_tabs + T_MG))   // per newer stage: wins that win it, losses that end it
#if NEW_COUNT
// THE COUNT: miniStep cards (deck[0..]) flash by, miniP frames each; miniB the named suit (0..3) or colour (4 red, 5 black); miniSel your number
NEWFN void countRound() {                                // a new deck, a new ask: more cards and faster as the nights go (round_)
    shuffleDeck();
    int d = round_ < 11 ? round_ : 11;
    miniStep = (uint8_t)(10 + d * 5 / 11); miniP = (uint8_t)(22 - d * 12 / 11);
    miniB = rnd() & 3;
    if (round_ > 1 && (rnd() & 1)) miniB = 4 + (miniB & 1);
    for (;;) {                                           // the answer, in miniC[0].r; the number is set with 0..9, so a bigger count asks the other suit
        int n = 0;
        for (int i = 0; i < miniStep; i++) n += miniB < 4 ? deck[i].s == miniB : isRed(deck[i].s) == (miniB == 4);
        if (n < 10) { miniC[0].r = (uint8_t)n; break; }
        miniB = miniB < 4 ? (miniB + 1) & 3 : 9 - miniB;
    }
    miniA = 0; miniF = 0; miniSel = 0;
}
#endif
NEWFN void newStart() {
    miniA = miniB = miniSel = 0; miniP = 20; miniEnd = 0; miniF = 0;
#if NEW_RAVEN
    if (miniK == MK_RAVEN) { miniSel = 56; for (int i = 0; i < 6; i++) miniC[i].r = 255; }   // the hand's x; six falling things (r: y + 24)
#endif
#if NEW_COUNT
    if (miniK == MK_COUNT) countRound();
#endif
#if NEW_GHOULS
    if (miniK == MK_GHOULS) for (int i = 0; i < 12; i++) miniC[i].r = miniC[i].s = 0;   // r the frames left up, s the age (255: whacked)
#endif
}
#endif

static void miniStart() {
    shuffleDeck();
    miniStep = miniWin = miniLoss = miniSel = miniT = 0; miniUp = miniDone = miniSeen = 0; miniA = miniB = 255;
    if (miniK == MK_PAIRS) {                            // six ranks, two of each, shuffled
        for (int i = 0, k = 0; i < 12; k++) {              // six cards of six different ranks, two of each
            bool d = false;
            for (int j = 0; j < i; j += 2) d |= miniC[j].r == deck[k].r;
            if (!d) { miniC[i] = miniC[i + 1] = deck[k]; i += 2; }
        }
        for (int i = 0; i < 12; i += 2) miniC[i + 1].s = (uint8_t)((miniC[i].s + 1 + (rnd() & 1)) & 3);
        shuffleCards(miniC, 12);
    } else for (int i = 0; i < 12; i++) miniC[i] = deck[i];
#if NEW_VM
    if (miniK >= MK_VM) vmStart(); else
#endif
#if NEW_SCAFFOLD
    if (miniK >= MK_SEANCE) newStart();
#endif
    state = MINI; timer = 0;
}
#if NEW_SCAFFOLD
// the newer stages: one function, the stage by miniK. Win/loss counts in miniWin/miniLoss (goals in MG)
NEWFN void newUpdate(bool up, bool dn, bool lf, bool rt, bool a) {
    const uint8_t *g = MG[miniK - MK_SEANCE];
    if (miniEnd) { if (!--miniEnd) timeUp(miniWin >= g[0]); return; }
    miniF++;
    bool busy = miniT;
    if (miniT) miniT--;
    (void)busy; (void)up; (void)dn; (void)lf; (void)rt; (void)a;
#if NEW_RAVEN
    if (miniK == MK_RAVEN) {                             // a hand catches the aces and dodges the knives
        int x = miniSel + (chgame.pressed(RIGHT_BUTTON) - chgame.pressed(LEFT_BUTTON)) * 3;
        miniSel = (uint8_t)(x < 0 ? 0 : x > 112 ? 112 : x);
        if (!--miniP) {
            miniP = (uint8_t)(18 + rnd() % 9);
            for (int i = 0; i < 6; i++) if (miniC[i].r == 255) { miniC[i].r = 0; miniC[i].s = (uint8_t)(rnd() % 8 + (rnd() % 5 < 2 ? 8 : 0)); break; }
        }
        for (int i = 0; i < 6; i++) {
            Card &c = miniC[i];
            if (c.r == 255) continue;
            c.r += 2 + miniWin / 6;
            int y = c.r - 24, d = (c.s & 7) * 16 - miniSel;
            if (y > 84 && y < 112 && d > -14 && d < 14) {
                c.r = 255;
                if (c.s & 8) { miniLoss++; flashT = 3; playSfx(Sfx::No); burst(miniSel + 8, 106, 8, P_RED, P_RED, 20); } else { miniWin++; playSfx(Sfx::Pop, (uint8_t)(miniWin & 7)); }
            } else if (y >= 128) c.r = 255;
        }
    } else
#endif
#if NEW_COUNT
    if (miniK == MK_COUNT) {                             // miniA: 0 the ask, 1 the cards, 2 you count, 3 the result
        if (miniA == 0) { if (miniF > 50) { miniA = 1; miniF = 0; } }
        else if (miniA == 1) {
            if (miniF % miniP == 2 && miniF / miniP < miniStep) playSfx(Sfx::Flick);
            if (miniF >= miniStep * miniP) { miniA = 2; miniF = 0; }
        } else if (miniA == 2) {
            miniSel = (uint8_t)((miniSel + up + 10 - dn) % 10);
            if (up || dn) playSfx(Sfx::Move);
            if ((a && miniF > 6) || miniF >= 180) {     // (a timeout is a wrong answer)
                if (!a) miniSel = 10;
                miniA = 3; miniF = 0;
                const bool ok = miniSel == miniC[0].r;
                if (!ok) flashT = 3;
                miniHit(ok, Sfx::Tada, Sfx::Wah);
            }
        } else if (miniF > 50) countRound();
    } else
#endif
#if NEW_GHOULS
    if (miniK == MK_GHOULS) {                            // GHOULS: whack them as they pop up
        if (lf && miniSel % 3) miniSel--;
        if (rt && miniSel % 3 < 2) miniSel++;
        if (up && miniSel > 2) miniSel -= 3;
        if (dn && miniSel < 6) miniSel += 3;
        if (up || dn || lf || rt) playSfx(Sfx::Move);
        Card &h = miniC[miniSel];
        if (a) { if (h.r && h.s != 255) { h.s = 255; h.r = 12; miniWin++; playSfx(Sfx::Boing); burst(22 + miniSel % 3 * 42, 52 + miniSel / 3 * 28, 8, P_RED, P_SKIN, 24); } else playSfx(Sfx::Snap); }
        if (!--miniP) {
            miniP = (uint8_t)(40 - miniWin);
            Card &n = miniC[rnd() % 9];
            if (!n.r) { n.r = (uint8_t)(74 - miniWin * 2); n.s = 0; }
        }
        for (int i = 0; i < 9; i++) {
            Card &c = miniC[i];
            if (c.r && c.s != 255 && c.s < 15) c.s++;
            if (c.r && !--c.r && c.s != 255) { miniLoss++; flashT = 2; playSfx(Sfx::No); }
        }
    } else
#endif
    {}
    if (miniWin >= g[0] || miniLoss >= g[1]) miniEnd = 40;
}
#endif
static void miniUpdate(bool up, bool dn, bool lf, bool rt, bool a) {
    if (timer < 255) timer++;
#if NEW_VM
    if (miniK >= MK_VM) { vmUpdate(); return; }
#endif
#if NEW_SCAFFOLD
    if (miniK >= MK_SEANCE) { newUpdate(up, dn, lf, rt, a); return; }
#endif
    if (miniT) {                                         // a reveal is showing: then the next step
        if (--miniT) return;
        if (miniK == MK_HILO) { if (++miniStep == 5) { timeUp(miniWin >= 4); return; } }
        else if (miniK == MK_DUEL) {
            if (miniWin == 2 || miniLoss == 2) { timeUp(miniWin == 2); return; }
            shuffleDeck(); for (int i = 0; i < 5; i++) miniC[i] = deck[i];
            miniA = miniB = 255;
        } else {
            if (miniC[miniA].r != miniC[miniB].r && miniDone != 0xFFF) {
                miniUp &= ~((1u << miniA) | (1u << miniB));
                uint16_t fr = ~miniSeen & 0xFFF;                // a miss: two cards that have never been turned swap places (a turned card stays put)
                if (fr & (fr - 1)) {
                    int i, j;
                    do i = rnd() % 12; while (!(fr >> i & 1));
                    do j = rnd() % 12; while (!(fr >> j & 1) || j == i);
                    Card t = miniC[i]; miniC[i] = miniC[j]; miniC[j] = t;
                }
            }
            miniA = miniB = 255;
            if (miniLoss >= 4) { timeUp(false); return; }
            if (miniDone == 0xFFF) { timeUp(true); return; }
        }
        return;
    }
    if (miniK == MK_HILO) {
        if (!up && !dn) return;
        uint8_t now = miniC[miniStep].r, next = miniC[miniStep + 1].r;
        bool ok = next == now || (up ? next > now : next < now);    // a tie counts for you
        miniHit(ok, Sfx::Pop, Sfx::No);
        miniSel = up ? 1 : 2; miniT = 28;
    } else if (miniK == MK_DUEL) {
        nav(miniSel, 4, lf, rt);
        if (!a) return;
        miniA = miniSel; miniB = (uint8_t)((miniSel + 1 + rnd() % 4) % 5);
        Card p = miniC[miniA], d = miniC[miniB];
        miniHit(p.r >= d.r, Sfx::Tada, Sfx::Wah);
        miniT = 40;
    } else {
        if (lf && miniSel % 6) miniSel--;
        if (rt && miniSel % 6 < 5) miniSel++;
        if ((up || dn) ) miniSel = (uint8_t)((miniSel + 6) % 12);
        if (lf || rt || up || dn) playSfx(Sfx::Move);
        if (!a || (miniUp >> miniSel & 1)) return;
        miniUp |= 1u << miniSel; miniSeen |= 1u << miniSel;
        playSfx(Sfx::Flick);
        if (miniA == 255) { miniA = miniSel; return; }
        miniB = miniSel;
        if (miniC[miniA].r == miniC[miniB].r) { miniDone |= (1u << miniA) | (1u << miniB); playSfx(Sfx::Pop); miniT = 6; }
        else { miniLoss++; miniT = 20; }
    }
}

// a small card, face up or down (16x22)
static void smallCard(int x, int y, Card c, bool up, uint8_t edge) {
    frame(x, y, 16, 22, edge, up ? P_CARD : P_WINE);
    if (!up) { gfx_rect(x + 3, y + 3, 10, 16, P_SHADE); pix(x + 8, y + 11, P_RED); return; }
    rankS(c.r, x + 2, y + 2, suitCol(c.s), 4);
    suit7(c.s, x + 7, y + 12, suitCol(c.s));
}
// ---- GRAVES: inside a coffin full of snakes. The far end sways with the creak (the depth); the lid has a crack of
// light; bloody tufted satin on the walls; snakes far (small, dark, slow) to near (big, fast) with tongues and eyes.
static void coffin() {
    int sw = (isinK(2) >> 5), bx = 36 + sw;
    gfx_fillRect(0, 12, 128, 116, P_WINE);
    for (int y = 12; y < 128; y++) {
        bool lid = y < 46;
        if (!lid && y < 83) continue;
        int d = lid ? (y - 12) * 36 / 34 : (127 - y) * 36 / 44, l = d * bx / 36;
        hline(l, y, 128 - l - d * (36 - sw) / 36, lid ? P_GOLDDK : P_DARK);
    }
    gfx_fillRect(bx, 46, 56, 37, P_BG);
    for (int k = 0; k < 5; k++) {
        gfx_line(k * 32 - (k == 4), 12, bx + k * 14, 46, k == 2 && (fc % 70) > 3 ? P_GLOW : P_INK);   // lid seams, the crack
        gfx_line(k * 32 - (k == 4), 127, bx + k * 14, 83, P_INK);                                       // floor planks
        if (k & 1) pix(k * 32 + (bx + k * 14 - k * 32) / 3, 23, P_GREY);                            // nail heads
        if (k && k < 4) for (int r = 0; r < 2; r++) {                                                   // satin folds and buttons
            int ex = r ? 127 : 0, wx = r ? bx + 56 : bx, ey = 12 + k * 29, wy = 46 + k * 9;
            gfx_line(ex, ey, wx, wy, P_RED); pix((ex + wx) / 2, (ey + wy) / 2 + 4, P_GOLD);
        }
    }
    gfx_fillCircle(12, 96, 4, P_RED); gfx_vline(11, 100, 14, P_RED); gfx_fillCircle(110, 70, 3, P_RED);   // bitten: blood on the lining
    hline(4, 116, 4, P_GOLD); hline(118, 108, 4, P_GOLD);   // slit eyes in the corners
}
static void snakes(int from, int to) {
    __attribute__((section(".rodata.sny"))) static const uint8_t SNY[5] = { 50, 66, 24, 86, 117 };
    for (int i = from; i < to; i++) {
        int sz = 2 + i / 2, dir = (i & 1) ? 1 : -1, hx = (fc * sz / 2 + i * 53) % 190 - 30, hy = 0;
        if (dir < 0) hx = 128 - hx;
        for (int k = 11; k >= 0; k--) {                   // tail first, the head on top
            int x = hx - dir * k * sz, y = SNY[i] + (isinF(4 + sz, k * 28) * (2 + sz) >> 6);
            if (i == 3 && k == 11) x += fc & 1;           // a rattle
            gfx_fillRect(x, y, sz + 1, sz + 1, (k & 1) || i < 2 ? P_FELT : P_FELTLT);
            hline(x, y + sz, sz + 1, i < 2 ? P_DARK : P_SKIN);
            hy = y;
        }
        int ex = dir > 0 ? hx + sz : hx;
        pix(ex, hy, P_RED);
        if (((fc + i * 11) & 15) < 4) hline(dir > 0 ? hx + sz + 1 : hx - 3, hy + sz / 2, 3, P_RED);   // the tongue flicks
    }
}
#if NEW_SCAFFOLD
static void eyes(int xl, int xr, int y, int w, int h, uint8_t c) { gfx_fillRect(xl, y, w, h, c); gfx_fillRect(xr, y, w, h, c); }
// ---- the newer stages' pictures (all drawn by code: no picture data)
NEWFN void drawNew() {
    int k = miniK, cx = 64, cy = 82;
    (void)cx; (void)cy;
#if NEW_RAVEN
    if (k == MK_RAVEN) {                                 // aces and knives fall; a hand below
        int rx = fc * 2 % 170 - 20, ry = 38 + isinK(5) / 12, fl = isinK(24) >> 4;
        gfx_line(rx - 6, ry + fl, rx, ry + 2, P_INK); gfx_line(rx + 6, ry + fl, rx, ry + 2, P_INK); gfx_fillRect(rx - 1, ry + 1, 3, 3, P_INK);
        gfx_fillRect(0, 124, 128, 4, P_DARK);
        for (int i = 0; i < 6; i++) {
            Card &c = miniC[i];
            if (c.r == 255) continue;
            int x = (c.s & 7) * 16, y = c.r - 24;
            if (c.s & 8) { gfx_fillRect(x + 7, y, 2, 13, P_TEXT); gfx_fillRect(x + 5, y + 13, 6, 3, P_RED); pix(x + 7, y - 1, P_TEXT); }
            else smallCard(x, y, { 12, (uint8_t)(i & 3) }, true, P_GOLD);
        }
        int hx = miniSel;
        gfx_fillRect(hx, 112, 16, 8, P_SKIN);
        for (int f = 0; f < 4; f++) { gfx_fillRect(hx + f * 4, 105 + (f == 0 || f == 3) * 3, 3, 8, P_SKIN); pix(hx + f * 4 + 1, 104 + (f == 0 || f == 3) * 3, P_RED); }
    } else
#endif
#if NEW_COUNT
    if (k == MK_COUNT) {                                 // the cards flash by one at a time, two blank frames between
        int i = miniF / miniP;
        textOutCT(35, 3, TX_C_ASK + miniB, P_GOLD);
        if (miniA == 0) { drawBack(52, 52, CW); textOutCT(100, 7, TX_N_WATCH, P_GLOW); }
        else if (miniA == 1) { if (i < miniStep && miniF % miniP > 1) drawFace(52, 52, deck[i], F_NONE); }
        else if (miniA == 2) {                           // your number, UP/DOWN; the time bar runs down
            char b[2] = { (char)('0' + miniSel), 0 };
            textOutC(52, b, P_TEXT, 4);
            gfx_fillRect(14, 98, (180 - miniF) * 100 / 180, 3, miniF > 120 ? P_RED : P_GLOW);
            textOutCT(108, 3, TX_C_LOCK, P_CARD);
        } else {
            bool ok = miniSel == miniC[0].r;
            textOutCT(52, 3, ok ? TX_RIGHT : TX_WRONG, ok ? P_GOLD : P_RED, 2);
            textOutC(80, strf(cardText(3, TX_C_WAS), miniC[0].r), P_TEXT);
        }
    } else
#endif
#if NEW_GHOULS
    if (k == MK_GHOULS) {                                // nine graves
        for (int i = 0; i < 9; i++) {
            Card &c = miniC[i];
            int x = 22 + i % 3 * 42, y = 52 + i / 3 * 28;
            rr3(x - 6, y - 14, 12, 16, P_SHADE); hline(x - 3, y - 8, 7, P_GREY); gfx_vline(x, y - 11, 8, P_GREY);
            if (c.r && c.s != 255) {
                int h = c.s < c.r ? c.s : c.r; if (h > 11) h = 11;   // a little person climbs out: 9 x 11, head, coat, hands up
                int ty = y + 8 - h;
                gfx_fillRect(x - 4, ty, 9, 11, P_INK); gfx_fillRect(x - 3, ty + 1, 7, 4, P_SKIN); gfx_fillRect(x - 3, ty + 5, 7, 5, P_WINE);
                gfx_fillRect(x - 4, ty + 5, 1, 3, P_SKIN); gfx_fillRect(x + 4, ty + 5, 1, 3, P_SKIN);
                eyes(x - 1, x + 1, ty + 2, 1, 1, P_RED); pix(x, ty + 4, P_WINE);
            } else if (c.r) gfx_fillCircle(x, y + 4, 3 + c.r / 3, P_RED);
            rr3(x - 13, y + 8, 26, 12, P_DARK);
        }
        gfx_roundRect(8 + miniSel % 3 * 42, 36 + miniSel / 3 * 28, 28, 30, 3, (fc & 4) ? P_GLOW : P_GOLD);
    } else
#endif
    {}
    textOutC(26, strf(cardText(7, TX_N_SCORE + k - MK_SEANCE), miniWin, miniLoss), P_TEXT);
}
#endif
static int pairsDone() { int n = 0; for (uint16_t v = miniDone; v; v &= v - 1) n++; return n / 2; }
static void drawMini() {
    bool box = miniK == MK_PAIRS;
    if (box) { coffin(); snakes(0, fxLow ? 4 : 5); } else drawFelt();   // (all the snakes behind the cards and the text)
    drawTopBar();
    dealerTick();
#if NEW_SCAFFOLD
    textOutC(14, miniName(miniK), P_GOLD);
#else
    textOutCT(14, 3, TX_MINI + miniK, P_GOLD);
#endif
    drawDealerHead(102, 9, 8);
#if NEW_VM
    if (miniK >= MK_VM) { vmDraw(); return; }
#endif
#if NEW_SCAFFOLD
    if (miniK >= MK_SEANCE) { drawNew(); return; }
#endif
    if (miniK == MK_HILO) {
        for (int i = 0; i < 6; i++) {
            bool up = i <= miniStep || (i == miniStep + 1 && miniT);
            int x = 4 + i * 20, y = 46 - (i == miniStep ? 6 : 0);
            smallCard(x, y, miniC[i], up, i == miniStep ? P_GOLD : P_INK);
        }
        textCF(80, 4, TX_S_HILO, miniWin, miniLoss, P_TEXT);
        if (!miniT) { textCT(96, 3, TX_UP, P_GLOW); textCT(106, 3, TX_DOWN, P_GLOW); }
        else textOutC(98, miniC[miniStep + 1].r == miniC[miniStep].r ? cardText(3, TX_TIE) : cardText(3, (miniSel == 1) == (miniC[miniStep + 1].r > miniC[miniStep].r) ? TX_RIGHT : TX_WRONG), P_GOLD, 2);
    } else if (miniK == MK_DUEL) {
        for (int i = 0; i < 5; i++) {
            bool shown = i == miniA || i == miniB;
            int x = 8 + i * 23, y = 40 - (i == miniSel && !miniT ? 5 + (isinK(8) >> 5) : 0) + (shown ? -8 : 0);
            smallCard(x, y, miniC[i], shown && miniT < 34, i == miniA ? P_GOLD : i == miniB ? P_RED : P_INK);
            if (shown && miniT < 34) gfx_text(x + 2, y + 24, i == miniA ? "YOU" : "ME", i == miniA ? P_GOLD : P_RED);
        }
        textCF(82, 4, TX_S_DUEL, miniWin, miniLoss, P_TEXT);
        if (!miniT) textCT(98, 3, TX_LRPICK, P_GLOW);
        else if (miniT < 34) textOutCT(96, 3, miniC[miniA].r >= miniC[miniB].r ? TX_YOUTAKE : TX_ITTAKES, P_GOLD, 1);
    } else {
        for (int i = 0; i < 12; i++) {
            int x = 6 + (i % 6) * 20, y = 32 + (i / 6) * 26;
            bool up = (miniUp >> i) & 1, done = (miniDone >> i) & 1;
            smallCard(x, y, miniC[i], up, i == miniSel ? P_GLOW : done ? P_GOLD : P_INK);
        }
        textOutC(90, strf(cardText(4, TX_S_PAIRS), pairsDone(), miniLoss), P_TEXT);
        textOutCT(102, 3, TX_ARROWS, P_CARD);
    }
}

#if CHGAME_DEBUG && !defined(PERF_BUILD)
// debug `say I 100`: ascending ranks (HIGH OR LOW: UP wins, DOWN loses; THE DUEL: the right card beats the others, the left loses), PAIRS: all found, the speed run: the goal met
// n 101: a sure loss, 102: an easy play (the newer stages: 100/101 jump the counters to the goal; 102 THE COUNT: sets the right number; the scripts: r8 = 1, SEANCE only UP, GALLOWS and DEAD WHEEL any stop hits)
static void cheatK(int n) {
#if NEW_VM
    if (miniK >= MK_VM) { if (vm) { if (n < 102) vmEnd(n == 100); else vm->R[8] = 1; } return; }
#endif
#if NEW_SCAFFOLD
    if (miniK >= MK_SEANCE) {
        if (n == 100) miniWin = MG[miniK - MK_SEANCE][0];
        else if (n == 101) miniLoss = MG[miniK - MK_SEANCE][1];
        else if (miniK == MK_COUNT) miniSel = miniC[0].r;
        else { for (int i = 0; i < 12; i++) miniC[i].s = 0; miniB = 200; }
        return;
    }
#endif
    if (n != 100) return;
    if (miniK == MK_PAIRS) miniDone = 0xFFF; else for (int i = 0; i < 12; i++) miniC[i].r = (uint8_t)(i * 2);
    score = 9999;
}
#endif
