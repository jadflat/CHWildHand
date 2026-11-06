// WILDHAND.DAT on the SD card (Card.cpp): the pictures, the songs (with their headers), the text and the sound effects, read through a five-block cache.
// The DAT's size limit is the cache's 16-bit block tags: block numbers below 65535, so 32 MB less 512 B (the file may be in up to four cluster runs).
// Every call reads the card over SPI1, which the panel shares: call them only between gfx_wait() and gfx_flushAsync().
#pragma once
#include <stdint.h>

namespace card {
bool open();                                       // finds and checks the file; false: no card, no file or the wrong one
bool ok();                                         // the file is open
void frame();                                      // after gfx_wait(), each frame: the scratch slot is stale
extern bool lowPri;                                // while set, at() gives up (nullptr) rather than read a third block this frame
const uint8_t *at(uint32_t off);                   // the byte at off and the rest of its 512 B block (nullptr: the card has gone)
uint8_t *hold(uint32_t off, uint16_t n);           // n bytes into a 512 B slot taken out of the cache (nullptr: the card has gone) ...
void release();                                    // ... and back into it
bool copyDirect(uint8_t *dst, uint32_t off, uint16_t n);   // n bytes (may span blocks), before the frame is drawn; the cache keeps its pictures
const uint8_t *settings();                         // the settings kept on the card (3 packed bytes, in the scratch buffer), nullptr if none or not good; before the first frame is drawn
bool putSettings(uint8_t music, uint8_t fx, uint8_t pad);   // keeps 3 packed bytes in the DAT's last block (one block write, a few ms); false: not kept
}  // namespace card

// A reader of run-length picture data: from flash (fromCard false), or from the card a block at a time
struct Rd {
    const uint8_t *p; uint32_t off; uint16_t left; bool fromCard;
    uint8_t next() {
        if (fromCard && !left) { p = card::at(off); left = (uint16_t)(512 - (off & 511)); if (!p) { left = 0; return 0xFF; } }
        left--; off++;
        return *p++;
    }
};
