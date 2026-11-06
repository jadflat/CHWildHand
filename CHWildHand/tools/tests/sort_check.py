"""Check the hand sort (Rules.h sortHand) against a brute-force model.

Reads a simulator log with the debug lines `KB n r.s ...` / `KA n r.s ...` (`say k n`: a hand before and after the
sort) and `HAND n r.s ...` (`say h`: the hand as dealt in play). For each sorted hand: the same cards; the face-down
cards ("h") last, in their old order; and the face-up cards in the order the model expects: the best made hand of
what is left (every subset of 2..5 cards scored by a port of evaluate(): the highest type, then the highest ranks),
again and again, then the loose cards high to low. Each group must also score as its type with every card scoring.
usage: python sort_check.py <log>
"""
import itertools
import re
import sys

GLEN = {8: 5, 7: 4, 6: 5, 5: 5, 4: 5, 3: 3, 2: 4, 1: 2}


def evaluate(c):                      # CHWildHand.ino evaluate(), card = (r, s)
    n = len(c)
    sm = [sum(1 for d in c if d[0] == x[0]) for x in c]
    s2 = sum(k == 2 for k in sm); s3 = sum(k == 3 for k in sm); s4 = sum(k == 4 for k in sm)
    mask = 0
    for x in c: mask |= 1 << x[0]
    lo = min(x[0] for x in c); hi = max(x[0] for x in c)
    flush = n == 5 and all(x[1] == c[0][1] for x in c)
    straight = n == 5 and not (s2 or s3 or s4) and (hi - lo == 4 or mask == 0x100F)
    t = 8 if straight and flush else 7 if s4 else 6 if s3 and s2 else 5 if flush else 4 if straight else 3 if s3 else (2 if s2 == 4 else 1) if s2 else 0
    scoring = [t in (8, 6, 5, 4) or (t and sm[i] >= 2) for i in range(n)]
    return t, scoring


def shown(c, t):                      # a group's ranks as it should lie: high to low, ace high (the wheel: A 5 4 3 2); a full house: the three first
    rs = [x[0] for x in c]
    if t == 6: return sorted(rs, key=lambda r: (-rs.count(r), -r))
    return sorted(rs, reverse=True)


def key(c, t):                        # which group of a type is the best: straights by the top card (the wheel: 5), else by count then rank
    rs = [x[0] for x in c]
    if t in (8, 4): return (t, 3 if sorted(rs) == [0, 1, 2, 3, 12] else max(rs))
    return (t, tuple(sorted(rs, key=lambda r: (-rs.count(r), -r))))


def expect(cards):                    # the model: face-up cards -> rank order
    left = list(cards); out = []
    while True:
        best = None
        for n in (2, 3, 4, 5):
            for sub in itertools.combinations(left, n):
                t, sc = evaluate(list(sub))
                if t and all(sc) and GLEN[t] == n:
                    k = key(sub, t)
                    if best is None or k > best[0]: best = (k, sub, t)
        if not best: break
        out.append((best[2], shown(best[1], best[2])))
        for x in best[1]: left.remove(x)
    return out, sorted((x[0] for x in left), reverse=True)


def parse(line):
    p = line.split()
    n = int(p[1]); cs = []
    for tok in p[2:2 + n]:
        h = tok.endswith('h'); r, s = tok.rstrip('h').split('.')
        cs.append((int(r), int(s), h))
    return cs


def check(after, before=None):
    up = [(r, s) for r, s, h in after if not h]
    hid = [(r, s) for r, s, h in after if h]
    k = len(up)
    if any(h for _, _, h in after[:k]): return "face-down cards not last"
    if before is not None:
        if sorted(before) != sorted(after): return "cards changed"
        if [(r, s) for r, s, h in before if h] != hid: return "face-down order changed"
    groups, loose = expect(up)
    pos = 0
    for t, ranks in groups:
        g = up[pos:pos + len(ranks)]
        if [x[0] for x in g] != ranks: return "group %d expected %s got %s" % (t, ranks, [x[0] for x in g])
        et, sc = evaluate(g)
        if et != t or not all(sc): return "group does not score as type %d" % t
        pos += len(ranks)
    if [x[0] for x in up[pos:]] != loose: return "loose cards expected %s got %s" % (loose, [x[0] for x in up[pos:]])
    return None


def main():
    lines = open(sys.argv[1], encoding="utf-8", errors="replace").read().splitlines()
    before = None; n = bad = 0; types = {}
    for ln in lines:
        m = re.search(r'\b(KB|KA|HAND) \d.*', ln)
        if not m: continue
        cs = parse(m.group(0))
        if m.group(1) == 'KB': before = cs; continue
        e = check(cs, before if m.group(1) == 'KA' else None)
        before = None; n += 1
        for t, _ in expect([(r, s) for r, s, h in cs if not h])[0]: types[t] = types.get(t, 0) + 1
        if e:
            bad += 1
            if bad <= 10: print("BAD", e, "|", m.group(0))
    print("checked %d hands, %d bad; groups by type %s" % (n, bad, dict(sorted(types.items()))))
    sys.exit(1 if bad or not n else 0)


if __name__ == "__main__":
    main()
