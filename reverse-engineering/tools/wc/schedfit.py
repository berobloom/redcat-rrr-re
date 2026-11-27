#!/usr/bin/env python3
"""schedfit.py U1.OBJ X1.OBJ [U2.OBJ X2.OBJ ...]: find tie-break orders for the scheduler
model in simsched.py that reproduce the real -ox code (X*.OBJ) from the -oeilm -s code (U*.OBJ)
of every basic block whose instructions are the same in both. Also: failing(pairs, key), show(...)."""
import itertools, re, sys
import simsched as S

FEATS = ['h_anc', 'h_desc', 'h_sum', 'stall', 'stall_noreg', 'nsucc', 'npred', 'flagw', 'adj', 'adjdata', 'id']


def blocks(ins):
    """split at jump targets and after jumps/rets (the scheduler's basic blocks)"""
    targets = set()
    for i in ins:
        t = i.text.split()
        if i.jump and len(t) == 2 and re.fullmatch(r'0x[0-9a-f]+', t[1]):
            targets.add(int(t[1], 16))
    out, cur = [], []
    for i in ins:
        if i.addr in targets and cur:
            out.append(cur); cur = []
        cur.append(i)
        if i.jump or i.mn == 'ret':
            out.append(cur); cur = []
    if cur:
        out.append(cur)
    return out


def core(block, first):
    """the part of a block the scheduler sees: no branch at the end (a block exit, not an
    instruction), no prologue (pushes, sub esp) and no epilogue (add esp, pops, ret), which the
    code generator adds after scheduling (LIKELY)"""
    b = list(block)
    for i in b:
        i.feeds_exit = False
    if b and b[-1].jump:
        cond = b[-1].mn != 'jmp'
        b = b[:-1]
        if cond and b:
            b[-1].feeds_exit = True
    if b and b[-1].mn == 'ret':
        b = b[:-1]
        while b and b[-1].mn == 'pop':
            b = b[:-1]
        if b and b[-1].text.startswith('add esp'):
            b = b[:-1]
    if first:
        while b and b[0].mn == 'push':
            b = b[1:]
        if b and b[0].text.startswith('sub esp'):
            b = b[1:]
    return b


def prep(block):
    S.set_call_args(block)
    S.set_cc_used(block)
    for k, i in enumerate(block):
        i.id = k
        i.deps = [block[j] for j in range(k) if S.order_dep(i, block[j], block)]
    succ = {id(i): [] for i in block}
    for i in block:
        for p in i.deps:
            succ[id(p)].append(i)
    for i in block:
        i.h_anc = max((p.h_anc for p in i.deps), default=0) + S.unit(i)[2]
        i.npred = len(i.deps)
    for i in reversed(block):
        i.h_desc = max((q.h_desc for q in succ[id(i)]), default=0) + S.unit(i)[2]
        i.nsucc = len(succ[id(i)])
    for i in block:
        i.stall = i.stallable
        i.h_sum = i.h_anc + i.h_desc
        i.flagw = int(i.flags_w)


def run(block, key):
    for i in block:
        i.anc = i.nsucc
    ready = [i for i in block if i.anc == 0]
    out = []
    while ready:
        top = out[0] if out else None
        for c in ready:
            c.adj = int(top is not None and c in top.deps)
            c.adjdata = int(top is not None and bool(c.wr & top.rd or c.rd & top.wr))
        best = max(ready, key=lambda c: (-S.stall_cost(c, out, block),) + tuple(s * getattr(c, f) for f, s in key))
        ready.remove(best)
        out.insert(0, best)
        for p in best.deps:
            p.anc -= 1
            if p.anc == 0:
                ready.append(p)
    return [i.text for i in out]


def collect(pairs):
    """pairs: [(UNSCHED.OBJ, REAL.OBJ)]: every block of every public function whose instructions
    are the same multiset in both (so only the order differs)"""
    cases = []
    for uobj, robj in pairs:
        (_, recs), = list(S.L.modules(open(uobj, 'rb').read()))
        funcs = sorted({S.M.plain(p[2]) for p in S.M.publics(recs) if p[0] in S.L.code_of(recs)})
        for f in funcs:
            try:
                u = S.load(uobj, f); r = S.load(robj, f)
            except Exception:
                continue
            ub, rb = blocks(u), blocks(r)
            if len(ub) != len(rb):
                continue
            for k, (a, b) in enumerate(zip(ub, rb)):
                a, b = core(a, k == 0), core(b, k == 0)
                if len(a) > 2 and len(a) < 400 and not any(x.fp for x in a) and sorted(x.text for x in a) == sorted(x.text for x in b):
                    prep(a)
                    cases.append((f, a, [x.text for x in b]))
    return cases


def main():
    args = sys.argv[1:]
    pairs = [(args[k], args[k + 1]) for k in range(0, len(args), 2)]
    cases = collect(pairs)
    print('%d blocks' % len(cases), file=sys.stderr)
    keys = []
    for n in (1, 2, 3):
        for fs in itertools.permutations([x for x in FEATS if x != 'id'], n):
            for signs in itertools.product((1, -1), repeat=n):
                for idsign in (1, -1):
                    keys.append(tuple(zip(fs, signs)) + (('id', idsign),))
    keys.append((('id', 1),)); keys.append((('id', -1),))
    res = []
    for key in keys:
        bad = sum(1 for f, a, want in cases if run(a, key) != want)
        res.append((bad, key))
    res.sort(key=lambda x: (x[0], len(x[1])))
    for bad, key in res[:12]:
        fails = [(f, a[0].addr) for f, a, want in cases if run(a, key) != want]
        print(bad, ' > '.join('%s%s' % ('+' if s > 0 else '-', f) for f, s in key), fails[:4])


if __name__ == '__main__':
    main()


def show(uobj, robj, func, addr, keytxt):
    key = []
    for part in keytxt.split('>'):
        part = part.strip(); key.append((part[1:], 1 if part[0] == '+' else -1))
    u = S.load(uobj, func); r = S.load(robj, func)
    for a, b in zip(blocks(u), blocks(r)):
        if a[0].addr == addr:
            prep(a)
            got = run(a, tuple(key))
            for k in range(max(len(got), len(b))):
                g = got[k] if k < len(got) else ''; w = b[k].text if k < len(b) else ''
                print('%s %-40s %s' % (' ' if g == w else '*', w, g))


def failing(pairs, keytxt, limit=8, maxlen=14):
    key = []
    for part in keytxt.split('>'):
        part = part.strip(); key.append((part[1:], 1 if part[0] == '+' else -1))
    key = tuple(key)
    cases = collect(pairs)
    shown = 0
    for f, a, want in sorted(cases, key=lambda c: len(c[1])):
        got = run(a, key)
        if got != want and len(a) <= maxlen:
            print('==', f, hex(a[0].addr), len(a))
            for k in range(len(want)):
                print('%s %-42s %s' % (' ' if got[k] == want[k] else '*', want[k], got[k]))
            shown += 1
            if shown >= limit:
                break
