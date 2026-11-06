// StageVM.h: bonus stages as small scripts on the SD card (tools/stage_asm.py writes them into WILDHAND.DAT, CARD_STAGE).
// A stage is miniK = MK_VM + v (v < STAGE_N, CardIndex.h); its name and rule lines are text block STAGE_TXG (strings v, N + v, 2N + v).
// Blob (<= 448 B): a 6-byte header [u16 offsets: the update program, the draw program, the end], the init program from 6. The
// script ends the stage (WIN, LOSE: then the usual pause and timeUp); it draws its own score line. While the stage runs, the blob and the registers live in one of
// the card cache's slots (card::hold). Registers r0..r31 (int16): r0 wins, r1 losses, r2 frames, r3 level (night / 3, at most 5),
// r8 the debug cheat (`say I 102`), r9 the frame counter (drawing), the rest the script's. Opcode bit 7: the destination register is
// the first value too. Operands: 0..222 a literal, 223 an int16 literal follows, 224..255 a register. Jumps go to blob offsets
// inside the same program. Included from Minis.h. A bad blob cannot hang or crash the game: the pc stays in the program and the
// buffer, a program runs at most VM_STEPS steps a frame, colours, radii, indices and lines are clamped.
#pragma once

static void smallCard(int x, int y, Card c, bool up, uint8_t edge);
enum { VM_STEPS = 1500, VM_END = 40, VM_CODE = 448 };
struct VM { uint8_t code[VM_CODE]; int16_t R[32]; };    // (LD and ST index the registers: r16..r31 as an array)
static VM *vm;                                         // the running stage (in the held cache slot); null until it is loaded
static uint8_t vmEndT, vmWon;                          // the pause after the end, the result
// the opcodes' operands: bit 3 a destination register first, bits 0-2 the values after it (stage_asm.py OPS: the same order)
static const uint8_t VMOPS[] = {
    0, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 9, 10, 9, 9, 2,     // END SET ADD SUB MUL DIV MOD AND OR SHR MIN MAX RND SIN ABS LD ST
    1, 3, 3, 3, 3, 9, 9, 9,                                         // JMP JEQ JNE JLT JGE DJNZ IN EDGE
    2, 1, 6, 0, 0,                                                  // SFX FLASH BURST WIN LOSE
    5, 5, 5, 4, 4, 5, 4, 6, 5, 5, 3, 2 };                           // FRECT RECT RRECT FCIRC CIRC LINE HLINE TEXT CARD SUIT HEAD HAND
enum { VM_NOPS = sizeof VMOPS };

static_assert(sizeof(VM) <= 512, "the machine fits a cache slot");
static void vmEnd(bool won) { if (!vmEndT) { vmWon = won; vmEndT = VM_END; } }
static const char *vmText(uint8_t i) { return cardText(STAGE_TXG, i < STAGE_STRS ? i : 0); }
__attribute__((noinline, optimize("jump-tables"))) static void vmRun(unsigned pc, unsigned end) {   // (a table: smaller than the file's compare chains here)
    VM &m = *vm;
    const unsigned start = pc;
    if (end > VM_CODE) end = VM_CODE;
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
        int r = 0, jt = -1;
#define c3 ((uint8_t)(v[3] & 15))
#define c4 ((uint8_t)(v[4] & 15))
        switch (op) {
        case 0: return;
        case 1: r = a; break;
        case 2: r = a + b; break;
        case 3: r = a - b; break;
        case 4: r = a * b; break;
        case 5: r = b ? a / b : 0; break;
        case 6: r = b ? a % b : 0; break;
        case 7: r = a & b; break;
        case 8: r = a | b; break;
        case 9: r = a >> (b & 15); break;
        case 10: r = a < b ? a : b; break;
        case 11: r = a > b ? a : b; break;
        case 12: r = a > 0 ? (int)(rnd() % (uint32_t)a) : 0; break;
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
        case 32: rr2(a, b, v[2], v[3], c4); break;
        case 33: gfx_fillCircle(a, b, v[2] & 63, c3); break;
        case 34: gfx_circle(a, b, v[2] & 63, c3); break;
        case 35: if ((unsigned)(a + 128) < 384 && (unsigned)(b + 128) < 384 && (unsigned)(v[2] + 128) < 384 && (unsigned)(v[3] + 128) < 384) gfx_line(a, b, v[2], v[3], c4); break;   // (a pixel at a time: no far ends)
        case 36: hline(a, b, v[2], c3); break;
        case 37: { const char *s = strf(vmText((uint8_t)v[2]), v[4], v[5]); if (a < 0) textOutC(b, s, c3); else textOut(a, b, s, c3); } break;   // (a format: up to two %d; x < 0: centred)
        case 38: smallCard(a, b, { (uint8_t)(v[2] % 13), (uint8_t)(v[3] & 3) }, (unsigned)v[2] < 13, c4); break;
        case 39: suit7(a & 3, b, v[2], c3, v[4] & 3); break;
        case 40: drawDealerHead(a, b, v[2] & 31); break;
        case 41: drawHand(a, b); break;
        }
#undef c3
#undef c4
        if (f & 8) m.R[d] = (int16_t)r;
        if (jt >= 0) { if ((unsigned)jt < start || (unsigned)jt >= end) return; pc = (unsigned)jt; }
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
