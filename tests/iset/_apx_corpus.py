"""_apx_corpus.py -- assemble LLVM's APX Intel-syntax tests with fasm2.

    python tests/iset/_apx_corpus.py [-v] [name ...]

LLVM's llvm/test/MC/X86/apx/*-intel.s (largely written by Intel engineers;
encoding authority 2 in include/iset/apx-plan.md) pair each instruction
with its encoding:
    # CHECK: ccmpo {dfv=of} rbx, rax
    # CHECK: encoding: [0x62,0xf4,0xc4,0x00,0x39,0xc3]
             ccmpo {dfv=of} rbx, rax
Each source line is translated to fasm2 syntax (see fasm_syntax), assembled
(_apx_diff.fasm) and compared with the encoding. Per file it prints:
    same      fasm2 writes the expected bytes
    DIFF      fasm2 writes other bytes                  (failure)
    refused   fasm2 refuses the line: not implemented yet, or a form fasm2
              rejects on purpose (listed with -v)
Names select files (ccmp, cfcmov, ...); without names, all 63 files.
Exit code: the number of DIFF lines not in REVIEWED.
"""
import os, re, sys, glob, collections

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import _apx_diff	# noqa: E402

CORPUS = r'C:\git\llvm-project\llvm\test\MC\X86\apx'
SIZES = {'xmmword': 'xword', 'ymmword': 'yword', 'zmmword': 'zword', 'tbyte': 'tword'}

# (regex on the translated line, reason): DIFFs where fasm2 is right, or
# chose a different valid encoding
REVIEWED = [
    (r'lea\s+\w+, \[1\*r\d+\]', 'fasm2 reads [1*reg] as a base register: same address, shorter; '
     'the test means index-only (X4); X4 is covered by apx_rex2.asm and the sweep'),
]


def fasm_syntax(line):
    line = re.sub(r'\b(\w+)\s+ptr\b', lambda m: SIZES.get(m.group(1), m.group(1)), line)
    # options after the mnemonic in fasm2: {nf} add ... -> add {nf} ...
    line = re.sub(r'^\{(nf)\}\s+(\w+)', r'\2 {\1}', line)
    # ZU as an option: setzuCC -> setCC {zu}, imulzu -> imul {zu}
    line = re.sub(r'^setzu(\w+)', r'set\1 {zu}', line)
    line = re.sub(r'^(\w+\s+\{nf\}\s+)?imulzu\b', lambda m: 'imul {zu}' if not m.group(1) else m.group(1).replace('imul', 'imul {zu}'), line)
    line = re.sub(r'^imulzu\s+\{nf\}', 'imul {zu} {nf}', line)
    return line


def harvest(path):
    """[(fasm line, expected bytes)]"""
    out, pending = [], None
    for raw in open(path, encoding='utf-8', errors='replace'):
        m = re.match(r'#\s*CHECK:\s*encoding:\s*\[([^\]]*)\]', raw)
        if m:
            pending = bytes(int(x, 16) for x in m.group(1).split(',') if x.strip())
            continue
        if raw.lstrip().startswith('#') or not raw.strip():
            continue
        if pending is not None:
            out.append((fasm_syntax(raw.strip()), pending))
            pending = None
    return out


def main():
    verbose = '-v' in sys.argv
    names = [a for a in sys.argv[1:] if not a.startswith('-')]
    files = sorted(glob.glob(os.path.join(CORPUS, '*-intel.s')))
    if names:
        files = [f for f in files if os.path.basename(f)[:-len('-intel.s')] in names]
    total = collections.Counter()
    failures = 0
    for f in files:
        cases = harvest(f)
        if not cases:
            continue
        got = _apx_diff.fasm([c[0] for c in cases])
        counts = collections.Counter()
        report = []
        for (line, want), (ok, fb) in zip(cases, got):
            if not ok:
                counts['refused'] += 1
                if verbose:
                    report.append('  refused  %s' % line)
            elif fb == want:
                counts['same'] += 1
            elif any(re.fullmatch(r, line) for r, why in REVIEWED):
                counts['reviewed'] += 1
            else:
                counts['DIFF'] += 1
                failures += 1
                report.append('  DIFF     %-44s fasm %s  llvm %s' % (line, fb.hex(' '), want.hex(' ')))
        total.update(counts)
        print('%-24s same %4d  DIFF %3d  refused %4d%s' % (os.path.basename(f), counts['same'], counts['DIFF'],
              counts['refused'], '  reviewed %d' % counts['reviewed'] if counts['reviewed'] else ''))
        for r in report:
            print(r)
    print('total: same %d, DIFF %d, refused %d, reviewed %d' % (total['same'], total['DIFF'], total['refused'], total['reviewed']))
    sys.exit(min(failures, 255))


if __name__ == '__main__':
    main()
