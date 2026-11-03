"""_apx_gas.py -- compare fasm2's APX forms with GNU as (binutils, in WSL).

    python tests/iset/_apx_gas.py [-v] [-x] [FILE]

GNU as is the independent second encoder (authority 4 in
include/iset/apx-plan.md): its x86 encoder shares no code with LLVM's.
The build is the latest binutils release under ~/opt/binutils-latest in
WSL (GAS_PREFIX overrides it); the distro's 2.42 lacks {nf}, ccmp,
cfcmov, setzu and jmpabs.

Forms: every mnemonic fasm2 defines with every _apx_sweep.py pattern,
again with each EDGE register for every form either side accepts, plus
every line of LLVM's APX corpus (_apx_corpus.harvest). With FILE, the
lines of FILE instead (one instruction per line, like _apx_diff.py).

Each line becomes `L<n>: <line>` in Intel syntax (.intel_syntax noprefix,
the operand spelling of _apx_diff.to_llvm), assembled with `as --64`.
Lines with errors are found in a first run and left out of a second; the
bytes of each line are the .text bytes from its label to the next (nm,
objcopy -O binary). Warnings do not refuse a line (-v lists them).

Per form:
    same   both assemble, same bytes
    DIFF   both assemble, different bytes
    FASM   only fasm2 assembles it
    gas    only GNU as assembles it
DIFF and FASM lines match REVIEWED (with the reason) or are printed.
-x decodes both sides of each printed DIFF with XED (_apx_xed.xed), which
outranks GNU as and LLVM, and every gas-only form: bytes XED refuses are
#UD that GNU as writes (listed; not a failure of fasm2). Exit code 1 when
an unreviewed DIFF or FASM form is printed.
"""
import os, re, sys, glob, subprocess, collections

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import _apx_diff, _apx_sweep, _apx_corpus	# noqa: E402

GAS_PREFIX = os.environ.get('GAS_PREFIX', '~/opt/binutils-latest')

# runs in WSL from the Linux home (WSL cannot chdir into this worktree)
SCRIPT = r'''cd ~ || exit 9
d=$(mktemp -d) || exit 9
trap 'rm -rf "$d"' EXIT
cat > "$d/a.s"
P=%s
"$P/bin/as" --64 -o "$d/a.o" "$d/a.s" 2> "$d/err"
sed 's/^/M /' "$d/err"
[ -f "$d/a.o" ] || exit 0
"$P/bin/nm" "$d/a.o" | sed 's/^/S /'
"$P/bin/objcopy" -O binary -j .text "$d/a.o" "$d/a.bin"
echo "X $(od -An -v -tx1 "$d/a.bin" | tr -d ' \n')"
''' % GAS_PREFIX


def version():
    p = subprocess.run(['wsl', '-e', 'bash', '-c', 'cd ~; %s/bin/as --version | head -1' % GAS_PREFIX],
                       capture_output=True, text=True)
    return p.stdout.strip()


def _run(lines):
    src = '.intel_syntax noprefix\n' + ''.join('L%d: %s\n' % (i, l) for i, l in enumerate(lines))
    p = subprocess.run(['wsl', '-e', 'bash', '-c', SCRIPT], input=src.encode(), capture_output=True)
    out = p.stdout.decode('utf-8', 'replace')
    errors, warnings, syms, blob = {}, {}, {}, b''
    for line in out.splitlines():
        if line.startswith('M '):
            m = re.match(r'M .*?:(\d+): (Error|Warning): (.*)', line)
            if m:
                i = int(m.group(1)) - 2
                (errors if m.group(2) == 'Error' else warnings).setdefault(i, []).append(m.group(3))
        elif line.startswith('S '):
            m = re.match(r'S ([0-9a-f]+) \w L(\d+)$', line)
            if m:
                syms[int(m.group(2))] = int(m.group(1), 16)
        elif line.startswith('X '):
            blob = bytes.fromhex(line[2:].strip())
    return errors, warnings, syms, blob


def gas(lines, chunk=40000):
    """[(ok, bytes, messages)] for each line (messages: errors or warnings)."""
    out = []
    for start in range(0, len(lines), chunk):
        part = [_apx_diff.to_llvm(l) for l in lines[start:start + chunk]]
        errors, _, _, _ = _run(part)
        good = ['' if i in errors else l for i, l in enumerate(part)]
        errors2, warnings, syms, blob = _run(good)
        assert not errors2, sorted(errors2.items())[:5]
        addr = [syms[i] for i in range(len(part))] + [len(blob)]
        for i in range(len(part)):
            if i in errors:
                out.append((False, b'', errors[i]))
            else:
                b = blob[addr[i]:addr[i + 1]]
                out.append((bool(b), b, warnings.get(i, [])))
    return out


# (mnemonic regex, line regex, reason): DIFF/FASM forms decided in favour
# of fasm2 or as an equally valid encoding (XED decodes both the same).
# The mnemonic is the line's first word ({evex} for an annotated line).
# A reason that starts with 'open' is a known defect not fixed yet: it is
# counted apart and listed (apx-plan.md 5.3).
R = r'\s*[\w]+\s*'	# a register operand
REVIEWED = [
    # equally valid encodings
    (r'cfcmov\w+', r'cfcmov\w+%s,%s' % (R, R), 'register form: NF=1 reg <- r/m (fasm2, llvm-mc) or NF=0 r/m <- reg (gas); XED: same'),
    (r'movbe|\{evex\}', r'(\{evex\}\s*)?movbe%s,%s' % (R, R), 'EVEX register form: 61 r/m <- reg (fasm2, llvm-mc) or 60 reg <- r/m (gas); XED: same'),
    (r'vmovq|\{evex\}', r'(\{evex\}\s*)?vmovq\s+(xmm\d+, \[.*|\[.*\], xmm\d+)', 'EVEX m64: F3 0F 7E / 66 0F D6 (fasm2, llvm-mc) or 66 0F 6E / 7E with W1 (gas); XED: same'),
    (r'xchg', r'xchg%s,%s' % (R, R), 'xchg is symmetric: either register in ModRM.reg'),
    (r'f(n?(stenv|save)|ldenv|rstor)w', r'.*', '66h and 67h in either order (fasm2 66 67, gas 67 66)'),
    (r'lar|lsl|sldt|str', r'.*', 'gas drops REX.W from a 64-bit destination (also with r8, rax: not APX); the 32-bit result is zero-extended, so the same; fasm2 writes the size asked for, as llvm-mc'),
    # spellings GNU as does not take, or takes differently
    (r'call|jmp', r'(call|jmp) (tword|far) \[.*', 'far m16:64: gas reads "far" as a symbol and wants no tbyte (also in _apx_sweep REVIEWED)'),
    (r'nop|movsx|movzx|lfs|lgs|lss', r'.*', 'fasm2 sizes the memory operand (nop: dword, movsx/movzx: byte, lfs/lgs/lss: m16:64); gas wants it written'),
    (r'fbld|fbstp|f(n?(stenv|save)|ldenv|rstor)[wd]?', r'.*', 'fasm2 accepts any size keyword on these memory operands; gas wants tbyte or none'),
    (r'tile(loadd|loaddt1|stored)', r'.*\[r\d+,rax\].*', "fasm2's AMX [base, index] syntax, which gas lacks; bytes equal [base+index] (apx_vex.asm)"),
    (r'enqcmds?|movdir64b|\{evex\}', r'(\{evex\}\s*)?(enqcmds?|movdir64b)\s.*zword.*', 'gas wants the m512 unsized'),
    (r'imul', r'imul \{zu\}\s*(e|r)\w+,.*', 'gas has imulzu for 16-bit operands only; XED decodes the 32/64-bit forms (LLVM corpus)'),
    (r'set\w+', r'set\w+ \{zu\}\s*byte \[.*', 'gas has no setzu<cc> m8; XED decodes it (LLVM corpus)'),
    # fasm2 defects already on record (apx-plan.md 5.3)
    (r'v(roundp[sd]|pcmp[ei]str[im]|cvtps2ph)', r'.*, \[r\d+\]', 'open G15: imm8 operand not checked (EXTENSIONS.md 2.4)'),
    (r'cvtt?ss2si', r'.*mm0', 'open G16b: fasm2 takes an mm source (EXTENSIONS.md 2.4)'),
    (r'lea', r'lea\s+\w+, \[1\*r\d+\]', 'fasm2 reads [1*reg] as a base register: same address, shorter (also in _apx_corpus REVIEWED)'),
    (r'urdmsr|\{evex\}', r'(\{evex\}\s*)?urdmsr%s,%s' % (R, R), 'open G25: urdmsr r64, r64 has the destination in ModRM.rm (SDM 093 Op/En MR, XED, gas); fasm2 and llvm-mc put it in ModRM.reg'),
]


def forms():
    """[(mnemonic, pattern or None, line)] of pass 1 and the corpus"""
    names = _apx_sweep.names()
    fails = _apx_diff.fasm_errors(names)
    defined = [n for i, n in enumerate(names) if 'illegal instruction' not in fails.get(i, '')]
    out = [(n, p, '%s %s' % (n, p)) for n in defined for p in _apx_sweep.PATTERNS]
    for f in sorted(glob.glob(os.path.join(_apx_corpus.CORPUS, '*-intel.s'))):
        for line, want in _apx_corpus.harvest(f):
            out.append((line.split()[0], None, line))
    return out


def compare(lines):
    result = []
    for (fo, fb), (go, gb, msg) in zip(_apx_diff.fasm(lines), gas(lines)):
        st = ('same' if fb == gb else 'DIFF') if fo and go else 'FASM' if fo else 'gas' if go else '-'
        result.append((st, fb, gb, msg))
    return result


def main():
    verbose = '-v' in sys.argv
    use_xed = '-x' in sys.argv
    files = [a for a in sys.argv[1:] if not a.startswith('-')]
    print(version())
    if files:
        todo = [(l.split()[0], None, l) for l in (r.strip() for r in open(files[0])) if l]
    else:
        todo = list({t[2]: t for t in forms()}.values())
    result = compare([t[2] for t in todo])
    if not files:
        edge = []
        for (n, p, l), (st, fb, gb, msg) in zip(todo, result):
            if p is not None and st != '-':
                edge += [(n, p, '%s %s' % (n, re.sub(r'\br16', r, p))) for r in _apx_sweep.EDGE]
        edge = [t for t in dict.fromkeys(edge) if t[2] not in {x[2] for x in todo}]
        todo += edge
        result += compare([t[2] for t in edge])
    counts = collections.Counter()
    report, gas_only, warned = [], collections.defaultdict(list), []
    open_items = collections.defaultdict(list)
    for (n, p, l), (st, fb, gb, msg) in zip(todo, result):
        counts[st] += 1
        if st in ('same', 'DIFF') and msg:
            warned.append((l, msg))
        if st in ('DIFF', 'FASM'):
            why = [w for m, q, w in REVIEWED if re.fullmatch(m, n) and re.fullmatch(q, l)]
            if why:
                counts['reviewed'] += 1
                if why[0].startswith('open'):
                    open_items[why[0]].append(l)
            else:
                report.append((st, l, fb, gb, msg))
        elif st == 'gas':
            gas_only[n].append((l, gb))
    print('%d forms: same %d, DIFF %d, FASM only %d, gas only %d, both refuse %d; reviewed %d (%d entries)'
          % (len(todo), counts['same'], counts['DIFF'], counts['FASM'], counts['gas'], counts['-'],
             counts['reviewed'], len(REVIEWED)))
    for why, ls in sorted(open_items.items()):
        print('reviewed, still open (%d): %s\n    e.g. %s' % (len(ls), why, ls[0]))
    if verbose:
        for n in sorted(gas_only):
            print('gas only  %-14s %5d  e.g. %s' % (n, len(gas_only[n]), gas_only[n][0][0]))
        for l, msg in warned:
            print('warning   %-40s %s' % (l, '; '.join(msg)))
    decoded = {}
    if use_xed:
        import _apx_xed
        blobs = sorted({b for r in report for b in r[2:4] if b} | {b for v in gas_only.values() for l, b in v})
        decoded = dict(zip(blobs, _apx_xed.xed(blobs)))
        # bytes only GNU as writes that XED refuses: #UD from gas
        bad = [(l, b, decoded[b]) for v in gas_only.values() for l, b in v if not decoded[b].startswith('OK')]
        print('gas only: %d forms, XED refuses %d of them (gas writes #UD bytes)'
              % (sum(len(v) for v in gas_only.values()), len(bad)))
        for l, b, r in bad if verbose else bad[:20]:
            print('  gas #UD  %-40s %-28s %s' % (l, b.hex(' '), r))
    for st, l, fb, gb, msg in report:
        print('%-4s  %-44s fasm %-30s gas %s%s' % (st, l, fb.hex(' '), gb.hex(' '),
              ('  [%s]' % '; '.join(msg)) if st == 'FASM' else ''))
        if decoded:
            print('        xed fasm: %s' % decoded.get(fb, '-'))
            print('        xed gas:  %s' % decoded.get(gb, '-'))
    print('%d unreviewed' % len(report))
    sys.exit(1 if report else 0)


if __name__ == '__main__':
    main()
