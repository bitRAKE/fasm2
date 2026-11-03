"""_imm_probe.py -- an immediate operand replaced by memory must be refused (G15 class).

    python tests/iset/_imm_probe.py


Each mnemonic x pattern ending in an immediate is assembled as a valid
line (immediate 3) directly followed by the same line with the immediate
replaced by [rbx], in one source, so stale operand state from the first
line is visible to the second.
"""
import sys, os, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import _apx_diff, _apx_sweep

PATTERNS = ['xmm0, xmm1, 3', 'xmm0, [rax], 3', 'xmm0, xmm1, xmm2, 3', 'xmm0, xmm1, [rax], 3',
            'ymm0, ymm1, 3', 'ymm0, ymm1, ymm2, 3', 'zmm0, zmm1, 3', 'zmm0, zmm1, zmm2, 3',
            'xmm0, ymm1, 3', '[rax], ymm1, 3', 'xmm0, zmm1, 3', 'ymm0, zmm1, 3', 'ymm0, ymm1, xmm2, 3',
            'zmm0, zmm1, xmm2, 3', 'zmm0, zmm1, ymm2, 3', 'eax, xmm1, 3', '[rax], xmm1, 3',
            'xmm0, xmm1, eax, 3', 'xmm0, eax, 3', 'k1, xmm1, xmm2, 3', 'k1, zmm1, zmm2, 3', 'k1, xmm1, 3',
            'k1, k2, 3', 'mm0, mm1, 3', 'mm0, [rax], 3', 'eax, mm1, 3', 'mm0, eax, 3',
            'eax, ebx, 3', 'rax, rbx, 3', 'eax, [rbx], 3', 'eax, 3', 'rax, 3', '[rax], 3', 'dword [rax], 3',
            'xmm0{k1}, xmm1, xmm2, 3', 'zmm0{k1}, zmm1, zmm2, 3', 'zmm0{k1}, zmm1, 3']


def main():
    names = _apx_sweep.names()
    fails = _apx_diff.fasm_errors(names)
    defined = [n for i, n in enumerate(names) if 'illegal instruction' not in fails.get(i, '')]
    lines, pairs = [], []
    for n in defined:
        for p in PATTERNS:
            good = '%s %s' % (n, p)
            bad = '%s %s' % (n, re.sub(r'3$', '[rbx]', p))
            pairs.append((len(lines), good, bad))
            lines += [good, bad]
    got = _apx_diff.fasm(lines)
    hits = []
    for i, good, bad in pairs:
        (ok1, b1), (ok2, b2) = got[i], got[i + 1]
        if ok1 and ok2:
            hits.append((bad, b1.hex(' '), b2.hex(' ')))
    # a real form also assembles alone, with the same bytes; stale state does not
    alone = _apx_diff.fasm_errors([bad for bad, b1, b2 in hits])
    alone_bytes = [_apx_diff.fasm([bad])[0] for bad, b1, b2 in hits]
    stale = 0
    for k, (bad, b1, b2) in enumerate(hits):
        ok, b = alone_bytes[k]
        if ok and b.hex(' ') == b2:
            continue
        stale += 1
        print('%-40s after valid: %-28s alone: %s' % (bad, b2, b.hex(' ') if ok else alone.get(k, '').strip()[:50]))
    print('%d mnemonics, %d pairs; %d accept memory as the immediate, %d of them only after another line (stale state)'
          % (len(defined), len(pairs), len(hits), stale))


if __name__ == '__main__':
    main()
