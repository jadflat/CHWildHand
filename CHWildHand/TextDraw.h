// TextDraw.h: Wild Hand's own text: its font (Font.h, drawn in tools/make_font.py) and a small renderer that
// takes over CHGfx's gfx_text, gfx_textFx, gfx_textWidth and gfx_textWidthScaled (macros below), so the
// library's text code and 5x7 font are not linked. Same layout as the library's built-in font: 5x7 glyphs in a
// 6 px advance, y the top, scale by pixel replication, a transparent background; gfx_textFx's outline is the
// glyphs grown by a pixel all round, its shadow the outline (or the glyphs) one pixel down and right, its ramp
// a colour per screen row from the top of the ink (per-letter y offsets included).
// Differences: one line only ('\n' is not special), the clip is the screen (the game never draws text inside a
// gfx_setClip), scale 1..6, and lower case draws as upper case.
// Included by CHWildHand.ino before Gfx.h.
#pragma once
#include <chgame/RamFunc.h>
#include "Font.h"

// One layer of a string: the glyphs (e = 0) or the glyphs grown by a pixel all round (e = 1), in col, or a
// colour per screen row from ramp (ramp[0] is row top). Each glyph's rows are built as bit masks (bit 0 = one
// pixel left of the glyph), grown, and written straight into the framebuffer.
CHGAME_APP_RAMFUNC(whlayer) static void whLayer(int x, int y, const char *s, int sc, uint8_t col, const uint8_t *ramp,
                                                const int8_t *dy, int top, int e)
{
    for (; *s; s++, x += 6 * sc) {
        unsigned ch = (uint8_t)*s;
        if (ch - 'a' < 26u) ch -= 32;
        if (ch - 32u > WH_FONT_LAST - 32u) ch = '?';
        unsigned b = (ch - 32) * 35;
        uint32_t R[7 * 6 + 4];                         // the glyph's pixel rows, two blank rows above and below
        int n = 2;
        R[0] = R[1] = 0;
        for (int r = 0; r < 7; r++, b += 5) {
            unsigned v = (unsigned)(WH_FONT[b >> 3] | WH_FONT[(b >> 3) + 1] << 8) >> (b & 7);
            uint32_t p = 0;
            for (int c = 4; c >= 0; c--) p = p << sc | ((v >> c & 1) ? (1u << sc) - 1 : 0);
            for (int i = 0; i < sc; i++) R[n++] = p << 1;
        }
        R[n] = R[n + 1] = 0;
        int gy = y + (dy ? *dy++ : 0) - 2;
        for (int i = 2 - e; i < n + e; i++) {
            uint32_t m = R[i];
            if (e) { m |= R[i - 1] | R[i + 1]; m |= m << 1 | m >> 1; }
            unsigned py = (unsigned)(gy + i);
            if (!m || py >= GFX_H) continue;
            uint8_t c = (uint8_t)((ramp ? ramp[py - top] : col) & 15);
            uint8_t *row = gfx_fb + py * GFX_FB_STRIDE;
            for (unsigned px = (unsigned)(x - 1); m; m >>= 1, px++)
                if ((m & 1) && px < GFX_W) {
                    uint8_t &q = row[px >> 1];
                    q = (uint8_t)(px & 1 ? (q & 0x0F) | c << 4 : (q & 0xF0) | c);
                }
        }
    }
}

__attribute__((noinline)) static void whText(int x, int y, const char *s, uint8_t c) { whLayer(x, y, s, 1, c, nullptr, nullptr, 0, 0); }

// gfx_textFx: the shadow, then the outline, then the fill; the library's order, so the same pixels.
__attribute__((noinline)) static bool whTextFx(int x, int y, const char *s, uint8_t sc, uint8_t fill, int outline = -1,
                                               int shadow = -1, const uint8_t *ramp = nullptr, const int8_t *dy = nullptr)
{
    if (sc - 1u > 5u) sc = 1;
    int top = y;                                       // the ink's top row: the ramp starts there
    if (dy) for (int k = 0; s[k]; k++) if (y + dy[k] < top) top = y + dy[k];
    if (shadow >= 0) whLayer(x + 1, y + 1, s, sc, (uint8_t)shadow, nullptr, dy, 0, outline >= 0);
    if (outline >= 0) whLayer(x, y, s, sc, (uint8_t)outline, nullptr, dy, 0, 1);
    whLayer(x, y, s, sc, fill, ramp, dy, top, 0);
    return true;
}

static int whTextWidthScaled(const char *s, uint8_t sc) { int w = 0; while (*s++) w += 6 * sc; return w; }

#define gfx_text            whText
#define gfx_textFx          whTextFx
#define gfx_textWidthScaled whTextWidthScaled
#define gfx_textWidth(s)    whTextWidthScaled(s, 1)
