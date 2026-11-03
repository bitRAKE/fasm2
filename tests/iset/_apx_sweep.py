"""_apx_sweep.py -- every APX extended-register form fasm2 accepts must be right.

    python tests/iset/_apx_sweep.py [-v]

r16-r31 are register elements (x86-2.inc), so every mnemonic can see them.
An encoder that does not know them masks the register number and writes
another register (add rax, r16 -> add rax, rax), so an accepted form is
only right if its bytes are. This sweep checks both directions at once.

Every instruction name (from _dupnames.py's scan of x86-2.inc and iset/*.inc,
plus the mnemonics in zydis-db) is first probed bare; names fasm2 does not
define are dropped. Each defined name is then assembled with each operand
pattern in PATTERNS, which all name r16, by fasm2 and by llvm-mc
(_apx_diff.py). Each form that fasm2 accepts is assembled again with r16
replaced by each register in EDGE (r20: SIB escape, r21: needs a
displacement, r24/r28/r29: bit 3 too, r31: all bits).

Failures:
  DIFF  fasm2 and llvm-mc both accept the form with different bytes;
  FASM  fasm2 accepts a form llvm-mc refuses.
Either is fine only when listed in REVIEWED with the reason (for example
when Intel's specification, which outranks LLVM, says otherwise).

Reported, not failures: forms only llvm-mc accepts (APX encodings fasm2
does not have yet; -v lists them by mnemonic).
"""
import os, re, sys, json, collections

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
from _dupnames import defs	# noqa: E402
import _apx_diff	# noqa: E402

ZYDIS_DB = r'C:\git\zyantific\zydis-db\datafiles\instructions.json'

# Intel operand order; r16 is replaced by the EDGE registers in pass 2
PATTERNS = [
    # GPR alone and in pairs, each size
    'r16', 'r16d', 'r16w', 'r16b', 'r16l',
    'r16, rax', 'rax, r16', 'r16d, eax', 'eax, r16d', 'r16w, ax', 'ax, r16w',
    'r16b, al', 'al, r16b', 'r16, 1', 'r16d, 1', 'r16, cl',
    'r16, rax, 1', 'rax, r16, 1', 'r16, [rax], 1', 'rax, r16, cl',
    # three-operand GPR (BMI, CMPccXADD, NDD-shaped)
    'r16, rax, rbx', 'rax, r16, rbx', 'rax, rbx, r16',
    'r16d, eax, ebx', 'eax, r16d, ebx', 'eax, ebx, r16d',
    'r16, [rax], rbx', 'rax, [r16], rbx', 'rax, rbx, [r16]', '[r16], rax, rbx',
    # memory: base, index, both sizes, with register and immediate
    '[r16]', 'qword [r16]', 'dword [r16]', 'word [r16]', 'byte [r16]', 'tword [r16]',
    '[rax+r16]', '[r16+rax*2]', '[rax+r16*8+8]', '[r16d]', '[eax+r16d]', '[r16w]',
    'rax, [r16]', '[r16], rax', 'eax, [r16]', '[r16], eax', 'ax, [r16]',
    'al, [r16]', '[r16], al', '[r16], 1', 'byte [r16], 1', 'qword [r16], 1',
    'r16, [rax]', '[rax], r16', 'r16d, [rax]', '[rax], r16d',
    'byte [r16], byte [rax]', 'byte [rax], byte [r16]', 'far [r16]', 'fs:[r16]',
    # system registers
    'r16, cr0', 'cr0, r16', 'r16, dr0', 'dr0, r16', 'es, r16w', 'r16w, es',
    # MMX/SSE/AVX with GPR or address operands
    'mm0, [r16]', '[r16], mm0', 'mm0, r16d', 'r16d, mm0', 'mm0, r16d, 1', 'r16d, mm0, 1',
    'xmm0, [r16]', '[r16], xmm0', 'ymm0, [r16]', '[r16], ymm0', 'zmm0, [r16]', '[r16], zmm0',
    'xmm0, xmm1, [r16]', 'ymm0, ymm1, [r16]', 'zmm0, zmm1, [r16]',
    'xmm0, [r16], 1', 'xmm0, xmm1, [r16], 1', 'xmm0, xmm1, [r16], xmm2',
    'xmm0, r16d', 'r16d, xmm0', 'xmm0, r16', 'r16, xmm0', 'xmm0, xmm1, r16d', 'xmm0, xmm1, r16',
    'r16d, xmm0, 1', 'xmm0, r16d, 1', 'xmm0, xmm1, r16d, 1', '[r16], xmm0, 1',
    'xmm0, [r16]{1to4}', 'zmm0, zmm1, [r16]{1to16}', 'zmm0{k1}, [r16]',
    'k1, r16d', 'r16d, k1', 'k1, r16', 'r16, k1', 'k1, [r16]', '[r16], k1',
    # VSIB, MIB and AMX
    'zmm0{k1}, [r16+zmm1]', '[r16+zmm1]{k1}, zmm0', '[r16+zmm1]{k1}',
    'xmm0, [r16+xmm1], xmm2', 'ymm0, [r16+ymm1], ymm2',
    'bnd0, [r16]', 'bnd0, [r16,rax]', '[r16,rax], bnd0', 'bnd0, r16',
    'tmm0, [r16,rax]', 'tmm0, [rax+r16]', '[r16,rax], tmm0',
    # FPU memory forms
    'st0, [r16]', 'qword [r16], st0',
]
PATTERNS = list(dict.fromkeys(PATTERNS))
EDGE = ['r20', 'r21', 'r24', 'r28', 'r29', 'r31']

# (mnemonic regex, pattern regex, reason); pass-2 forms match by their pass-1
# pattern. Each was checked to behave the same with r8 in place of r16, i.e.
# it is not an APX encoding question.
REVIEWED = [
    (r'call|jmp', r'(tword|far) \[.*', 'far m16:64 needs REX.W (SDM); llvm reads "tbyte ptr" without W, "far" as a symbol'),
    (r'mov', r'(es|cs|ss|ds|fs|gs), r16w|r16w, (es|cs|ss|ds|fs|gs)', 'mov sreg: fasm2 and llvm-mc differ on the optional 66h'),
    (r'fbld|fbstp', r'\w+ \[.*', 'fasm2 accepts any size for m80bcd; llvm-mc wants tbyte'),
    (r'invlpg', r'(qword|dword|word|byte|tword) \[.*', 'fasm2 accepts any size for m; llvm-mc wants byte'),
    (r'clrssbsy|rstorssp', r'qword \[.*', 'llvm-mc wants the m64 unsized'),
    (r'nop', r'\[.*', 'fasm2 defaults nop m to dword; llvm-mc wants a size'),
    (r'movsxd', r'.*', 'fasm2 allows movsxd with a 16/32-bit destination; llvm-mc does not'),
    (r'ud0', r'.*', 'llvm-mc does not take this ud0 operand form'),
    (r'f(n?(stenv|save)|ldenv|rstor)w', r'.*', 'fasm2-only 16-bit environment spellings (66h)'),
    (r'bnd(cl|cu|cn|mk|ldx|stx|mov)', r'.*', 'MPX: llvm-mc no longer has it'),
    (r'tile\w+', r'.*\[r16,rax\].*', "fasm2's AMX [base, index] syntax, which llvm-mc lacks; bytes equal [base+index] (apx_vex.asm)"),
    (r'v(gather|scatter)pf[01][dq]p[sd]', r'\[r16\+zmm1\]\{k1\}', 'AVX512PF: llvm-mc wants a size on the memory operand; bytes checked with one in apx_vector.asm'),
]

# not instructions: settings, prefixes that take an instruction, directives
SKIP = re.compile(r'^(use\w*|require\w*|xacquire|xrelease)$')


def names():
    out = set(k for k in defs if re.fullmatch(r'[a-z_][a-z0-9_]*', k))
    if os.path.exists(ZYDIS_DB):
        for rec in json.load(open(ZYDIS_DB, encoding='utf-8')):
            m = rec.get('mnemonic', '').lower()
            if re.fullmatch(r'[a-z_][a-z0-9_]*', m):
                out.add(m)
    return sorted(n for n in out if not SKIP.match(n))


def main():
    verbose = '-v' in sys.argv
    candidates = names()
    defined = []
    # a bare mnemonic fasm2 does not define is 'illegal instruction'; any
    # defined one either assembles or fails for its operands
    fails = _apx_diff.fasm_errors(candidates)
    for i, n in enumerate(candidates):
        if 'illegal instruction' not in fails.get(i, ''):
            defined.append(n)
    forms = [(n, p) for n in defined for p in PATTERNS]
    lines = ['%s %s' % f for f in forms]
    result = _apx_diff.compare(lines)
    edge_forms, edge_lines = [], []
    for (n, p), (st, fb, lb) in zip(forms, result):
        if st in ('same', 'DIFF', 'FASM'):
            for r in EDGE:
                edge_forms.append((n, p))
                edge_lines.append('%s %s' % (n, re.sub(r'\br16', r, p)))
    edge_result = _apx_diff.compare(edge_lines)

    counts = collections.Counter()
    failures, llvm_only = [], collections.defaultdict(list)
    for (n, p), line, (st, fb, lb) in list(zip(forms, lines, result)) + list(zip(edge_forms, edge_lines, edge_result)):
        counts[st] += 1
        if st in ('DIFF', 'FASM'):
            why = [r for m, q, r in REVIEWED if re.fullmatch(m, n) and re.fullmatch(q, p)]
            if why:
                counts['reviewed'] += 1
            else:
                failures.append((st, line, fb, lb))
        elif st == 'llvm':
            llvm_only[n].append(p)
    print('%d names, %d defined by fasm2; %d forms, %d with edge registers'
          % (len(candidates), len(defined), len(lines), len(edge_lines)))
    print('  same %(same)d, DIFF %(DIFF)d, FASM only %(FASM)d, llvm only %(llvm)d, both refuse %(-)d' % counts)
    print('  of DIFF/FASM, %d match a REVIEWED entry (%d entries)' % (counts['reviewed'], len(REVIEWED)))
    if verbose:
        for n in sorted(llvm_only):
            print('llvm only  %-14s %s' % (n, ' | '.join(llvm_only[n])))
    for st, line, fb, lb in failures:
        print('%-4s  %-40s fasm %-30s llvm %s' % (st, line, fb.hex(' '), lb.hex(' ')))
    print('%d failure(s)' % len(failures))
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
