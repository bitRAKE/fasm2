"""_apx_xed.py -- XED must decode every APX form fasm2 assembles.

    python tests/iset/_apx_xed.py [-v]

The other APX tools compare fasm2 with LLVM, and LLVM's assembler encodes
some forms the processor refuses (#UD), for example `push2 rsp, rax` or
`{rex2} movbe`. XED is Intel's own decoder, independent of LLVM, and it
refuses those bytes (BAD_REGISTER, BAD_REG_MATCH, GENERAL_ERROR,
BAD_LOCK_PREFIX, ...). fasm2 must not emit an instruction that raises #UD
unless it is asked to, so every form fasm2 accepts must decode.

Forms: every mnemonic with every _apx_sweep.py pattern, with r16 and with
each EDGE register, and with r8 (the same form without APX, so an error
there is not an APX question), plus every line of LLVM's APX corpus
(_apx_corpus.py). Each line fasm2 assembles is decoded by _xedcheck.exe
(see _xedcheck.c) instruction after instruction.

Failures: XED refuses bytes fasm2 wrote, unless REVIEWED gives the reason.
-v also prints XED's Intel-syntax text for each failure's mnemonic.
"""
import os, re, sys, glob, subprocess, collections

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import _apx_diff, _apx_sweep, _apx_corpus	# noqa: E402

XEDCHECK = r'C:\git\_Intel\xed\obj-wkit\wkit\bin\xedcheck.exe'

# (mnemonic regex, line regex, reason)
REVIEWED = [
]


def xed(blobs):
    p = subprocess.run([XEDCHECK], input=''.join(b.hex() + '\n' for b in blobs),
                       capture_output=True, text=True)
    out = p.stdout.splitlines()
    assert len(out) == len(blobs), p.stderr
    return out


def forms():
    names = _apx_sweep.names()
    fails = _apx_diff.fasm_errors(names)
    defined = [n for i, n in enumerate(names) if 'illegal instruction' not in fails.get(i, '')]
    lines = []
    for n in defined:
        for p in _apx_sweep.PATTERNS:
            for r in ['r16', 'r8'] + _apx_sweep.EDGE:
                lines.append((n, '%s %s' % (n, re.sub(r'\br16', r, p))))
    for f in sorted(glob.glob(os.path.join(_apx_corpus.CORPUS, '*-intel.s'))):
        for line, want in _apx_corpus.harvest(f):
            lines.append((line.split()[0], line))
    return list(dict.fromkeys(lines))


def main():
    verbose = '-v' in sys.argv
    todo = forms()
    got = _apx_diff.fasm([l for n, l in todo])
    accepted = [(n, l, b) for (n, l), (ok, b) in zip(todo, got) if ok and b]
    result = xed([b for n, l, b in accepted])
    counts = collections.Counter()
    failures = collections.defaultdict(list)
    for (n, l, b), r in zip(accepted, result):
        if r.startswith('OK'):
            counts['ok'] += 1
        elif any(re.fullmatch(m, n) and re.fullmatch(q, l) for m, q, why in REVIEWED):
            counts['reviewed'] += 1
        else:
            counts['ERR'] += 1
            failures[(n, r.split()[1])].append((l, b, r))
    print('%d forms, %d assembled by fasm2: XED decodes %d, refuses %d, reviewed %d'
          % (len(todo), len(accepted), counts['ok'], counts['ERR'], counts['reviewed']))
    for (n, err), cases in sorted(failures.items()):
        print('%-14s %-16s %5d  e.g. %-36s %s' % (n, err, len(cases), cases[0][0], cases[0][1].hex(' ')))
        if verbose:
            for l, b, r in cases:
                print('      %-40s %-30s %s' % (l, b.hex(' '), r))
    print('%d failure(s) in %d groups' % (counts['ERR'], len(failures)))
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
