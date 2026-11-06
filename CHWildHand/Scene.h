// Scene.h: a haunted card house, a different backdrop per area. Each area retints the four
// background colours (felt, felt light, dark, panel) and paints its own pattern under a dithered
// vignette: the table's mouldy felt, the title's slow-turning rotten boards, the shop's crypt stalls
// by candlelight, the boss's blood-black felt with a cracked frame and a heartbeat, the bonus stage's
// nightmare swirl, the game-over cold and a raven. The lights flicker (palTick in Gfx.h).
// Included by Gfx.h. Integer maths only; the backdrop is written straight into the frame rows.
#pragma once

enum Scn : uint8_t { S_TABLE, S_TITLE, S_SHOP, S_BOSS, S_BONUS, S_OVER, S_WON };
static uint8_t curScn;
static int16_t parX;                                   // parallax: eases after the cursor or the selection (x16)
__attribute__((noinline)) static uint8_t scene() {
    if (state == TITLE) return S_TITLE;
    if (state == SHOP) return S_SHOP;
    if (state == OVER) return S_OVER;
    if (state == WON) return S_WON;
    if (state == BONUS || state == MINI || bonusOn) return S_BONUS;
    if (boss != 255 && !timed && (state == PLAY || state == RESULT || state == PAUSED || state == DEAL)) return S_BOSS;
    return S_TABLE;
}
// each area's felt drifts round its own ring of six colours (palette cycling: nearly free); the light felt and the dark are
// worked out from it; BG is fixed. The bonus stage: bruise purple, bile green, blood.
#define RING ((const uint8_t (*)[6][3])(wh_tabs + T_RING))     // (in RAM, from the card: tools/tables.py)
#define SCNBG ((const uint8_t (*)[3])(wh_tabs + T_SCNBG))   // (in RAM, from the card: tools/tables.py)
#define LTK ((const uint8_t *)(wh_tabs + T_LTK))
#define RSH ((const uint8_t *)(wh_tabs + T_RSH))
   // light felt = v + v/k + 10; frames per step = 1 << RSH (4x slower under CALM)
static uint8_t curPal[4][3];
static const uint8_t (*scenePal(uint8_t s))[3] {
    if (state == MINI && miniK == MK_PAIRS) s = S_TABLE;              // the snake box: green scales
    int sh = RSH[s] + (fxLow ? 2 : 0), i = (fc >> sh) % 6, f = ((fc & ((1 << sh) - 1)) << 5) >> sh;
    for (int c = 0; c < 3; c++) {
        int v = RING[s][i][c] + (RING[s][(i + 1) % 6][c] - RING[s][i][c]) * f / 32;
        curPal[0][c] = (uint8_t)v;
        curPal[2][c] = (uint8_t)(v + v / LTK[s] + 10);
        curPal[1][c] = s == S_BONUS ? (uint8_t)(c == 0 ? 20 : c == 1 ? 6 : 30) : (uint8_t)(v * 7 / 16);
        curPal[3][c] = SCNBG[s][c];
    }
    return curPal;
}

// ---- the backdrop: lut[row parity][vignette level * 2 + pattern bit] is a byte of two pixels
static int sinF(int a) { int i = (a >> 3) & 31, f = a & 7; return SIN32[i] + (SIN32[(i + 1) & 31] - SIN32[i]) * f / 8; }   // isin, smooth
__attribute__((noinline)) static uint8_t vigLv(int e, int off) { e -= off; return e < 2 ? 4 : e < 4 ? 3 : e < 7 ? 2 : e < 11 ? 1 : 0; }
// Worked out in 4x2 blocks (a quarter of the sums): the pattern bit per block, then a lookup per row.
#pragma GCC push_options
#pragma GCC optimize("tree-loop-im","move-loop-invariants")   // (this is the one hot loop: keep it optimised for speed while the rest of the file is built for size)
static void sceneBg() {
    uint8_t s = curScn, lut[2][10], colLv[32], bl[32];
    // the vignette's dither pairs: level 0 the pattern, 1 pattern/dark, 2 dark, 3 dark/bg, 4 bg
    const uint8_t *LA = wh_tabs + T_LAB, *LB = LA + 5;
    for (int v = 0; v < 10; v++) {
        uint8_t P = (v & 1) ? P_FELTLT : P_FELT, a = v < 4 ? P : LA[v >> 1], c = v > 1 ? LB[v >> 1] : P;
        lut[0][v] = (uint8_t)(a | (c << 4)); lut[1][v] = (uint8_t)(c | (a << 4));
    }
    int off = s == S_BOSS ? 5 : s == S_TITLE || s == S_WON ? -2 : 0;
    if (!fxLow) off += s == S_BOSS ? (fc % 48 < 12 ? -2 : 0) : (isinK(3) >> 6);   // the vignette breathes (the boss's gasps on the heartbeat)
    for (int i = 0; i < 16; i++) colLv[i] = colLv[31 - i] = vigLv(i * 4, off);
    // everywhere else the bonus stages' backdrop: a turning starburst with rings flowing out through it. At the table it
    // turns round the dealer's head (and shakes with the table); the hex's turns faster; the dawn's spins, no rings
    const bool spin = s != S_TITLE && s != S_OVER, tbl = state == PLAY || state == RESULT || state == PAUSED;
    int ang = fc * (s == S_WON ? 3 : s == S_BOSS ? 2 : 1), c = sinF(ang + 64), sn = sinF(ang), ph = fc & 255;
    const int scx = 64 + ox * 2, scy = (tbl ? 54 : 58) + oy * 2;
    for (int y = 0; y < 128; y += 2) {
        const bool dup = (s == S_WON || tbl || state == SHOP) && (y & 2);
        if (spin && dup) {}                                // the win screen, the table and the shop: 4x4 blocks, the lower pair of rows is a copy (the board's frame budget)
        else if (spin) {                                   // a turning starburst of twelve rays (the bonus stage adds rings that flow out)
            int dy = y - scy, dx = 2 - scx, rx = dx * c - dy * sn, ry = dx * sn + dy * c;
            for (int i = 0; i < 32; i++, rx += 4 * c, ry += 4 * sn) {
                int ax = rx < 0 ? -rx : rx, ay = ry < 0 ? -ry : ry;
                int k = (ay << 8) < ax * 148 ? 0 : (ay << 8) < ax * 443 ? 1 : 2;
                int b = (k + ((rx < 0) ^ (ry < 0))) & 1;
                if (s != S_WON) b ^= (((ax + ay) >> 6) - ph) >> 3 & 1;
                bl[i] = (uint8_t)b;
            }
        } else if (!(y & 15)) {                            // rotten boards (the title, the grave's coffin wood): 16-px planks, seams and knots
            for (int i = 0; i < 32; i++) bl[i] = (uint8_t)(((y >> 4) & 1) ^ ((i * 5 + (y >> 4) * 3) % 13 == 0));
        }
        gfx_waitRow(y + 2);
        for (int par = 0; par < 2; par++) {
            int yy = y + par, ye = yy < 64 ? yy : 127 - yy, rl = vigLv(ye, off);
            uint16_t *r16 = (uint16_t *)(gfx_fb + yy * GFX_FB_STRIDE);
            if (dup && !rl && yy > 10) {                       // no vignette: the same two rows as the pair above, copy them
                memcpy(r16, r16 - GFX_FB_STRIDE, GFX_FB_STRIDE);
                continue;
            }
            const uint8_t *lp = lut[par];
            uint16_t m0 = (uint16_t)(lp[0] * 257), m1 = (uint16_t)(lp[1] * 257);
            for (int i = 0; i < 32; i++) {
                if (!rl && i == 3) { for (; i < 29; i++) r16[i] = bl[i] ? m1 : m0; }   // the middle: no vignette
                int L = rl + colLv[i]; if (L > 4) L = 4;
                r16[i] = (uint16_t)(lp[L * 2 + bl[i]] * 257);
            }
        }
    }
}

#pragma GCC pop_options

// ---- candles along the edge, each flame flickering on its own
static void candles(int y0) {
    for (int i = 0; i < 8; i++) {
        int x = i < 4 ? 2 : 122, y = y0 + 8 + (i & 3) * (110 - y0) / 4;
        gfx_fillRect(x, y, 4, 7, P_SHADE);
        int f = (frand() & 3) == 0;
        gfx_fillRect(x + 1, y - 3 + f, 2, 3 - f, P_GOLD); pix(x + 1, y - 2, P_GLOW);
    }
}

// ---- drifting things, one recipe a row (tools/tables.py PART): how many, two colours, speed | 32 falls | 64 blinks, w << 4 | h, the sway's
// divisor. The rows: each area's motes (spores, embers, dust, ghost lights in four depths), the BG styles
#define PART ((const uint8_t (*)[6])(wh_tabs + T_PART))
static void drifts(const uint8_t *m) {
    for (int i = 0; i < m[0]; i++) {
        int x = (i * 53 + 17 + isinF(2, i * 29) / m[5] - (parX >> 6)) & 127;
        int y = (((fc * (1 + (i & 3)) * (m[3] & 31)) >> (fxLow ? 4 : 3)) + i * 37) % 150;
        y = m[3] & 32 ? y - 22 : 127 - y;
        if (m[3] & 64 && (fc + i * 23) & 32) continue;      // (fireflies: each glows half the time)
        gfx_fillRect(x, y, (m[4] >> 4) + !(i & 3), m[4] & 15, m[2 - (i & 1)]);   // (every fourth one a pixel wider)
    }
}
// ---- over the backdrop, under everything else
// ---- a raven on a branch at the top of the grave screen; it blinks and now and then the lightning shows it
static void raven(int x, int y) {
    gfx_line(x - 14, y + 9, x + 16, y + 7, P_INK);                 // the branch
    gfx_fillCircle(x, y + 3, 4, P_INK); gfx_fillCircle(x + 4, y, 3, P_INK);
    gfx_fillRect(x + 7, y, 3, 1, P_GREY); gfx_fillRect(x - 7, y + 4, 4, 2, P_INK);   // beak, tail
    if ((fc % 90) > 3) pix(x + 5, y - 1, P_RED);
}
static void rainCloud() {
    for (int i = 0; i < 6; i++) gfx_fillRect(3 + i * 22, 36 + (i & 1) * 2, 5, 7, P_DARK);   // far: graves on the skyline
#ifndef PERF_LEAN   // (a raven on the grave screen only; the board's debug build is out of flash)
    raven(100, 10);
#endif
    if (fc % 150 == 0) flashT = 3;
}
// ---- eyes in the dark: a pair opens in the vignette now and then, looks, and shuts
static void darkEyes() {
    int t = fc % 400;
    if (t > 40 || fxLow) return;
    const int8_t (*EP)[2] = (const int8_t (*)[2])(wh_tabs + T_EP);
    const int8_t *e = EP[(fc / 400) & 3];
    int h = t < 3 || t > 37 ? 1 : 2;                       // half open as they open and shut
    gfx_fillRect(e[0], e[1] + 2 - h, 2, h, P_RED); gfx_fillRect(e[0] + 4, e[1] + 2 - h, 2, h, P_RED);
}


// the BG option: 0 AUTO (the night picks: BGORD[night & 7], a PART row; 0 the area's own), 1 SCENE (the area's own motes), 2.. a style
// (PART rows 7..); CALM and OFF keep the area's motes
#define BGORD ((const uint8_t *)(wh_tabs + T_BGORD))
static void sceneFg(bool dark) {
    int k = bgSel ? bgSel + 5 : BGORD[round_ & 7];
    drifts(PART[k < 7 || fxLow ? curScn : k]);
#ifndef PERF_LEAN   // (the eyes are 4 small rects for 40 frames in 400; the board's debug build is out of flash)
    if (dark) darkEyes();
#endif
}
