"""x86db_cover.py -- XED's mnemonics against fasm2's.

    python tools/x86db/x86db_cover.py [build/xed.json] [-v]

XED names instruction classes, not mnemonics: ADD_LOCK, REP_MOVSB,
REPE_CMPSB, XED-only spellings. They are mapped to Intel mnemonics (the
_LOCK/REP prefixes dropped, lowercased) and compared with every name
fasm2 defines (tests/iset/_dupnames.py's scan). Prints the counts and,
with -v, both difference lists by XED extension.
"""
import os, re, sys, json, collections

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, '..', '..', 'tests', 'iset'))
from _dupnames import defs	# noqa: E402
import _apx_diff	# noqa: E402


def mnemonic(iclass):
    m = iclass.lower()
    m = re.sub(r'_lock$', '', m)
    m = re.sub(r'^(rep|repe|repne)_', '', m)
    return m


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    verbose = '-v' in sys.argv
    recs = json.load(open(args[0] if args else os.path.join(here, 'build', 'xed.json')))
    ext = collections.defaultdict(set)
    for r in recs:
        ext[mnemonic(r['iclass'])].add(r['extension'])
    xed = set(ext)
    fasm = {k for k in defs if re.fullmatch(r'[a-z][a-z0-9_]*', k)}
    # names built with # in an iterate escape the scan: ask fasm2 itself
    probe = sorted(xed - fasm)
    fails = _apx_diff.fasm_errors(probe)
    fasm |= {n for i, n in enumerate(probe) if 'illegal instruction' not in fails.get(i, '')}
    both = xed & fasm
    print('XED mnemonics %d, fasm2 names %d, both %d, XED only %d, fasm2 only %d'
          % (len(xed), len(fasm), len(both), len(xed - fasm), len(fasm - xed)))
    by = collections.defaultdict(list)
    for m in sorted(xed - fasm):
        by['/'.join(sorted(ext[m]))].append(m)
    print('XED only, by extension:')
    for e, ms in sorted(by.items(), key=lambda kv: -len(kv[1])):
        print('  %-28s %4d  %s' % (e, len(ms), ' '.join(ms[:12]) + (' ...' if len(ms) > 12 and not verbose else ' '.join(ms[12:]) if verbose else '')))
    if verbose:
        print('fasm2 only:')
        print('  ' + ' '.join(sorted(fasm - xed)))


if __name__ == '__main__':
    main()
