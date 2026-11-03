"""_ref_check.py -- check an IEV reference against llvm-mc and XED; optionally fill it.

    python tests/IEV/_ref_check.py FILE.ref [--write]

For each instruction line (`instruction ; hex`, the comment optional) of a
reference, in order and with its `use` lines:
  fasm2    assembles it (this tree);
  llvm-mc  assembles it (same mode: use16/32/64 -> .code16/32/64);
  XED      decodes fasm2's bytes, which must be exactly one instruction
           (or as many as the line has) and consume every byte.
Each line is reported as
  same     fasm2 = llvm-mc
  SHORTER  fasm2 shorter than llvm-mc (an optimization; XED confirms it)
  LONGER   fasm2 longer than llvm-mc: a missed optimization to examine
  alt      same length, other bytes (a valid alternative encoding)
  fasm     only fasm2 assembles it        llvm  only llvm-mc does
  XED!     XED refuses fasm2's bytes (#UD)
  rel      a jump to $+n: XED's target is right and it is the short form
           when the displacement fits (llvm-mc shows jumps unrelaxed)
  REF!     the reference's bytes differ from fasm2's (a regression or a
           reference to correct)
--write replaces each instruction line's comment with fasm2's bytes (after
review: SHORTER/alt lines are decisions, LONGER and XED! are not).
"""
import os, re, sys, subprocess
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, '..', 'iset'))
import _apx_diff	# noqa: E402

XEDCHECK = r'C:\git\_Intel\xed\obj-wkit\wkit\bin\xedcheck.exe'
REFLINE = re.compile(r'^(\s*)([^;]*?)\s*(;\s*(.*))?$')
HEX = re.compile(r'^([0-9A-Fa-f]{2})( [0-9A-Fa-f]{2})*$')


def llvm_syntax(line):
    line = _apx_diff.to_llvm(line)
    # fasm writes the segment inside the brackets, llvm-mc before them
    line = re.sub(r'\[\s*([cdefgs]s)\s*:\s*', r'\1:[', line)
    line = re.sub(r'(\w+ ptr) ([cdefgs]s:\[)', r'\1 \2', line)
    return line


def llvm(items):
    """items: [(mode, line)] -> [(ok, bytes)]"""
    src = ['.intel_syntax noprefix']
    mode = None
    for i, (m, l) in enumerate(items):
        if m != mode:
            src.append('.code%d' % m)
            mode = m
        # fasm's $ (this instruction) is the line's label for llvm-mc
        src += ['L%d:' % i, llvm_syntax(l).replace('$', 'L%d' % i)]
    p = subprocess.run([_apx_diff.LLVM_MC, '-triple=x86_64', '--show-encoding', '-output-asm-variant=1',
                        '-mattr=' + _apx_diff.MATTR], input='\n'.join(src) + '\n', capture_output=True, text=True)
    enc, cur = {}, None
    for l in p.stdout.splitlines():
        m = re.match(r'^L(\d+):', l)
        if m:
            cur = int(m.group(1))
            continue
        m = re.search(r'encoding: \[([^\]]*)\]', l)
        if m and cur is not None:
            enc[cur] = enc.get(cur, b'') + bytes(int(x, 16) for x in m.group(1).split(',') if x.strip())
    # map error lines back: find the Lk label preceding each error line number
    labels = {}
    for n, s in enumerate(src, 1):
        m = re.match(r'^L(\d+):$', s)
        if m:
            labels[n + 1] = int(m.group(1))
    bad = {labels.get(int(m.group(1))) for m in re.finditer(r'^<stdin>:(\d+):\d+: error', p.stderr, re.M)}
    return [(i in enc and i not in bad, enc.get(i, b'')) for i in range(len(items))]


def xed(blobs, modes):
    p = subprocess.run([XEDCHECK], input=''.join('%d:%s\n' % (m, b.hex()) for b, m in zip(blobs, modes)),
                       capture_output=True, text=True)
    return p.stdout.splitlines()


def main():
    path = sys.argv[1]
    write = '--write' in sys.argv
    text = open(path, encoding='utf-8').read().splitlines()
    mode = 64
    items, where, fasm_lines = [], [], []
    for n, raw in enumerate(text):
        m = REFLINE.match(raw)
        code = m.group(2).strip()
        if not code:
            continue
        um = re.match(r'^use(16|32|64)\b', code, re.I)
        if um:
            mode = int(um.group(1))
        if re.match(r'^use', code, re.I):
            fasm_lines.append(code)		# settings travel with the lines
            continue
        items.append((mode, code))
        where.append((n, len(fasm_lines)))
        fasm_lines.append(code)
    fasm_all = _apx_diff.fasm(fasm_lines)
    fasm = [fasm_all[k] for n, k in where]
    ref_llvm = llvm(items)
    dec = xed([b for ok, b in fasm], [m for m, c in items])
    counts = {}
    for (n, k), (mode, code), (fo, fb), (lo, lb), x in zip(where, items, fasm, ref_llvm, dec):
        ref = REFLINE.match(text[n]).group(4) or ''
        rel = re.search(r'\$([-+][0-9A-Fa-fh+\-]*)', code)
        if fo and not x.startswith('OK'):
            st = 'XED!'
        elif fo and rel:
            # llvm-mc shows a jump before relaxation; XED, decoding at
            # address 0, gives the target, which must be $ + the offset
            want = eval(re.sub(r'\b([0-9][0-9A-Fa-f]*)h\b', r'0x\1', rel.group(1))) % (1 << mode)
            got = re.search(r'0x([0-9a-f]+)\s*$', x)
            st = 'rel' if got and int(got.group(1), 16) % (1 << mode) == want else 'TARGET!'
            if st == 'rel':
                # shortest: the rel8 form whenever the displacement fits
                disp = want - 2		# the rel8 form is 2 bytes
                disp = disp - (1 << mode) if disp >= 1 << (mode - 1) else disp
                if -128 <= disp <= 127 and len(fb) > 2 and code.split()[0].lower() != 'call':
                    st = 'LONGER'
        elif fo and HEX.match(ref.strip()) and bytes.fromhex(ref.replace(' ', '')) != fb:
            st = 'REF!'
        elif fo and lo:
            st = 'same' if fb == lb else 'SHORTER' if len(fb) < len(lb) else 'LONGER' if len(fb) > len(lb) else 'alt'
        elif fo:
            st = 'fasm'
        elif lo:
            st = 'llvm'
        else:
            st = '-'
        counts[st] = counts.get(st, 0) + 1
        if st != 'same':
            print('%-7s %-44s fasm %-30s llvm %-30s %s' % (st, code, fb.hex(' ').upper() if fo else '-',
                                                           lb.hex(' ').upper() if lo else '-', x if st == 'XED!' else ''))
        if write and fo and st not in ('XED!',):
            indent = REFLINE.match(text[n]).group(1)
            text[n] = '%s%-40s; %s' % (indent, code, fb.hex(' ').upper())
    print(', '.join('%s %d' % kv for kv in sorted(counts.items())))
    if write:
        open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(text) + '\n')
    sys.exit(1 if counts.get('XED!') or counts.get('REF!') or counts.get('TARGET!') else 0)


if __name__ == '__main__':
    main()
