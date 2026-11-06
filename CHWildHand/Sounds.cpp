#include "Sounds.h"
#pragma GCC optimize("no-jump-tables","no-caller-saves","reorder-blocks","reorder-blocks-algorithm=stc","no-tree-loop-im","no-schedule-insns2","no-move-loop-invariants","no-lra-remat","no-tree-forwprop","no-tree-dominator-opts","no-shrink-wrap")
#include "Card.h"
#include "CardIndex.h"

// The effects' step tables are on the SD card (tools/sfx_src.txt, WILDHAND.DAT at CARD_SFX): read once into this RAM block when the
// card is opened. Layout: where each effect's steps start (SFX_COUNT + 1 bytes), their flags (SFX_COUNT), then the steps.
static_assert(SFX_COUNT == (uint16_t)Sfx::COUNT, "the effects on the card");
__attribute__((section(".bss.zz_blob"))) static uint8_t blob[SFX_LEN + JT_LEN + MISC_LEN];           // ... and after them the wild cards' table (tools/jokers.py): per card a 4-byte header, then 4 bytes a rule; then the table's lines (card_text.py MISC)

__attribute__((section(".sbss.wh_tabs"))) uint8_t wh_tabs[TAB_LEN];
bool openCard() { return card::open() && card::copyDirect(blob, CARD_SFX, SFX_LEN + JT_LEN + MISC_LEN) && card::copyDirect(wh_tabs, CARD_SFX + SFX_LEN + JT_LEN + MISC_LEN, TAB_LEN); }   // (setup, or after gfx_wait(): the scratch is free)
const uint8_t *wildRec(uint8_t id) {
    const uint8_t *p = blob + SFX_LEN;
    while (id--) p += 4 + 4 * ((p[0] >> 4) & 3);
    return p;
}

const char *miscText(uint8_t i) { const char *p = (const char *)blob + SFX_LEN + JT_LEN; while (i--) while (*p++) {} return p; }

__attribute__((noinline)) void playSfx(Sfx s, uint8_t semi) {
    const uint8_t i = (uint8_t)s;                           // (before the card is read, n is 0: silence)
    const audio::Effect e = { (const audio::Step *)(blob + 2 * SFX_COUNT + 1) + blob[i], (uint8_t)(blob[i + 1] - blob[i]), blob[SFX_COUNT + 1 + i] };
    synth::sfx(e, semi);   // over the music (the synth's effect voice)
}
void playSfx(Sfx s) { playSfx(s, 0); }

// memcpy and memset (newlib's are 350 B of unrolled word loops; here they copy a few hundred bytes a frame at most): words when all is aligned
#if !defined(CHSIM)
#include <stddef.h>
extern "C" __attribute__((used, noinline)) void *memcpy(void *d, const void *s, size_t n) {
    if (!(((uintptr_t)d | (uintptr_t)s | n) & 3)) { uint32_t *o = (uint32_t *)d; const uint32_t *i = (const uint32_t *)s; for (; n; n -= 4) *o++ = *i++; }
    else { uint8_t *o = (uint8_t *)d; const uint8_t *i = (const uint8_t *)s; while (n--) *o++ = *i++; }
    return d;
}
extern "C" __attribute__((used, noinline)) void *memset(void *d, int c, size_t n) {
    uint8_t *o = (uint8_t *)d;
    while (n--) *o++ = (uint8_t)c;
    return d;
}
#endif
