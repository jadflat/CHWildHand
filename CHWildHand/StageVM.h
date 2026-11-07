// StageVM.h: bonus stages as small scripts on the SD card (tools/stage_asm.py writes them into WILDHAND.DAT, CARD_STAGE).
// A stage is miniK = MK_VM + v (v < STAGE_N, CardIndex.h); its name and rule lines are text block STAGE_TXG (strings v, N + v, 2N + v).
// Blob (<= 448 B): a 6-byte header [u16 offsets: the update program, the draw program, the end], the init program from 6. The
// script ends the stage (WIN, LOSE: then the usual pause and timeUp); it draws its own score line. While the stage runs, the blob and the registers live in one of
// the card cache's slots (card::hold). Registers r0..r31 (int16): r0 wins, r1 losses, r2 frames, r3 level (night / 3, at most 5),
// r8 the debug cheat (`say I 102`), r9 the frame counter (drawing), the rest the script's. Opcode bit 7: the destination register is
// the first value too. Operands: 0..222 a literal, 223 an int16 literal follows, 224..255 a register. Jumps are relative to the next
// instruction (biased by VM_JB), inside the same program. Included from Minis.h. A bad blob cannot hang or crash the game: the pc stays in the program and the
// buffer, a program runs at most VM_STEPS steps a frame, colours, radii, indices and lines are clamped.
#pragma once

static void smallCard(int x, int y, Card c, bool up, uint8_t edge);
enum { VM_STEPS = 1500, VM_END = 40, VM_CODE = 448, VM_JB = 64 };
struct VM { uint8_t code[VM_CODE]; int16_t R[32]; };    // (LD and ST index the registers: r16..r31 as an array)
static VM *vm;                                         // the running stage (in the held cache slot); null until it is loaded
static uint8_t vmEndT, vmWon, shoe[13], shoeN;                         // the pause after the end, the result
// the opcodes' operands: bit 3 a destination register first, bits 0-2 the values after it (stage_asm.py OPS: the same order)
#define VMOPS ((const uint8_t *)(wh_tabs + T_VMOPS))   // (in RAM, from the card: tools/tables.py; the layout is in the comment there)
enum { VM_NOPS = 43 };

static_assert(sizeof(VM) <= 512, "the machine fits a cache slot");
// a card turning over; w 0..32: 0 a full back, 16 an edge, 32 a full face (a wide card: the CARD, a back is rank 13)
static void flipCard(int x, int y, int r, int su, int w, uint8_t e) {
    const bool bk = w < 16;
    const int aw = bk ? 16 - w : w - 16;
    if (bk) r = 13;
    if (aw < 13) { if (aw) frame(x + ((16 - aw) >> 1), y, aw, 22, e, bk ? P_WINE : P_CARD); return; }
    smallCard(x, y, { (uint8_t)(r % 13), (uint8_t)(su & 3) }, (unsigned)r < 13, e);
}
static void vmEnd(bool won) { if (!vmEndT) { vmWon = won; vmEndT = VM_END; } }
static const char *vmText(uint8_t i) { return cardText(STAGE_TXG, i); }
__attribute__((noinline, optimize("jump-tables"))) static void vmRun(unsigned pc, unsigned end) {   // (a table: smaller than the file's compare chains here)
    VM &m = *vm;
    const unsigned start = pc;
    for (int steps = VM_STEPS; pc < end && steps; steps--) {
        const uint8_t raw = m.code[pc++], op = raw & 127;
        if (op >= VM_NOPS) return;
        const uint8_t f = VMOPS[op];
        uint8_t d = 0;
        int v[6], i = 0;
        if (f & 8) d = m.code[pc++] & 31;
        if (raw & 128) v[i++] = m.R[d];                  // (bit 7: the destination is the first value too, ADD X, X, 1)
        for (; i < (f & 7); i++) {
            uint8_t b = m.code[pc++];
            if (b == 223) { v[i] = (int16_t)(m.code[pc] | m.code[pc + 1] << 8); pc += 2; }
            else v[i] = b < 223 ? b : m.R[b - 224];
        }
        const int a = v[0], b = v[1];
        int r = 0, jt = 0x10000;
#define c3 ((uint8_t)(v[3] & 15))
#define c4 ((uint8_t)(v[4] & 15))
        switch (op) {
        case 0: return;
        case 1: r = a; break;
        case 2: r = a + b; break;
        case 3: r = a - b; break;
        case 4: r = a * b; break;
        case 5: r = a / b; break;
        case 6: r = a % b; break;
        case 7: r = a & b; break;
        case 8: {   // (DRAW: a rank from the shoe, 4 decks, reshuffled when empty; empty at each stage start)
            if (!shoeN) { memset(shoe, 16, sizeof shoe); shoeN = 208; }
            int n = (int)(rnd() % shoeN), k = 0;
            for (; (n -= shoe[k]) >= 0; k++) {}
            shoe[k]--; shoeN--;
            r = k;
        } break;
        case 9: r = a >> (b & 15); break;
        case 10: r = a < b ? a : b; break;
        case 11: r = a > b ? a : b; break;
        case 12: r = (int)(rnd() % (uint32_t)a); break;
        case 13: r = isin(a) * b >> 6; break;
        case 14: r = a < 0 ? -a : a; break;
        case 15: r = m.R[a & 31]; break;
        case 16: m.R[a & 31] = (int16_t)b; break;
        case 17: jt = a; break;
        case 18: if (a == b) jt = v[2]; break;
        case 19: if (a != b) jt = v[2]; break;
        case 20: if (a < b) jt = v[2]; break;
        case 21: if (a >= b) jt = v[2]; break;
        case 22: r = m.R[d] - 1; if (r) jt = a; break;
        case 23: r = chgame.buttons() & a; break;
        case 24: r = chgame.justPressedMask() & a; break;
        case 25: playSfx((Sfx)(a < (int)Sfx::COUNT ? a : 0), (uint8_t)b); break;
        case 26: flashT = (uint8_t)a; break;
        case 27: burst(a, b, v[2] & 31, c3, c4, v[5] & 63); break;
        case 28: case 29: vmEnd(op == 28); return;
        case 30: gfx_fillRect(a, b, v[2], v[3], c4); break;
        case 31: gfx_rect(a, b, v[2], v[3], c4); break;
        case 33: gfx_fillCircle(a, b, v[2] & 63, c3); break;
        case 34: gfx_circle(a, b, v[2] & 63, c3); break;
        case 35: if (!((a + 128 | b + 128 | v[2] + 128 | v[3] + 128) >> 9)) gfx_line(a, b, v[2], v[3], c4); break;   // (a pixel at a time: no far ends)
        case 36: hline(a, b, v[2], c3); break;
        case 37: { const char *s = strf(vmText((uint8_t)v[2]), v[4], v[5]); if (a < 0) textOutC(b, s, c3); else textOut(a, b, s, c3); } break;   // (a format: up to two %d; x < 0: centred)
        case 32: flipCard(a, b, v[2], v[2] & 3, v[3], c4); break;   // (FLIP x, y, rank, w, edge)
        case 38: smallCard(a, b, { (uint8_t)(v[2] % 13), (uint8_t)(v[3] & 3) }, (unsigned)v[2] < 13, c4); break;
        case 39: suit7(a & 3, b, v[2], c3, v[4] & 3); break;
        case 42: {   // (blackjack: the hand's b cards (r16..) and the split's waiting hands (v[2]: base-16 digits, rank + 1): a slot each, cards side by side at y = a)
            int h = 1, s = v[2];
            for (; s; s >>= 4) h++;
            const int w = 128 / h;
            for (int j = 0; j < h; j++) {
                const int c = j ? 1 : b, p = 15 * c > w ? 6 : 15;
                for (int i = 0; i < c; i++) flipCard(j * w + ((w - p * c) >> 1) + p * i, a, j ? (v[2] >> 4 * j - 4 & 15) - 1 : m.R[16 + i], i, j ? 32 : v[3] - 12 * i < 0 ? 0 : v[3] - 12 * i, j ? 14 : 4);
            }
        } break;
        }
#undef c3
#undef c4
        if (f & 8) m.R[d] = (int16_t)r;
        if (jt != 0x10000) { pc += jt - VM_JB; if (pc - start >= end - start) return; }
    }
}
static void vmStart() { vm = nullptr; vmEndT = 0; }      // (loaded by the next drawing: the card is read only then)
static const uint16_t *vmH() { return (const uint16_t *)vm->code; }   // [0] the update program, [1] the draw program, [2] the end
// the stage's blob into the held cache slot, the registers cleared, the init program run (drawing time: it reads the card)
static void vmLoad() {
    const uint8_t v = (uint8_t)(miniK - MK_VM), *d = card::at(CARD_STAGE + 4u * v);
    if (!d || v >= STAGE_N) return;
    const unsigned n = d[2] | d[3] << 8;
    VM *m = (VM *)card::hold(CARD_STAGE + (d[0] | d[1] << 8), n < VM_CODE ? n : VM_CODE);
    if (!m) return;
    memset(m->R, 0, sizeof m->R);
    shoeN = 0;
    vm = m;
    int lv = round_ / 3;
    m->R[3] = (int16_t)(lv > 5 ? 5 : lv);
    vmRun(6, vmH()[0]);
}
static void vmUpdate() {
    VM *m = vm;
    if (!m) return;
    if (vmEndT) { if (!--vmEndT) { vm = nullptr; card::release(); timeUp(vmWon); } return; }
    m->R[2]++;
    vmRun(vmH()[0], vmH()[1]);
}
static void vmDraw() {
    if (!vm) vmLoad();
    VM *m = vm;
    if (!m) return;
    m->R[9] = (int16_t)fc;
    vmRun(vmH()[1], vmH()[2]);
}
