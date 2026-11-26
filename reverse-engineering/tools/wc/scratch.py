"""scratch.py: the game's code for one function as decomp.me target assembly.

usage: scratch.py PROG NAME 0xADDR OURS.OBJ OUTDIR

Writes OUTDIR/target.asm (MASM syntax: decomp.me assembles its Watcom platform with JWasm) and
OUTDIR/target.s (the same instructions in GNU syntax), then assembles target.s with GNU as and
checks it against the game's bytes outside the relocation fields - the MASM file differs from
it only in syntax.

Every absolute address in the function comes from the executable's own fixup records, not from
guessing which immediates are addresses. Its symbol is the external of OURS.OBJ (the module our
C compiles to) that the address falls in, as symbol+offset - so the target's names are the ones
decomp.me's compile of our C produces. Calls name the callee the same way: OURS.OBJ's external
whose rrr_names.txt address is the call target. Branches inside the function become labels.
Fails loudly on anything it cannot place (a fixup with no external, a jump table, a call to a
function OURS.OBJ does not reference)."""
import os
import re
import struct
import subprocess
import sys

import capstone

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, HERE)
from lelib import Image, ROOT  # noqa: E402
import leparse  # noqa: E402
import libmatch as L  # noqa: E402


def names_by_addr(prog):
    out = {}
    for line in open(os.path.join(ROOT, 'reverse-engineering', 'analysis', prog + '_names.txt')):
        f = line.split('#')[0].split()
        if len(f) >= 2:
            out.setdefault(f[1], int(f[0], 16))
    return out


def game_fixups(prog):
    """{linear address of a fixup field: target linear address} for the 32-bit offset fixups"""
    exe = {'rrr': 'RRR.EXE', 'reken': 'REKEN.EXE', 'rrrmenu': 'RRRMENU.EXE', 'intro': 'INTRO.EXE'}[prog]
    le = leparse.LE(open(os.path.join(ROOT, '.work', 'iso', exe), 'rb').read())
    out = {}
    for o in le.objects:
        for k in range(o['page_count']):
            for stype, so, tobj, toff in le.fixups(o['page_index'] + k):
                if stype == 0x07:
                    out[o['base'] + k * le.page_size + so] = le.objects[tobj - 1]['base'] + toff
    return out


def func_range(img, md, addr):
    """the function's instructions, ending as disasm.py --func does"""
    far, out = addr, []
    for ins in md.disasm(img.read(addr, 0x10000), addr):
        out.append(ins)
        if ins.mnemonic.startswith('j') and ins.op_str.startswith('0x'):
            t = int(ins.op_str, 16)
            if addr <= t < addr + 0x4000:
                far = max(far, t)
        if ins.mnemonic in ('ret', 'retf', 'iretd') or (ins.mnemonic == 'jmp' and ins.address >= far):
            if ins.address >= far:
                return out
    raise SystemExit('no end found')


def hexm(v):                       # MASM hex
    s = '%x' % v
    return ('0' + s if s[0] in 'abcdef' else s) + 'h'


def main():
    prog, name, addr, obj, outdir = sys.argv[1], sys.argv[2], int(sys.argv[3], 0), sys.argv[4], sys.argv[5]
    os.makedirs(outdir, exist_ok=True)
    img = Image(prog)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    inss = func_range(img, md, addr)
    end = inss[-1].address + inss[-1].size
    fix = {a: t for a, t in game_fixups(prog).items() if addr <= a < end}

    (_, recs), = list(L.modules(open(obj, 'rb').read()))
    exts = sorted({f[5] for f in L.fixups_of(recs, only_code=False) if f[4] == 'ext'})
    byaddr = names_by_addr(prog)
    def plain(e):                  # Watcom names: data _name, functions name_
        return e[1:] if e.startswith('_') else e[:-1] if e.endswith('_') else e
    ext_at = sorted((byaddr[plain(e)], e) for e in exts if plain(e) in byaddr)
    missing = [e for e in exts if plain(e) not in byaddr]
    if missing:
        raise SystemExit('externals with no address in %s_names.txt: %s' % (prog, missing))
    sym_of_call = {a: e for a, e in ext_at}

    def data_sym(t):
        best = None
        for a, e in ext_at:
            if a <= t and not e.endswith('_'):
                best = (a, e)
        if best is None:
            raise SystemExit('no external of %s holds 0x%x' % (obj, t))
        return best[1], t - best[0]

    targets = {int(i.op_str, 16) for i in inss
               if i.mnemonic.startswith('j') and re.fullmatch(r'0x[0-9a-f]+', i.op_str)}
    used_data, used_funcs, gas, masm = set(), set(), [], []
    covered = set()
    for ins in inss:
        lab = 'L%x' % ins.address
        if ins.address in targets:
            gas.append(lab + ':'); masm.append(lab + ':')
        op = ins.op_str
        f_here = [a for a in fix if ins.address <= a < ins.address + ins.size]
        if ins.mnemonic == 'call' or (ins.mnemonic.startswith('j') and re.fullmatch(r'0x[0-9a-f]+', op)):
            t = int(op, 16)
            if addr <= t < end:
                g = m = 'L%x' % t
            elif t in sym_of_call:
                g = m = sym_of_call[t]; used_funcs.add(g)
            else:
                raise SystemExit('0x%x: %s to 0x%x, which %s does not reference' % (ins.address, ins.mnemonic, t, obj))
            gas.append('    %s %s' % (ins.mnemonic, g)); masm.append('    %s %s' % (ins.mnemonic, m))
            continue
        gop = mop = op
        for a in f_here:
            covered.update(range(a, a + 4))
            t = fix[a]
            v = struct.unpack_from('<I', img.read(a, 4))[0]
            sym, off = data_sym(t)
            used_data.add(sym)
            lit = '0x%x' % v
            if lit not in op:
                raise SystemExit('0x%x: fixup value %s not in "%s"' % (ins.address, lit, op))
            gsub = sym + ('+%d' % off if off else '')
            msub = sym + ('+' + hexm(off) if off else '')
            inmem = re.search(r'\[[^\]]*' + lit + r'[^\]]*\]', op)
            if not inmem:              # an address as an immediate
                gsub, msub = 'OFFSET ' + gsub, 'offset ' + msub
            gop = re.sub(lit + r'\b', gsub, gop, count=1)
            mop = re.sub(lit + r'\b', msub, mop, count=1)
        mop = re.sub(r'(?<![\w+])0x([0-9a-f]+)', lambda m_: hexm(int(m_.group(1), 16)), mop)
        mop = re.sub(r'\+(\w+)\+0x([0-9a-f]+)', lambda m_: '+%s+%s' % (m_.group(1), hexm(int(m_.group(2), 16))), mop)
        gas.append('    %s %s' % (ins.mnemonic, gop) if gop else '    ' + ins.mnemonic)
        masm.append('    %s %s' % (ins.mnemonic, mop) if mop else '    ' + ins.mnemonic)
    stray = [a for a in fix if not any(x in covered for x in range(a, a + 4))]
    if stray:
        raise SystemExit('fixups outside instruction operands (a jump table?): %s' % [hex(a) for a in stray])

    fn = name + '_'
    open(os.path.join(outdir, 'target.s'), 'w').write('\n'.join(
        ['.intel_syntax noprefix', '.globl ' + fn, fn + ':'] + gas) + '\n')
    hdr = ['.386p', '.model flat', '']
    hdr += ['extrn %s:dword' % s for s in sorted(used_data)]
    hdr += ['extrn %s:near' % s for s in sorted(used_funcs)]
    hdr += ['', "_TEXT segment dword public use32 'CODE'", 'public ' + fn, fn + ' proc near']
    open(os.path.join(outdir, 'target.asm'), 'w').write('\n'.join(
        hdr + masm + [fn + ' endp', '_TEXT ends', 'end']) + '\n')

    # check: GNU as of target.s == the game's bytes outside the relocation fields
    o = os.path.join(outdir, 'check.o')
    subprocess.run(['as', '--32', '-o', o, os.path.join(outdir, 'target.s')], check=True)
    sec = subprocess.run(['objcopy', '-O', 'binary', '-j', '.text', o, '/dev/stdout'],
                         check=True, capture_output=True).stdout
    rel = subprocess.run(['objdump', '-r', o], check=True, capture_output=True, text=True).stdout
    relf = set()
    for line in rel.splitlines():
        p = line.split()
        if len(p) >= 3 and re.fullmatch(r'[0-9a-f]{8}', p[0]):
            relf.update(range(int(p[0], 16), int(p[0], 16) + 4))
    game = img.read(addr, end - addr)
    bad = [i for i in range(max(len(sec), len(game)))
           if i not in relf and (i >= len(sec) or i >= len(game) or sec[i] != game[i])]
    print('%s: %d bytes, %d instructions, %d fixups, %d relocations; %s' % (
        name, end - addr, len(inss), len(fix), len(relf) // 4,
        'bytes identical outside relocations' if not bad else
        'MISMATCH at offsets %s' % [hex(i) for i in bad[:12]]))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
