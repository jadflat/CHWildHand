// WILDHAND.DAT (Card.h): block 0 holds "WILDHAND" and the build's hash (CardIndex.h, written with the file by
// tools/make_card.py), so a card from another build is refused; then the picture section, then the song section.
// Five 512 B slots (and CHGfx's scratch buffer while a frame is drawn) keep the blocks used last: the pictures on screen are read once and drawn from RAM after that.
#include <CHGame.h>
#include <Fat.h>
#include <SdSpi.h>
#ifndef CHSIM
#include <Arduino.h>                                // SPI1, GPIOx, micros() for the settings block's write (below)
#else
#include <stdio.h>
#include <stdlib.h>
#endif
#include "Card.h"
#include "CardIndex.h"
static const char WHDAT[] = "WILDHANDDAT";
#ifdef WH_TRACE
#include <stdio.h>
#endif

namespace card {

#ifndef WH_SLOTS
#define WH_SLOTS 5                                  // (the simulator can try more: -D WH_SLOTS=n)
#endif
enum { SLOTS = WH_SLOTS, ALL = SLOTS + 1 };                // the last slot is CHGfx's scratch buffer, free while a frame is drawn
alignas(4) static uint8_t buf[SLOTS][512];
__attribute__((section(".sbss.card_tag"))) static uint16_t tag[ALL];   // (.sbss: inside the global pointer's reach)
static uint16_t age[ALL], tick;                   // the least recently used slot is the one to replace (16-bit: many uses a frame)
static uint8_t used[ALL], frameNo;               // the frame each slot was last used in
static uint8_t misses;                            // blocks read from the card this frame
static bool noWrite;                              // a settings write failed: no more tries until the card is opened again
bool lowPri;                                      // scenery: it may wait a frame rather than read more than two blocks
static uint8_t *slotBuf(uint8_t i) { return i < SLOTS ? buf[i] : gfx_chunkScratch(); }
__attribute__((section(".sbss.card_runs"))) static fat::Run runs[4];
static uint8_t nRuns;
static bool open_;
static uint8_t held;                              // 1: the last slot is held (hold()): out of the cache

// WILDHAND.DAT's place on the card, found without CHSd's general FAT code (only its block reads): the first FAT16 or FAT32
// volume (at LBA 0, or the MBR's first partition), the file in its root, in up to four runs. A card that is not like
// that simply finds no file; the header check below catches the rest. About half the size of fat::open().
static uint32_t fatLba, dataLba;
static uint8_t shift;
static bool f32;
static uint32_t rd16(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8; }
static uint32_t rd32(const uint8_t *p) { return rd16(p) | rd16(p + 2) << 16; }
static uint32_t clus(uint32_t c) { return dataLba + ((c - 2) << shift); }
static uint32_t nextC(uint32_t c, uint8_t *b) {                 // the cluster after c, 0 at the end (or on a read error)
    const uint32_t off = c << (f32 ? 2 : 1);
    if (!sd::read(fatLba + (off >> 9), b)) return 0;
    const uint32_t n = f32 ? rd32(b + (off & 511)) & 0x0FFFFFFFu : rd16(b + (off & 511));
    return n >= (f32 ? 0x0FFFFFF8u : 0xFFF8u) ? 0 : n;
}
static uint8_t findDat(uint8_t *b) {
    uint32_t base = 0;
    if (!sd::init() || !sd::read(0, b)) return 0;
    if (b[0] != 0xEB && b[0] != 0xE9 && !sd::read(base = rd32(b + 454), b)) return 0;   // an MBR: its first partition
    uint32_t fsz = rd16(b + 22);
    f32 = !fsz;
    if (f32) fsz = rd32(b + 36);
    for (shift = 0; (1u << shift) < b[13]; shift++) {}
    fatLba = base + rd16(b + 14);
    const uint32_t rootSecs = (rd16(b + 17) + 15) >> 4, rootLba = fatLba + b[16] * fsz;
    dataLba = rootLba + rootSecs;
    uint32_t dir = rd32(b + 44), sec = 0;
    for (uint32_t guard = 4096; guard--;) {
        uint32_t lba;
        if (!f32) { if (sec == rootSecs) return 0; lba = rootLba + sec; }
        else { if (sec >> shift) { if ((dir = nextC(dir, b)) < 2) return 0; sec = 0; } lba = clus(dir) + sec; }
        sec++;
        if (!sd::read(lba, b)) return 0;
        for (const uint8_t *d = b; d < b + 512; d += 32) {
            if (!d[0]) return 0;
            if (d[0] == 0xE5 || (d[11] & 0x1A) || memcmp(d, WHDAT, 8)) continue;
            uint32_t c = rd16(d + 26) | (f32 ? rd16(d + 20) << 16 : 0), left = (rd32(d + 28) + 511) >> 9;
            uint8_t n = 0;
            while (left) {
                if (c < 2) return 0;
                const uint32_t at = clus(c), k = left < (1u << shift) ? left : (1u << shift);
                if (n && runs[n - 1].lba + runs[n - 1].blocks == at) runs[n - 1].blocks += k;
                else { if (n == 4) return 0; runs[n].lba = at; runs[n].blocks = k; n++; }
                left -= k;
                if (left) c = nextC(c, b);
            }
            return n;
        }
    }
    return 0;
}
__attribute__((noinline)) static uint32_t lbaOf(uint32_t k) {          // the card block of the file's block k (0: none; the file is never at block 0)
    for (uint8_t i = 0; i < nRuns; i++) { if (k < runs[i].blocks) return runs[i].lba + k; k -= runs[i].blocks; }
    return 0;
}
static bool readBlk(uint32_t k, uint8_t *dst) {
    gfx_wait();                                    // (every read of the card, and so every write: the last flush may still be running, and the card shares its SPI)
    const uint32_t lba = lbaOf(k);
    return lba && sd::read(lba, dst);
}

bool open() {
    for (uint8_t i = 0; i < ALL; i++) tag[i] = 0xffff;
    noWrite = false;
    nRuns = findDat(buf[0]);
    open_ = nRuns && readBlk(0, buf[0]) && !memcmp(buf[0], WHDAT, 8) && *(const uint32_t *)(buf[0] + 8) == CARD_HASH;   // (the name's first 8 letters: one string; buf is word-aligned)
    return open_;
}

bool ok() { return open_; }

void frame() { tag[SLOTS] = 0xffff; frameNo++; misses = 0; }              // the scratch buffer went out with the last flush

const uint8_t *at(uint32_t off) {
    if (!open_) return nullptr;
    const uint16_t blk = (uint16_t)(off >> 9);
#ifdef WH_TRACE
    if ((frameNo & 63) == 7) fprintf(stderr, "use f%u b%u lp%d\n", (unsigned)frameNo, (unsigned)blk, (int)lowPri);
#endif
    uint8_t v = 0;
    while (v < ALL && tag[v] != blk) v++;
    if (v == ALL) {
        // the slot to replace: the least recently used of those not used this frame (a frame that needs more blocks than there
        // are slots reads the extra ones through the scratch slot, so the others stay put and only the extra ones miss)
        v = SLOTS;
        for (uint8_t i = 0; i < SLOTS - held; i++)
            if (used[i] != frameNo && (v == SLOTS || (uint16_t)(tick - age[i]) > (uint16_t)(tick - age[v]))) v = i;
        if (lowPri && misses >= 2) return nullptr;    // the scenery can show up a frame later; a frame cannot be late
        misses++;
#ifdef WH_TRACE
        fprintf(stderr, "miss f%u b%u lp%d\n", (unsigned)frameNo, (unsigned)blk, (int)lowPri);
#endif
        tag[v] = 0xffff;
        if (!readBlk(blk, slotBuf(v))) { open_ = false; return nullptr; }
        tag[v] = blk;
    }
    age[v] = ++tick;                              // (a hit, or the block just read)
    used[v] = frameNo;
    return slotBuf(v) + (off & 511);
}

static bool copy(uint8_t *dst, uint32_t off, uint16_t n) {   // (through the cache: hold() only)
    while (n) {
        const uint8_t *s = at(off);
        if (!s) return false;
        uint16_t k = (uint16_t)(512 - (off & 511));
        if (k > n) k = n;
        memcpy(dst, s, k);
        dst += k; off += k; n -= k;
    }
    return true;
}
// n bytes copied into the last slot, which then stays out of the cache until release(): a stage script's own RAM (StageVM.h)
uint8_t *hold(uint32_t off, uint16_t n) {
    held = 1; tag[SLOTS - 1] = 0xffff;
    if (copy(buf[SLOTS - 1], off, n)) return buf[SLOTS - 1];
    held = 0;
    return nullptr;
}
void release() { held = 0; }

// A piece of one block, read through CHGfx's scratch buffer (free until the frame is drawn) instead of the five slots: the
// music's chunks must not push the pictures out of the cache, or the frame they land in reads those back (4-5 reads, 7 ms).
bool copyDirect(uint8_t *dst, uint32_t off, uint16_t n) {
    if (!open_) return false;
    uint8_t *b = gfx_chunkScratch();
    tag[SLOTS] = 0xffff;
    while (n) {
        if (!readBlk(off >> 9, b)) { open_ = false; return false; }
        uint16_t k = (uint16_t)(512 - (off & 511));
        if (k > n) k = n;
        memcpy(dst, b + (off & 511), k);
        dst += k; off += k; n -= k;
    }
    return true;
}

// ---- the settings block: CARD_SET, the DAT's last block (make_card.py writes it as 0xFF), keeps the pause menu's settings where
// another game's save cannot overwrite them. Five bytes: 'W', the three packed bytes, a check byte; the rest stays 0xFF.
// The library's driver only reads, so the one write is done here: a single-block write (CMD24) on the same SPI1 set-up as
// SdSpi.cpp (HypeRunner's clean-room driver, MIT; the helpers below are cut down from it). It touches only a block inside the file's
// own cluster runs, and only one that still holds the 0xFF fill or an earlier settings block. Nothing reads the block through the
// slots (the load reads it into the first slot and drops its tag, the read-back uses the scratch buffer), so no slot holds a stale copy.
static uint8_t chk(const uint8_t *h) { return (uint8_t)(h[1] + h[2] + h[3] + 0xA5); }
static bool readSet(uint8_t i) { tag[i] = 0xffff; return readBlk(CARD_SET >> 9, slotBuf(i)); }      // into slot i (the scratch one: SLOTS)

const uint8_t *settings() {                         // the three bytes, if the block holds a good set
    const uint8_t *h = buf[0];                       // (the first slot: this is read at boot, and when the debug hook reloads, mid-frame)
    return open_ && readSet(0) && h[0] == 'W' && h[4] == chk(h) ? h + 1 : nullptr;       // (any three bytes decode to a valid setting: loadSave() takes them modulo)
}

#ifdef CHSIM
// the simulator's card is read-only (CHSd/host/sd_host.cpp): with $WH_CARDW set the block goes into the file $CHSD_CARD names
// (a plain file: the DAT itself), for the tests; otherwise the write is refused
static bool writeBlk(uint32_t k, const uint8_t *src) {
    const char *path = getenv("WH_CARDW") ? getenv("CHSD_CARD") : nullptr;
    FILE *f = path ? fopen(path, "r+b") : nullptr;
    const bool ok = f && !fseek(f, (long)k * 512, SEEK_SET) && fwrite(src, 1, 512, f) == 512;
    if (f) fclose(f);
    return ok;
}
#else
static const uint32_t SPE = 1u << 6, RXNE = 1u << 0, BSY = 1u << 7;
__attribute__((noinline)) static uint8_t xfer(uint8_t b) {
    SPI1->DATAR = b;
    while (!(SPI1->STATR & RXNE)) {}
    return (uint8_t)SPI1->DATAR;
}
__attribute__((noinline)) static void spiSet(uint32_t c1) {
    while (SPI1->STATR & BSY) {}
    SPI1->CTLR1 = (uint16_t)(c1 & ~SPE);
    SPI1->CTLR1 = (uint16_t)c1;
}
__attribute__((noinline)) static uint8_t cmd(uint8_t c, uint32_t arg) {       // as SdSpi.cpp's: the CRC is off since sd::init()
    xfer(0xFF);
    xfer((uint8_t)(0x40 | c));
    for (int sh = 24; sh >= 0; sh -= 8) xfer((uint8_t)(arg >> sh));
    xfer(0x95);
    uint8_t r;
    uint32_t k = 9;
    do r = xfer(0xFF); while ((r & 0x80) && --k);
    return r;
}
static bool writeBlk(uint32_t k, const uint8_t *src) {
    const uint32_t lba = lbaOf(k);
    if (!lba) return false;
    GPIOA->BSHR = 1u << 4;                          // SdSpi.cpp's claim(): the panel deselected, SPI1 mode 0 at 12 MHz, the card selected
    const uint16_t lcdCtlr1 = (uint16_t)SPI1->CTLR1;
    spiSet((1u << 2) | (1u << 8) | (1u << 9) | SPE | (1u << 3));
    (void)SPI1->DATAR;
    (void)SPI1->STATR;
    GPIOB->BCR = 1u << 11;
    bool ok = !cmd(58, 0);                // READ_OCR: bit 30 = block addressing, as sd::init() reads it
    if (ok) {
        uint32_t ocr = 0;
        for (int i = 0; i < 4; i++) ocr = (ocr << 8) | xfer(0xFF);
        ok = !cmd(24, (ocr >> 30 & 1) ? lba : lba << 9);
    }
    if (ok) {
        for (int i = -1; i < 514; i++) xfer(i < 0 ? 0xFE : i < 512 ? src[i] : 0xFF);   // the token, the data, the CRC (unchecked)
        ok = (xfer(0xFF) & 0x1F) == 0x05;            // "data accepted"; the card is busy programming until the read-back's wait(false) sees it done (SdSpi.cpp, 1 s at most)
    }
    GPIOB->BSHR = 1u << 11;                         // SdSpi.cpp's release()
    xfer(0xFF);
    spiSet(lcdCtlr1);
    return ok;
}
#endif

__attribute__((noinline)) bool putSettings(uint8_t music, uint8_t fx, uint8_t pad) {   // after gfx_wait(), before the flush; false: not kept on the card (silently: the flash save has them)
    if (!open_ || noWrite) return false;
    uint8_t *b = gfx_chunkScratch(), h[5] = { 'W', music, fx, pad };
    h[4] = chk(h);
    bool ok = readSet(SLOTS);
    ok = ok && (b[0] == 'W' || (b[0] & b[1] & b[2] & b[3]) == 0xFF);              // only over the fill or an earlier set: the block is ours
    if (ok) { memcpy(b, h, 5); ok = writeBlk(CARD_SET >> 9, b) && readSet(SLOTS) && !memcmp(b, h, 5); }
    if (!ok) { noWrite = true; open_ = sd::init(); }    // a card left mid-command needs its CMD0 (the library's note); gone: the retry in loop() finds it
    return ok;
}

}  // namespace card
