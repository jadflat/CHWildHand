// Tiny snprintf stand-in: %d %ld %s %% only (newlib's printf costs flash and milliseconds on the CH32V).
#pragma once
#include <stdarg.h>
#include <stdint.h>

static int whFmtV(char *o, int n, const char *f, va_list ap) {
    int k = 0;
    char one[2] = { 0, 0 }, t[12];
    for (; *f; f++) {
        one[0] = *f;
        const char *s = one;                      // the character itself, or the argument below
        if (*f == '%') {
            f++;
            if (*f == 'l') f++;                   // %ld: long is int-sized here
            one[0] = *f;                          // %% or anything else: that character
            if (*f == 's') s = va_arg(ap, const char *);
            else if (*f == 'd') {
                int32_t v = va_arg(ap, int32_t);
                uint32_t u = v < 0 ? (uint32_t)0 - (uint32_t)v : (uint32_t)v;
                char *q = t + 11; *q = 0;
                do *--q = (char)('0' + u % 10); while (u /= 10);
                if (v < 0) *--q = '-';
                s = q;
            }
        }
        for (; *s; s++, k++) if (k < n - 1) o[k] = *s;
    }
    if (n > 0) o[k < n ? k : n - 1] = 0;
    return k;
}
static int whFmt(char *o, int n, const char *f, ...) { va_list ap; va_start(ap, f); int k = whFmtV(o, n, f, ap); va_end(ap); return k; }
// strf(): the same into one shared buffer, returned: use the string before the next strf() (no local buffers, two arguments less at each call)
static char fbuf[32];
static const char *strf(const char *f, ...) { va_list ap; va_start(ap, f); whFmtV(fbuf, sizeof fbuf, f, ap); va_end(ap); return fbuf; }
#define snprintf whFmt
