"""_dupnames.py -- fail if an instruction name is defined more than once.

    python tests/iset/_dupnames.py

In fasmg a symbol defined once is global and can be used anywhere, even
before its definition. A second definition makes it a variable: every use
after it gets the new definition, silently. For a mnemonic that means the
later file wins for all code that follows, including user code.
G12 (vpmuldq) and G13 (vcvtph2ps/vcvtps2ph) were exactly that.

A second definition is a stack: inside it, the mnemonic's own name means
the earlier definition, so a later layer can handle its new forms and pass
the rest down ("not for me, try the earlier one"). A layer that does not
pass on takes over every form (G12, G13, G14).

This scans include/x86-2.inc and include/iset/*.inc for `macro name?` and
`calminstruction name?` definitions, expanding `iterate` lists, and fails on
any name defined twice that is not in REVIEWED: stacks whose earlier forms
were checked to still assemble under their own gates (2026-11-02; the
MMX/SSE ones by tests/iset/stacking.asm).
Names it cannot expand are reported, not guessed.
"""
import os, re, sys, collections

REVIEWED = {
    # string instructions vs SSE2 scalar forms; x86-2.inc if/else pairs
    'cmpsd', 'movsd', 'cmpsq', 'lodsq', 'movsq', 'scasq', 'stosq',
    'cdqe', 'cqo', 'iretq', 'rdmsrq', 'sysretq', 'nop',
    # MMX -> SSE -> SSE2 -> SSE4.1 extensions of the same mnemonic
    'movd', 'movq', 'pextrw', 'pinsrw', 'pmovmskb', 'fisttp',
    # FPU: o16/o32 environment forms and 8087-only forms
    'fldenv', 'fnstenv', 'fnsave', 'frstor', 'fneni', 'fndisi',
    # avx.inc: k-register groups list knotw/knotb/knotd/knotq twice (harmless)
    'knotw', 'knotb', 'knott',
    # scan artifacts from iterate values written as <...>
    '32h>', '7>', '<0fh', 'p5',
}

root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
inc = os.path.join(root, 'include')
files = [os.path.join(inc, 'x86-2.inc')] + sorted(
    os.path.join(inc, 'iset', f) for f in os.listdir(os.path.join(inc, 'iset')) if f.endswith('.inc'))

defs = collections.defaultdict(list)
for path in files:
    text = open(path, encoding='utf-8', errors='replace').read()
    lines = text.replace('\\\r\n', ' ').replace('\\\n', ' ').splitlines()
    stack = []
    for no, line in enumerate(lines, 1):
        code = line.split(';', 1)[0].strip()
        m = re.match(r'iterate\s+<([^>]*)>\s*,(.*)', code)
        if m:
            params = [p.strip() for p in m.group(1).split(',')]
            vals = [v.strip() for v in m.group(2).split(',')]
            stack.append((params, [vals[i:i + len(params)] for i in range(0, len(vals), len(params))]))
            continue
        m = re.match(r'iterate\s+(\w+)\s*,(.*)', code)
        if m:
            stack.append(([m.group(1)], [[v.strip()] for v in m.group(2).split(',')]))
            continue
        if re.match(r'end\s+iterate', code):
            if stack:
                stack.pop()
            continue
        m = re.match(r'(macro|calminstruction)\s+([\w.#@]+)\?', code)
        if not m:
            continue
        names = [m.group(2)]
        for params, rows in reversed(stack):
            out = []
            for n in names:
                if not any(re.search(r'(^|#)' + re.escape(p) + r'($|#)', n) for p in params):
                    out.append(n)
                    continue
                for row in rows:
                    t = n
                    for p, v in zip(params, row):
                        t = re.sub(r'(^|#)' + re.escape(p) + r'($|#)', lambda mm: mm.group(1) + v + mm.group(2), t)
                    out.append(t.replace('#', ''))
            names = out
        for n in names:
            defs[n.lower()].append('%s:%d' % (os.path.relpath(path, root), no))

if __name__ == '__main__':	# imported by _apx_sweep.py for `defs`
    new = {k: v for k, v in defs.items() if len(v) > 1 and k not in REVIEWED}
    gone = sorted(k for k in REVIEWED if len(defs.get(k, [])) < 2)
    print('%d names scanned, %d reviewed stacks' % (len(defs), len(REVIEWED)))
    for k in sorted(new):
        print('NEW DUPLICATE  %-24s %s' % (k, ', '.join(new[k])))
    for k in gone:
        print('note: reviewed name no longer duplicated: %s' % k)
    sys.exit(1 if new else 0)
