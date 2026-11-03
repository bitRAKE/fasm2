"""_internal_errors.py -- macro bugs: an undefined symbol the source line does not name.

    python tests/iset/_internal_errors.py

Every mnemonic is assembled with the sweep patterns (r16 written as rax and
r8 names of the same size) and a few immediate forms. An error naming a
symbol the line does not contain (supp, opcode, @dest.dword ...) comes from
the macro, never from the user: G26 and G27 were two. Exit code 1 if any.
"""
import sys, os, re, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import _apx_diff, _apx_sweep

UNDEFINED = re.compile(r"symbol '([^']+)' is undefined or out of scope")
# r16 in the sweep patterns, by size suffix, as rax and r8 names
NAMES = {'rax': {'': 'rax', 'd': 'eax', 'w': 'ax', 'b': 'al', 'l': 'al'},
         'r8': {'': 'r8', 'd': 'r8d', 'w': 'r8w', 'b': 'r8b', 'l': 'r8l'}}


def internal(line, errors):
    """An undefined symbol the line does not name: the macro's own (supp,
    opcode, @dest.dword ...), never the user's."""
    words = set(re.findall(r'[\w.@]+', line.lower()))
    for sym in UNDEFINED.findall(errors):
        parts = sym.lower().split('.')
        if not any(p in words for p in parts) and sym.lower() not in words:
            return sym
    return None


def main():
    names = _apx_sweep.names()
    fails = _apx_diff.fasm_errors(names)
    defined = [n for i, n in enumerate(names) if 'illegal instruction' not in fails.get(i, '')]
    pats = set()
    for p in _apx_sweep.PATTERNS + ['xmm0, xmm1, 1', 'xmm0, xmm1, xmm2, 1', 'ymm0, ymm1, ymm2, 1',
                                    'zmm0, zmm1, zmm2, 1', 'xmm0, [rax], 1', 'eax, xmm0, 1']:
        for r in ('rax', 'r8'):
            pats.add(re.sub(r'\br16([dwbl]?)\b', lambda m: NAMES[r][m.group(1)], p))
    lines = ['%s %s' % (n, p) for n in defined for p in sorted(pats)]
    # one source of mostly failing lines is slow in fasmg: chunks, in parallel
    from concurrent.futures import ThreadPoolExecutor
    chunk = 4000
    starts = list(range(0, len(lines), chunk))
    errs = {}
    with ThreadPoolExecutor(8) as pool:
        for start, part in zip(starts, pool.map(lambda s: _apx_diff.fasm_errors(lines[s:s + chunk]), starts)):
            errs.update({start + k: v for k, v in part.items()})
    hits = collections.defaultdict(list)
    for i, l in enumerate(lines):
        sym = internal(l, errs.get(i, ''))
        if sym:
            hits[(l.split()[0], sym)].append(l)
    for (n, sym), ls in sorted(hits.items()):
        print('%-18s %-24s %4d  e.g. %s' % (n, sym, len(ls), ls[0]))
    print('%d mnemonics, %d forms; %d mnemonic/symbol groups with a macro-internal undefined symbol'
          % (len(defined), len(lines), len(hits)))
    sys.exit(1 if hits else 0)


if __name__ == '__main__':
    main()
