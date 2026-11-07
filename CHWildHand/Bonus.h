// Bonus.h: the v1.0 screens and HUD parts: the dealer's deal (DEAL), the bonus stage's card show and trick
// (BONUS), the end of a timed play (TIMEUP), the hand-levels table, the timer bar, the pause menu with its
// settings, the title menu, and the sounds for Gfx.h's gag hooks. Included after Gfx.h and Dealers.h.
#pragma once

static void drawMini();
#define RANKCH ((const char *)(wh_tabs + T_RANKCH))   // (in RAM, from the card: tools/tables.py)
// text block 2 (tools/card_text.py BONUS): (twelve blank lines), the prizes, the stages' rules, the outcomes
enum { TX_DNAME = 0, TX_DEDGE = NDEAL, TX_PRIZE = 12, TX_RULE = 15, TX_LOSE = 23, TX_GOLD, TX_NOROOM, TX_NEWWILD, TX_X2, TX_HALF, TX_MINUS1 };
enum { TX_MINI = 0, TX_UP = TX_MINI + 4, TX_DOWN, TX_TIE, TX_RIGHT, TX_WRONG, TX_YOUTAKE, TX_ITTAKES, TX_LRPICK, TX_ARROWS, TX_CARDSZ, TX_EQ = TX_CARDSZ + 5, TX_AUTO = TX_EQ + 9, TX_C_ASK, TX_C_WAS = TX_C_ASK + 6, TX_C_LOCK, TX_C_IDX, TX_C_IDXEND = TX_C_IDX + 4 };   // (block 3 ends with THE COUNT's six asks, IT WAS, the lock hint)
// text block 7 (tools/card_text.py MINI2): the newer stages' names, rule lines, score formats, then WATCH, YOUR TURN, HIT, MISS
enum { TX_N_NAME = 0, TX_N_RULE = NNEW, TX_N_SCORE = 3 * NNEW, TX_N_WATCH = 4 * NNEW, TX_N_TURN, TX_N_HIT, TX_N_MISS };
// a stage's name and its two rule lines: the first four in text blocks 3 and 2, the newer ones in block 7
#if NEW_SCAFFOLD
static const char *miniName(uint8_t k) { return k < MK_SEANCE ? cardText(3, TX_MINI + k) : k >= MK_VM ? cardText(STAGE_TXG, k - MK_VM) : cardText(7, k - MK_SEANCE); }
static const char *miniRule(uint8_t k, uint8_t n) { return k < MK_SEANCE ? cardText(2, TX_RULE + k * 2 + n) : k >= MK_VM ? cardText(STAGE_TXG, (n + 1) * STAGE_N + k - MK_VM) : cardText(7, TX_N_RULE + k - MK_SEANCE + n * NNEW); }
#else
static const char *miniName(uint8_t k) { return cardText(3, TX_MINI + k); }
static const char *miniRule(uint8_t k, uint8_t n) { return cardText(2, TX_RULE + k * 2 + n); }
#endif

// ---- small parts
static void arrow(int x, int y, uint8_t col) { gfx_text(x + (isinK(8) > 0), y, ">", col); }   // a menu's cursor, nudging
static void prompt2(int xl, int xr, int y, uint8_t i) { gfx_text(xl, y, cardText(4, i), (fc & 8) ? P_GLOW : P_GOLD); textR(xr, y, cardText(4, i + 1), P_GREY); }   // go (blinking) left, back right: text block 4's lines i, i + 1
static_assert(TX_B_FLEE == TX_B_ENTER + 1 && TX_T_BACK == TX_T_PLAY + 1, "prompt2's pairs");
// lo: the bar's text ends here; hi: the money starts here (the extras sit between, right to left)
static int16_t barLo, barEnd;
static void topBarExtra(int lo, int hi) {
    if (x2Now && !timed && state != SHOP) { hi -= 15; rr2(hi, 1, 15, 9, P_RED); gfx_text(hi + 2, 2, "X2", P_TEXT); hi -= 3; }
    if (state == SHOP) { hi -= 38; btnGlyph(hi, 1, 'B'); for (int k = 0; k < JMAX; k++) gfx_fillRoundRect(hi + 12 + k * 5, 3, 4, 5, 1, k < nJok ? P_GOLD : P_DARK); }   // B: your cards (the slots filled)
    if ((dealer == 0 || hasJoker(38)) && !timed && (state == PLAY || state == RESULT) && deckPos < 52) {   // the beholder (or the seer bone) sees the next card
        int x = hi -= 12;
        char s[2] = { RANKCH[deck[deckPos].r], 0 };
        gfx_text(x, 2, s, P_TEXT);
        suit5(deck[deckPos].s, x + 7, 3, deck[deckPos].s < 2 ? P_RED : P_GREY);
    }
    barLo = lo; barEnd = hi;   // (what is left for playIdle's guidance)
}
static void drawTimer() {                              // a timed play: the time left in place of the pips
    int total = bonusOn ? BONUS_FRAMES : TURBO_FRAMES;
    int t = tLeft > total ? total : tLeft;
    bool low = tLeft < 150;
    textOut(3, PIPS_Y + 1, miscText(TX_M_TIME), P_CARD);                 // (two rows, left of the head: TIME and the seconds, then the bar)
    rr2(2, PIPS_Y + 11, 40, 9, P_INK);
    int w = 36 * t / total;
    if (w > 0) gfx_fillRect(4, PIPS_Y + 14, w, 3, low && (fc & 4) ? P_RED : P_GOLD);
    textOut(30, PIPS_Y + 1, strf("%d", (tLeft + 29) / 30), low ? P_RED : P_TEXT);
}
static const char *bossLine() {                              // the boss's twist, for the deal and the table
    return boss == 0 ? strf(miscText(TX_M_DEAD), miscText(TX_M_SUIT + deadSuit)) : strf(miscText(TX_M_HEXC), miscText(TX_M_BOSS + boss));
}
static void playIdle() {                              // the hint line over the hand, outlined on the backdrop
    const char *s; uint8_t col = P_GLOW;
    if (timed) { s = miscText(bonusOn ? TX_M_PAIRS : TX_M_RUN); col = (fc & 8) ? P_GLOW : P_GOLD; }
    else if (boss != 255) s = bossLine();
    else { col = P_CARD; s = (fc / 90) & 1 ? miscText(TX_M_LEVELS) : strf(miscText(TX_M_PICK), maxPick); }
    int x = 4, y = HINT_Y;   // the top bar, next to NIGHT n, when it fits; else the old line over the hand
    if (barLo + gfx_textWidth(s) < barEnd) x = barLo, y = 2;
    textOut(x, y, s, col);
}

// ---- the dealer deals (each round)
__attribute__((noinline)) static void drawDeal() {
    drawFelt();
    drawTopBar();
    dealerTick();
    int t = timer;
    if (t == 1) { static const Sfx V[NDEAL] = { Sfx::Bleep, Sfx::Blub, Sfx::Whistle, Sfx::Tada, Sfx::Arr, Sfx::Puff, Sfx::Whoosh, Sfx::Boing, Sfx::Tick }; playSfx(V[dealer]); }
    if (t == 8) playSfx(Sfx::Shuffle);
    // the eight places on the table
    for (int k = 0; k < HANDN; k++) gfx_roundRect(4 + k * 15, 98, 10, 14, 1, P_DARK);
    drawDealerScene(t, 64, 52);
    if (t > 20) {
        textOutCT(79, 8, TX_DNAME + dealer, P_GOLD);     // (text block 8: the nine dealers)
        textOutCT(89, 8, TX_DEDGE + dealer, P_TEXT);
    }
    if (boss != 255 && t > 30) {
        const char *s = bossLine();
        int w = gfx_textWidth(s) + 8;
        rr2((128 - w) / 2 - 6, 115, w, 11, P_RED);
        gfx_text((128 - w) / 2 - 2, 117, s, P_TEXT);
        blinkA();
    } else if (t > 100) blinkA();
}

// ---- the bonus stage's show: cards fly in arcs and circles, then a trick: fan out, riffle cascade, the goal card flips
enum { BT_FAN = 70, BT_RIFFLE = 110, BT_FLIP = 150, BT_PROMPT = 200 };
__attribute__((noinline)) static void selRow(int x, int ry, int w, int h, uint8_t c, int ax, int ay, uint8_t ac) { rr2(x, ry, w, h, c); arrow(ax, ay, ac); }
static void drawPrize(int x, int y, int w) {
    if (w < 2) return;
    frame(x, y, w, CH, P_GOLD, P_GLOW);
    if (w < CW) return;
    gfx_rect(x + 2, y + 2, w - 4, CH - 4, P_GOLDDK);
    if (bonusPrize == 1) { drawArt(wildArt(7), x + 4, y + 9); return; }
    textOut(x + (CW - gfx_textWidthScaled(bonusPrize ? "X2" : "$", 2)) / 2, y + 10, bonusPrize ? "X2" : "$", P_RED, 2);
}
static void drawBonusShow() {
    drawFelt();
    drawTopBar();
    int t = timer, cx = 52, cy = 58;
    dealerTick();
    drawDealerHead(40, 12 + (isinK(8) >> 5), 16);
    if (t == 35 || t == 1) playSfx(Sfx::Whoosh);
    if (t == BT_RIFFLE) playSfx(Sfx::Reroll);
    if (t > BT_FLIP && t < BT_FLIP + 16 && !(t & 1)) playSfx(Sfx::Tick, (uint8_t)((t - BT_FLIP) / 2));   // a drum roll
    if (t == BT_FLIP + 30) { playSfx(Sfx::Tada); burst(64, 50, 16, P_GOLD, P_GLOW, 36); flashT = 3; }
    if (t < BT_FAN) {                                    // circles: the cards spiral in from the edges and close into one deck
        int r = (BT_FAN - t) * 5 / 4;
        for (int k = 0; k < 8; k++) {
            int a = t * 10 + k * 32;
            drawBack(cx + (isin(a + 64) * r >> 6), cy + (isin(a) * r * 2 / 3 >> 6), CW);
        }
    } else if (t < BT_RIFFLE) {                          // the fan: seven cards spread in an arc
        int f = t - BT_FAN; if (f > 16) f = 16;
        for (int k = 0; k < 7; k++) {
            int d = k - 3;
            int x = cx + d * 13 * f / 16, y = cy + 4 + d * d * 2 * f / 16;
            drawBack(x, y, CW);
            if (k == 3 && f == 16) gfx_roundRect(x, y, CW, CH, 2, P_GOLD);
        }
    } else if (t < BT_FLIP) {                            // the riffle: the halves cascade back down, interleaving
        int f = t - BT_RIFFLE;
        for (int k = 0; k < 8; k++) {
            int s = f - k * 4; s = s < 0 ? 0 : s > 10 ? 10 : s;      // (each half held up, then dropping in turn)
            drawBack(cx + ((k & 1) ? 10 - s : s - 10), cy - 40 + s * 4 + k / 2, CW);
        }
    } else {                                             // the flip: the top card rises and turns over
        drawBack(cx, cy + 4, CW);
        int f = t - BT_FLIP, y = cy - (f < 20 ? f : 20);
        if (f < 20) drawBack(cx, y, CW);
        else if (f < 25) { int w = CW * (25 - f) / 5; drawBack(cx + (CW - w) / 2, y, w); }
        else if (f < 30) { int w = CW * (f - 25) / 5; drawPrize(cx + (CW - w) / 2, y, w); }
        else drawPrize(cx, y, CW);
    }
    if (t >= BT_FLIP + 30) {
        panel(1, 74, 126, 54);
        textOutCT(78, 2, TX_PRIZE + bonusPrize, P_GOLD);
#if NEW_SCAFFOLD
        textC(89, miniK == MK_SPEED ? strf(cardText(4, TX_B_SPEED), (long)bonusGoal) : miniRule(miniK, 0), P_TEXT);
        textC(98, miniRule(miniK, 1), P_GREY);
#else
        textC(89, miniK == MK_SPEED ? strf(cardText(4, TX_B_SPEED), (long)bonusGoal) : cardText(2, TX_RULE + miniK * 2), P_TEXT);
        textCT(98, 2, TX_RULE + 1 + miniK * 2, P_GREY);
#endif
        textCT(107, 2, TX_LOSE, P_RED);
        if (t >= BT_PROMPT) prompt2(8, 119, 117, TX_B_ENTER);
#if NEW_SCAFFOLD
    } else textOutC(116, t < BT_FAN ? cardText(4, TX_B_NIGHT) : miniName(miniK), (fc & 4) ? P_GLOW : P_GOLD, 1);
#else
    } else textOutC(116, t < BT_FAN ? cardText(4, TX_B_NIGHT) : cardText(3, TX_MINI + miniK), (fc & 4) ? P_GLOW : P_GOLD, 1);
#endif
}

// ---- after a timed play
static void drawTimeUp() {
    drawFelt();
    drawTopBar();
    bool good = bonusWon;
    if (timer < 2 && good) burst(64, 40, 24, P_GOLD, P_GLOW, 40);
    if (good && (fc & 7) == 0) confetti();
    if (timer == 1) dealerReact(good);
    dealerTick();
    drawDealerHead(40, 12, 16);
    panel(6, 48, 116, 62);
    if (mode == M_TURBO) {
        textOutCT(53, 4, TX_U_TIME, P_GOLD, 2);
        textCF(74, 4, TX_U_SCORE, (int)score, 0, P_TEXT);
        textCF(85, 4, TX_U_BEST, (int)turboHi, 0, good ? P_GLOW : P_GREY);
        if (good && (fc & 8)) textCT(96, 4, TX_U_NEWBEST, P_GOLD);
    } else {                                             // the stage survived (the prize) or caught (the cost)
        textOutCT(53, 4, good ? TX_U_SURV : TX_U_CAUGHT, good ? P_GOLD : P_RED, 2);
        textCT(76, 2, good ? TX_PRIZE + bonusPrize : TX_HALF, good ? P_GLOW : P_TEXT);
        textCT(88, 2, !good ? TX_MINUS1 : bonusPrize == 0 ? TX_GOLD : bonusPrize == 2 ? TX_X2 : prizeFull ? TX_NOROOM : TX_NEWWILD, P_TEXT);
    }
    if (timer > 30 && (fc % 40) < 28) textOutCT(114, 4, TX_U_PRESS, P_GLOW);
}

#ifndef PERF_LEAN
// ---- the title's BONUS: pick a bonus stage to play alone (the stage's rules under the list)
static void drawBsel() {
    drawFelt();
    panel(4, 6, 120, 116);
    textOutCT(11, 4, TX_T_STAGES, P_GOLD);
#if NEW_SCAFFOLD
    int top = bselSel < 2 ? 0 : bselSel > NPICK - 3 ? NPICK - 5 : bselSel - 2;      // five rows show, the list scrolls
    for (int r = 0; r < 5; r++) {
        int i = top + r, y = 24 + r * 11;
        if (i == bselSel) { selRow(10, y - 2, 100, 11, (fc & 16) ? P_RED : P_WINE, 14, y, P_TEXT); }
        textC(y, miniName(pickK(i)), P_TEXT);
    }
    gfx_fillRect(116, 22, 3, 56, P_DARK); gfx_fillRect(116, 22 + bselSel * 46 / (NPICK - 1), 3, 10, P_GOLD);   // the scroll bar
#else
    for (int i = 0; i < MK_SEANCE; i++) {
        int y = 28 + i * 12;
        if (i == bselSel) { selRow(10, y - 2, 108, 11, (fc & 16) ? P_RED : P_WINE, 14, y, P_TEXT); }
        textCT(y, 3, TX_MINI + i, P_TEXT);
    }
#endif
#if NEW_SCAFFOLD
    uint8_t k = pickK(bselSel);
    textC(84, miniRule(k, 0), P_GREY);
    textC(94, miniRule(k, 1), P_GREY);
#else
    textCT(84, 2, TX_RULE + bselSel * 2, P_GREY);
    textCT(94, 2, TX_RULE + 1 + bselSel * 2, P_GREY);
#endif
    prompt2(10, 117, 110, TX_T_PLAY);
}
#endif
__attribute__((noinline)) static void drawExtra() {
    if (state == DEAL) drawDeal();
    else if (state == BONUS) drawBonusShow();
    else if (state == MINI) drawMini();
#ifndef PERF_LEAN
    else if (state == BSEL) drawBsel();
#endif
    else drawTimeUp();
}

// the pause menu's and the levels table's lines in text block 1, after the 24 descriptions (tools/card_text.py CALM)
enum { TX_RESUME = 20, TX_SAVEGAME, TX_QUIT, TX_PAUSED, TX_CALM, TX_FULLSHOW, TX_LEVELS, TX_RISE };
// ---- the hand-levels table (DOWN in a round)
static void drawOverlay() {
    gfx_wait();
    gfx_clear(P_BG);
    drawTopBar();
    panel(3, 13, 122, 110);
    textOutCT(16, 1, TX_LEVELS, P_GOLD);
    for (int i = 0; i < 9; i++) {
        int y = 27 + i * 9;
        gfx_text(7, y, miscText(TX_M_HAND + i), P_TEXT);
        gfx_text(70, y, strf("L%d", hlv[i] + 1), hlv[i] ? P_GLOW : P_GREY);
        textR(120, y, strf("%dX%d", handChips(i), handMult(i)), P_GOLD);
    }
    int y = 109;
    for (int k = 0; k < 4; k++) if (comboOn(k)) { textC(y, strf("%s!", cardText(4, TX_COMBO + k)), P_RED); break; }
    if (!nJok) textCT(110, 1, TX_RISE, P_GREY);     // (no wild cards: no combos either)
}

// ---- pause menu and settings
static void drawPauseMenu() {
    panel(8, 4, 112, 122);
    textOutCT(7, 1, TX_PAUSED, P_GOLD, 2);
    const int top = pauseSel > 9 ? pauseSel - 9 : 0;          // ten lines show: the list scrolls under the cursor
    for (int i = 0; i < 10; i++) {
        const int y = 26 + i * 9, L = i + top;
        static_assert(TX_P_MUSIC == 7 && TX_P_FX == 8 && TX_P_CARD == 9 && TX_P_INDEX == 10 && TX_P_EQ == 13 && TX_POFF == 14 && TX_FULL2 == 16 && TX_CARDSZ == 13 && TX_EQ == 18 && TX_RESUME == 20, "PM in tools/tables.py");
        #define PM ((const uint8_t (*)[4])(wh_tabs + T_PM))   // a line: the block and line of its text, the block and first line of its values (in RAM, from the card: tools/tables.py)
        const uint8_t *m = PM[L];
        const char *s = cardText(m[0], m[1]);
#ifdef PERF_LEAN
        if (L > 3 && L < 7) s = "";                           // (the board's debug build has no INDEX, SONG or EQ)
        else
#endif
        if (L == 5) s = songSel ? strf(cardText(4, TX_P_SONGN), songSel) : strf(s, cardText(3, TX_AUTO));
        else if (L && L < PL_SAVE) s = strf(s, cardText(m[2], L == 2 && fxLow > 1 ? TX_POFF : m[3] + sett[L - 1]));
        if (L == pauseSel) { selRow(9, y - 1, 110, 9, P_GOLD, 12, y, P_INK); }
        textC(y, s, L == pauseSel ? P_INK : P_TEXT);
    }
    if (note) textCT(116, 4, TX_N_QUIT - N_QUIT + note, P_GOLD);
    else if (pauseSel == 2) textCT(116, 1, fxLow ? TX_CALM : TX_FULLSHOW, P_GREY);
}
static void drawTitleMenu() {
    int n = titleN();
    int y0 = n == 4 ? 96 : n == 3 ? 102 : 107, h = n == 4 ? 8 : 9;
    for (int i = 0; i < n; i++) {
        int y = y0 + i * h, k = titleK(i);
        const char *t = cardText(4, TX_T_CONT + k);
        if (!k) t = strf(t, sv.round_ + 1);
        if (titleSel == i) { selRow(18, y - 1, 92, h, (fc & 16) ? P_RED : P_WINE, 22, y, P_TEXT); }
        textC(y, t, P_TEXT);
    }
}
static void endPrompt(int y) {
    if ((fc % 40) >= 28) return;
    textOutCT(y, 4, state == WON ? TX_E_DEEPER : TX_U_PRESS, P_SHADE);
}

// ---- sounds for the gag hooks
static void gagHook(Gag g) {
    switch (g) {
    case Gag::Deal: playSfx(Sfx::Deal); break;
    case Gag::Tally: playSfx(Sfx::Pop); break;
    case Gag::Pop: case Gag::Peek: playSfx(Sfx::Bwip); break;
    case Gag::Twitch: if (state == DEAL || state == PLAY) playSfx(Sfx::Bwip, 5); break;
    case Gag::BigHand: playSfx(Sfx::Boing); break;
    case Gag::Riffle: playSfx(Sfx::Reroll); break;
    case Gag::HeatUp: playSfx(Sfx::HeatUp, (uint8_t)(heat >> 5)); break;
    case Gag::Beat: playSfx(Sfx::Beat); break;
    default: break;   // Pick, Drop, Play, Clear, Bust, Jackpot, Buy, Broke and Full already sound in update()
    }
}

// ---- debug commands, after Gfx.h's: G bonus show, U turbo, D dealer, Y boss twist, I levels, F effects, E time left (not S K L N P T Q B: the library's)
#if CHGAME_DEBUG
#ifndef PERF_BUILD
static void cheatK(int n);
// the regression: every hand x each of the first 24 wild cards (and the four combos, and three three-card stacks) x three situations, scored
// by scoreHand() itself; `say q` prints one line each (set, hand, situation, chips, mult, money). The output must equal the old switch's.
static void regress() {
    static const uint8_t H[14][11] = {   // n, then rank, suit a card
        { 5, 0, 0, 3, 1, 7, 2, 9, 3, 11, 0 }, { 1, 12, 3 }, { 2, 5, 0, 5, 1 }, { 2, 11, 3, 11, 2 }, { 5, 1, 0, 1, 3, 7, 1, 7, 2, 12, 0 },
        { 3, 2, 3, 2, 2, 2, 1 }, { 5, 2, 0, 3, 1, 4, 2, 5, 3, 6, 0 }, { 5, 0, 3, 2, 3, 4, 3, 8, 3, 11, 3 }, { 5, 12, 0, 12, 1, 12, 3, 9, 2, 9, 0 },
        { 4, 3, 0, 3, 1, 3, 2, 3, 3 }, { 5, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0 }, { 2, 1, 2, 12, 3 }, { 2, 10, 0, 10, 2 }, { 4, 0, 0, 0, 1, 4, 0, 4, 1 } };
    static const uint8_t STK[4][3] = { { 6, 0, 16 }, { 2, 5, 17 }, { 23, 13, 12 }, { 21, 3, 14 } };
    char b[64];
    dealer = 3; boss = 255; bonusOn = false; x2Now = false;
    for (int set = 0; set < 32; set++) {
        for (int hh = 0; hh < 14; hh++) for (int cx = 0; cx < 3; cx++) {
            clearJok();
            if (set < 24) addJok((uint8_t)set);
            else if (set < 28) { addJok(COMBO[set - 24][0]); addJok(COMBO[set - 24][1]); }
            else for (int i = 0; i < 3; i++) addJok(STK[set - 28][i]);
            timed = cx == 2; playsLeft = cx == 0 ? 4 : cx == 1 ? 1 : 99; playsDone = cx == 1 ? 2 : 0; discLeft = cx == 0 ? 3 : cx == 1 ? 0 : 99;
            money = cx == 0 ? 20 : cx == 1 ? 60 : 0; round_ = cx == 0 ? 2 : cx == 1 ? 5 : 0;
            for (int i = 0; i < 9; i++) hlv[i] = 0;
            Card c[5];
            for (int i = 0; i < H[hh][0]; i++) c[i] = { H[hh][1 + 2 * i], H[hh][2 + 2 * i] };
            scoreHand(c, H[hh][0]);
            snprintf(b, sizeof b, "Q %d %d %d %d %d %ld\n", set, hh, cx, lastChips, lastMult, (long)money);
            dbg::print(b);
        }
    }
    timed = false; round_ = 0; clearJok();
}
// the hand as text: "<tag> n r.s ..." (rank 0..12 = 2..A, suit; "h": face down); `say h` prints it, `say k n` sorts n test hands (tools/tests/sort_check.py)
static void handDump(const char *tag) {
    char b[80]; int l = snprintf(b, sizeof b, "%s %d", tag, nHand);
    for (int i = 0; i < nHand; i++) l += snprintf(b + l, sizeof b - l, hidden[i] ? " %d.%dh" : " %d.%d", hand[i].r, hand[i].s);
    snprintf(b + l, sizeof b - l, "\n"); dbg::print(b);
}
static void sortTest(int n) {
    while (n--) {
        shuffleDeck();
        int m = rnd() % 3, w = rnd() % 13, s2 = rnd() & 3;   // m: any cards, ranks in a window of six (straights), two suits (flushes)
        nHand = (uint8_t)(rnd() % 9); if (nHand < 3) nHand = HANDN;
        int k = 0;
        for (int j = 0; j < 52 && k < nHand; j++) {
            Card c = deck[j];
            if (m == 1 && (c.r - w + 13) % 13 > 5) continue;
            if (m == 2 && c.s != s2 && c.s != (s2 + 1) % 4) continue;
            hidden[k] = rnd() % 6 == 0; hand[k++] = c;
        }
        nHand = (uint8_t)k;
        handDump("KB"); sortHand(); handDump("KA");
    }
    nHand = 0;
}
#endif
static bool leadHook(char cmd, const char *a) {
    const char *a2 = a;                                // (parseNum moves its pointer: Gfx.h's hook needs the original)
    int n = (int)dbg::parseNum(a2, 10);
    switch (cmd) {
    case 'G': if (!round_) round_ = 3; startBonus(); return true;
    case 'U': startTurbo(); return true;
#ifndef PERF_BUILD
    case 'j': { char b[96]; int l = snprintf(b, sizeof b, "J n=%d", nJok); for (int i = 0; i < nJok; i++) l += snprintf(b + l, sizeof b - l, " %d:%d", jokers[i], cnt[i]); snprintf(b + l, sizeof b - l, " plays=%d disc=%d money=%ld score=%ld\n", playsLeft, discLeft, (long)money, (long)score); dbg::print(b); return true; }   // j: the wild cards, their counters and the state
    case 'x': { const char *q = a; for (nHand = 0; nHand < HANDN; nHand++) { uint8_t r = (uint8_t)dbg::parseNum(q, 10); hand[nHand] = { r, (uint8_t)dbg::parseNum(q, 10) }; hidden[nHand] = false; } return true; }   // x r s r s ... (8 cards): set the hand
    case 'q': regress(); return true;                  // q: the wild-card regression table (see regress())
    case 'h': handDump("HAND"); return true;           // h: the hand as it lies
    case 'k': sortTest(n); return true;                // k n: n test hands, before and after the sort
#endif
    case 'D': dealerBase = (uint8_t)((n % NDEAL + NDEAL * 8 - round_) % NDEAL); dealer = n % NDEAL; return true;
    case 'Y': forceBoss = (int8_t)n; return true;
    case 'E': tLeft = (int16_t)n; return true;
    case 'Z': skipOpen = n == 99; forceMini = n == 99 ? -1 : (int8_t)n; return true;   // 99: the run opens without its bonus stage (the scripts)
#ifndef PERF_BUILD                                     // (the board's perf build: only the commands the perf scripts use)
    case 'I': if (n >= 100) { cheatK(n); return true; }     // I 100: a sure win for the next play (the practice test); 101, 102: the newer stages (Minis.h cheatK)
              for (int i = 0; i < 9; i++) hlv[i] = (uint8_t)(n > LVMAX ? LVMAX : n); return true;
    case 'F': fxLow = n; return true;
    case 'V': loadSave(); state = TITLE; titleSel = 0; mode = M_RUN; timed = bonusOn = false; return true;   // power cycle (simsave)
    case 'A': { char b[112]; snprintf(b, sizeof b, "STATE music=%d fx=%d save=%d round=%d money=%ld nj=%d lv1=%d turbo=%ld col=%ld th=%d\n", musicOn, fxLow, hasSave, round_, (long)money, nJok, hlv[1], (long)turboHi, (long)colPack(), cop[0]); dbg::print(b); return true; }
#endif
    }
    return gfxHook(cmd, a);
}
static void leadInit() { dbg::hook = leadHook; }
#else
static void leadInit() {}
#endif

// ---- no WILDHAND.DAT on the SD card: the pictures and songs live there
static void drawNeedCard() {
    fc++;
    gfx_clear(P_BG);
    panel(8, 34, 112, 60);
    textOutC(42, "WILD HAND", P_GOLD, 2);
    textC(62, "NEED WILDHAND.DAT", P_TEXT);
    textC(72, "ON THE SD CARD", P_TEXT);
    if ((fc % 40) < 28) textC(84, "LOOKING...", P_GREY);
}
