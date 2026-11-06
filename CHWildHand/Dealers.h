// Dealers.h: the nine dealers of the haunted house, monsters all (the beholder, the mimic, the dragon, the mind
// flayer, the cube, the lich, the plague doctor, the scarecrow, the porcelain doll). Each has a 16x16 head (Art2.h) with eyes the code animates (blink, look side to
// side, tilt, look at the player), a body made of shared parts in its own colours, a shuffle style and a dealing
// style. Included after Gfx.h (it uses its helpers). Integer maths only.
#pragma once

// the head is 24x24 (Art2.h D0-D8, tools/dealers24.py): ex, ex2, ey: the eyes' top left in the picture (255: none), es: the
// socket's side (2 or 4)
struct Dealer { uint8_t ex, ex2, ey, es, lid, pupil, body, trim, hand, eye; };
#define DL ((const Dealer *)(wh_tabs + T_DL))                  // (in RAM, from the card: tools/tables.py)

// ---- the head, eyes and moods
// the heads' places on the card (ArtCard.h's DART, as 16-bit numbers: half the flash; the art section is < 64 KB, Gfx.h)
static const uint16_t DART16[9] = { (uint16_t)(uintptr_t)ART_D0, (uint16_t)(uintptr_t)ART_D1, (uint16_t)(uintptr_t)ART_D2, (uint16_t)(uintptr_t)ART_D3, (uint16_t)(uintptr_t)ART_D4,
                                    (uint16_t)(uintptr_t)ART_D5, (uint16_t)(uintptr_t)ART_D6, (uint16_t)(uintptr_t)ART_D7, (uint16_t)(uintptr_t)ART_D8 };
enum DAct : uint8_t { DA_IDLE, DA_SHIFTY, DA_TILT, DA_STARE, DA_WOW, DA_SAD };
static uint8_t dAct, dActT;
__attribute__((noinline)) static void dealerReact(bool good) { dAct = good ? DA_WOW : DA_SAD; dActT = 40; }
static void dealerTick() {
    if (dActT) { dActT--; return; }
    int r = fr(8);
    // the hotter the table, the more often they get shifty
    if (r < 2 + heat / 64) { dAct = DA_SHIFTY; dActT = 48; gag(Gag::Twitch); }
    else if (r < 5) { dAct = DA_TILT; dActT = 26; }
    else if (r < 6) { dAct = DA_STARE; dActT = 30; }
    else { dAct = DA_IDLE; dActT = (uint8_t)(50 - heat / 8); }
}
// x, y: the head's top left; s8: its size in eighths (8: 24 px, 16: 48, 19: the table's 57). Eyes and nudges scale with it, shifts only.
static void drawDealerHead(int x, int y, int s8) {
    const int d = dealer;
    const Dealer &D = DL[d];
    int lx = 0, ly = 0, tilt = 0, u = s8 >> 3;            // u: a nudge, a whole pixel per 24
    switch (dAct) {
    case DA_SHIFTY: lx = dActT > 12 ? (((dActT >> 3) & 1) ? -1 : 1) : 0; ly = dActT > 12 ? 0 : 1; break;
    case DA_TILT: tilt = (dActT > 4 && dActT < 22) ? u : 0; lx = tilt ? 1 : 0; break;
    case DA_STARE: ly = 1; break;
    case DA_WOW: x += (dActT & 2) ? u : -u; ly = -1; break;   // a twitch
    case DA_SAD: ly = 1; tilt = -u; break;
    }
    int blinkEvery = 64 - heat / 8;
    bool shut = (fc % blinkEvery) < 3 && dAct != DA_WOW;
    if (shut && (fc % blinkEvery) == 0 && s8 >= 13) gag(Gag::Blink);
    x += tilt; drawArtE((const uint8_t *)(uintptr_t)DART16[d], x, y, s8);               // (a lean: the whole head sways)
    int py = y + (D.ey * s8 >> 3), S = ((D.ey + D.es) * s8 >> 3) - (D.ey * s8 >> 3), px0 = 0;
    for (int e = 0; e < 2; e++) {
        int ex = e ? D.ex2 : D.ex;
        if (ex == 255) continue;
        int px = x + (ex * s8 >> 3);
        px0 = px;
        if (shut) { gfx_fillRect(px, py, S, S, D.lid); gfx_fillRect(px, py + S / 2, S, u, P_INK); continue; }
        gfx_fillRect(px, py, S, S, D.eye);                              // the sockets (the beholder's iris red)
        int ox_ = (lx + 1) * S >> 2, oy_ = ly < 0 ? 0 : (ly + 1) * S >> 2;
        gfx_fillRect(px + ox_, py + oy_, S / 2, S / 2, D.pupil);
    }
    if (s8 < 13) return;                                 // gore: a bloodshot eye, drool, and teeth dripping when it beats you
    #define G(v) ((v) * s8 >> 4)                         // (the places below are for the 48-px head)
    pix(px0 + 2, py + S - 1, P_RED);
    int dr = dAct == DA_SAD ? 10 : heat > 170 ? 6 : 2, sy = y + G(40) + ((fc >> 2) & 7);
    for (int k = 0; k < 3; k++) gfx_vline(x + G(18 + k * 6), y + G(38), 1 + (fc + k * 5) % dr, P_RED);
    if (dAct == DA_SAD || heat > 170) gfx_fillRect(x + G(28), sy, 2, 2, P_RED);
    if (dAct == DA_WOW && !(fc & 3)) spark(x + G(6), y + G(36), G(36));   // a hand that hurts it: it fizzes
    #undef G
}

// ---- the body (shared parts in the dealer's colours) and hands
static void miniBack(int x, int y, int w = 10) {
    if (w < 1) return;
    if (w < 3) { gfx_fillRect(x, y, w, 14, P_INK); return; }
    gfx_fillRoundRect(x, y, w, 14, 1, P_INK);
    gfx_fillRect(x + 1, y + 1, w - 2, 12, P_WINE);
    if (w > 5) { gfx_rect(x + 2, y + 2, w - 4, 10, P_SHADE); pix(x + w / 2, y + 7, P_RED); }
}
static void drawHand(int x, int y) {
    const Dealer &D = DL[dealer];
    gfx_fillCircle(x, y, 3, P_INK);
    gfx_fillCircle(x, y, 2, D.hand);
    gfx_vline(x - 2, y - 4, 3, D.hand); gfx_vline(x, y - 5, 3, D.hand); gfx_vline(x + 2, y - 4, 3, D.hand);   // long bony fingers
}
static void drawBody(int d, int cx, int y) {
    const Dealer &D = DL[d];
    if (d == 1) return;                                  // the mimic is all chest
    if (d == 3) {                                        // the mind flayer's tentacles curl under the head
        for (int k = 0; k < 4; k++) {
            int bx = cx - 12 + k * 8;
            for (int s = 0; s < 7; s++) {
                int xx = bx + (isinF(6, k * 50 + s * 30) * (s + 1) >> 7), yy = y + s * 3;
                gfx_fillRect(xx, yy, 4 - s / 3, 3, (s & 1) ? D.trim : D.body);
            }
        }
        return;
    }
    if (d == 5) {                                        // the lich's trailing mist
        for (int k = 0; k < 3; k++) gfx_fillCircle(cx - 8 + k * 8, y + 2 + ((fc / 8 + k) & 1), 4, P_SHADE);
        return;
    }
    gfx_fillRoundRect(cx - 18, y, 36, 22, 5, P_INK);
    gfx_fillRoundRect(cx - 17, y + 1, 34, 21, 4, D.body);
    switch (d) {
    case 0: case 6: gfx_fillRect(cx - 2, y + 1, 4, 21, D.trim); break;   // a robe, a long coat: dark, one seam
    case 2: gfx_fillRect(cx - 17, y + 15, 34, 2, D.trim); break;          // the dragon's belt
    }
}

// ---- shuffle styles, as numbers: n cards, each running a phase p (0..63) at sp a frame, gap apart; card k sits at x0 + k * kx,
// y0 (from the centre), travels dx, lifts in an arc `lift` high, and swings on an orbit of radius rad (+ grow at the arc's top; x
// twice y) turning fa per phase step, fk apart
struct Shuf { int8_t n, sp, gap, x0, y0, kx, dx, lift, rad, grow, fa, fk; };
#define SHUF ((const Shuf *)(wh_tabs + T_SHUF))   // the six styles (tools/tables.py SHUF)
enum { SHUF_T = 54, DEAL_T0 = 54, DEAL_EACH = 5, DEAL_FLY = 12 };
// each dealer's styles: the shuffle (low nibble), the deal (high). The newer three borrow: the doctor the breeze and the lob,
// the scarecrow the cannon and the zigzag, the doll the carousel and the spin
#define STYLE ((const uint8_t *)(wh_tabs + T_STYLE))
static void drawShuffle(int d, int t, int cx, int cy) {
    const int st = STYLE[d] & 15;
    const Shuf &S = SHUF[st];
    int lx = cx - 22, rx = cx + 22, hy = cy + 16;        // the hands, where the deck sits
    if (st == 0 || st == 2) miniBack(lx - 5, hy - 10, 10);
    if (st == 0) miniBack(rx - 5, hy - 10, 10);
    for (int k = 0; k < S.n; k++) {
        int p = (t * S.sp + k * S.gap) & 63, arc = p * (64 - p) >> 2, a = p * S.fa + k * S.fk, r = S.rad + (arc * S.grow >> 8);
        int x = cx + S.x0 + k * S.kx + (p * S.dx >> 6) + (isin(a + 64) * r >> 5), y = cy + S.y0 - (arc * S.lift >> 8) + (isin(a) * r >> 6);
        if (st == 1) gfx_line(cx, cy + 10, x, y + 7, P_WINE);
        if (st == 0) hline(lx, y + 8, x - lx, P_GLOW);
        miniBack(x - 5, y, 10);
    }
    if (st != 1 && st != 5) { drawHand(lx, hy); drawHand(rx, hy); }   // (the hands go over the cards)
}
// card k's place while it is dealt to slot k (p 0..DEAL_FLY), by dealing style: sp, how fast (2: fired, then it waits); lift, an arc
// (negative: a low swoop); zig, a sideways weave
#define DEALS ((const int8_t (*)[3])(wh_tabs + T_DEALS))
static void dealPath(int d, int k, int p, int cx, int cy, int &x, int &y) {
    const int sd = STYLE[d] >> 4;
    const int8_t *D = DEALS[sd];
    int sx = 4 + k * 15, sy = 98;
    int fx = sd == 1 ? 20 + (k & 3) * 29 : cx - 5, fy = cy + 10;
    p *= D[0]; if (p > DEAL_FLY) p = DEAL_FLY;
    x = fx + (sx - fx) * p / DEAL_FLY + (isin(p * 40) * D[2] >> 6); y = fy + (sy - fy) * p / DEAL_FLY - p * (DEAL_FLY - p) * D[1] / 8;
}
// the dealer at work: shuffling, then dealing the eight cards down to the table (t: frames into the scene)
static void drawDealerScene(int t, int cx, int cy) {
    const int d = dealer;
    int bob = isinK(3 + heat / 48) >> 5;
    drawBody(d, cx, cy + 2 + bob);
    if (t < SHUF_T) drawShuffle(d, t, cx, cy);
    else {
        int lx = cx - 22, rx = cx + 22, hy = cy + 16;
        if (d != 1 && d != 5) { miniBack(lx - 5, hy - 10, 10); drawHand(lx, hy); drawHand(rx, hy - ((t >> 2) & 1) * 3); }
        for (int k = 0; k < HANDN; k++) {
            int st = DEAL_T0 + (d == 1 ? (k >> 2) * 12 : k * DEAL_EACH);
            if (t < st) continue;
            int p = t - st; if (p > DEAL_FLY) p = DEAL_FLY;
            if (p == 1) playSfx(Sfx::Deal, (uint8_t)k);
            int x, y; dealPath(d, k, p, cx, cy, x, y);
            miniBack(x, y, 10);
        }
    }
    drawDealerHead(cx - 24, cy - 40 + bob, 16);
}
