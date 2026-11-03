"""_apx_diff.py -- assemble Intel-syntax lines with fasm2 and llvm-mc, compare bytes.

    python tests/iset/_apx_diff.py FILE      (one instruction per line)

Library for _apx_sweep.py; run on a file it prints one line per input:
    same   both assemble, same bytes
    DIFF   both assemble, different bytes
    FASM   only fasm2 assembles it
    llvm   only llvm-mc assembles it (fasm2 refuses; not an error by itself)
    -      both refuse

fasm2 assembles every line inside VIRTUAL under `use64`, `use everything`
and displays its bytes; a line with any error counts as refused even if it
displayed bytes. llvm-mc (LLVM 23, build-native) assembles the same lines in
Intel syntax with the APX features on; fasm's size keywords get `ptr`.
"""
import os, re, sys, subprocess, tempfile

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(os.path.dirname(here))
LLVM_MC = r'C:\git\llvm-project\build-native\bin\llvm-mc.exe'
MATTR = ('+egpr,+push2pop2,+ppx,+ndd,+ccmp,+nf,+cf,+zu,+avx512f,+avx512bw,+avx512dq,'
         '+avx512vl,+bmi,+bmi2,+adx,+movbe,+cmpccxadd,+raoint,+usermsr,+amx-tile,+shstk')

FASM_PRELUDE = r'''use64
use everything
macro x86.t id, line&
	local n, b, h, l
	virtual at 0
		line
		n = $
		load b:n from 0
	end virtual
	display 'T', `id, ':'
	repeat n
		h = ((b shr (8*(%-1))) and 0FFh) shr 4
		l = (b shr (8*(%-1))) and 0Fh
		display ' ', h + '0' + (h / 10) * 7, l + '0' + (l / 10) * 7
	end repeat
	display 13, 10
end macro
'''
PRELUDE_LINES = FASM_PRELUDE.count('\n')

SIZES = {'byte': 'byte', 'word': 'word', 'dword': 'dword', 'qword': 'qword',
         'tword': 'tbyte', 'tbyte': 'tbyte', 'dqword': 'xmmword', 'xword': 'xmmword',
         'qqword': 'ymmword', 'yword': 'ymmword', 'dqqword': 'zmmword', 'zword': 'zmmword'}


# fasm2 mnemonics llvm-mc spells differently (same encoding in 64-bit code)
MNEMONICS = {'pushq': 'push', 'popq': 'pop', 'pushw': 'push', 'popw': 'pop',
             'fldenvd': 'fldenv', 'fstenvd': 'fstenv', 'fnstenvd': 'fnstenv',
             'fsaved': 'fsave', 'fnsaved': 'fnsave', 'frstord': 'frstor'}


def to_llvm(line):
    # fasm2 writes instruction options after the mnemonic, llvm-mc before it
    line = re.sub(r'^(\{\w+\}\s*)?(\w+)\s+\{(nf)\}\s*', r'\1{\3} \2 ', line)
    # ZU: setCC {zu} -> setzuCC, imul {zu} -> imulzu
    line = re.sub(r'\bset(\w+)\s+\{zu\}\s*', r'setzu\1 ', line)
    line = re.sub(r'\bimul\s+\{zu\}\s*', 'imulzu ', line)
    m = re.match(r'(\{\w+\}\s*)?(\w+)(.*)', line)
    if m and m.group(2) in MNEMONICS:
        rest = m.group(3)
        if m.group(2) in ('pushw', 'popw', 'pushq', 'popq'):	# the suffix sizes a memory operand
            rest = re.sub(r'^\s*\[', ' %s [' % ('word' if m.group(2)[-1] == 'w' else 'qword'), rest)
        line = (m.group(1) or '') + MNEMONICS[m.group(2)] + rest
    line = re.sub(r'\b(r\d+)l\b', r'\1b', line)
    line = re.sub(r'\b(%s)\s+(?=[\[\w]+:?\[|\[)' % '|'.join(SIZES),
                  lambda m: SIZES[m.group(1)] + ' ptr ', line)
    line = re.sub(r'\bst(\d)\b', r'st(\1)', line)
    line = re.sub(r'\b([0-9][0-9a-fA-F]*)h\b', r'0x\1', line)
    return line


def fasm_errors(lines):
    """{index: error messages} for the lines fasmg rejects, assembled as they are."""
    fd, src = tempfile.mkstemp(suffix='.asm')
    with os.fdopen(fd, 'w') as f:
        f.write('use64\nuse everything\n' + '\n'.join(lines) + '\n')
    env = dict(os.environ, INCLUDE=os.path.join(root, 'include') + ';' + os.environ.get('INCLUDE', ''))
    p = subprocess.run([os.path.join(root, 'fasmg.exe'), "-iInclude('fasm2.inc')", '-e10000000',
                        src, src + '.bin'], capture_output=True, text=True, env=env)
    errors, current = {}, None
    tag = re.escape(os.path.basename(src))
    for line in (p.stdout + p.stderr).splitlines():
        m = re.match(r'.*%s \[(\d+)\]:' % tag, line)
        if m:
            current = int(m.group(1)) - 3
            continue
        m = re.match(r'(?:Custom )?[Ee]rror: (.*)', line)
        if m and current is not None:
            errors[current] = errors.get(current, '') + m.group(1)
    for f in (src, src + '.bin'):
        if os.path.exists(f):
            os.remove(f)
    return errors


def fasm(lines, chunk=20000):
    """[(ok, bytes)] for each line."""
    out = []
    for start in range(0, len(lines), chunk):
        part = lines[start:start + chunk]
        fd, src = tempfile.mkstemp(suffix='.asm')
        with os.fdopen(fd, 'w') as f:
            f.write(FASM_PRELUDE)
            for i, line in enumerate(part):
                f.write('x86.t %d, %s\n' % (i, line))
        env = dict(os.environ, INCLUDE=os.path.join(root, 'include') + ';' + os.environ.get('INCLUDE', ''))
        p = subprocess.run([os.path.join(root, 'fasmg.exe'), "-iInclude('fasm2.inc')", '-e10000000',
                            src, src + '.bin'], capture_output=True, text=True, env=env)
        text = p.stdout + p.stderr
        got = {}
        for m in re.finditer(r'^T(\d+):([0-9A-F ]*)$', text, re.M):
            got[int(m.group(1))] = bytes.fromhex(m.group(2))
        bad = set()
        tag = re.escape(os.path.basename(src))
        for m in re.finditer(r'%s \[(\d+)\]:' % tag, text):
            bad.add(int(m.group(1)) - PRELUDE_LINES - 1)
        for i in range(len(part)):
            out.append((i not in bad and i in got, got.get(i, b'')))
        for f in (src, src + '.bin'):
            if os.path.exists(f):
                os.remove(f)
    return out


def llvm(lines, chunk=20000):
    """[(ok, bytes)] for each line."""
    out = []
    for start in range(0, len(lines), chunk):
        part = lines[start:start + chunk]
        src = '.intel_syntax noprefix\n' + ''.join('L%d:\n%s\n' % (i, to_llvm(l)) for i, l in enumerate(part))
        p = subprocess.run([LLVM_MC, '-triple=x86_64', '--show-encoding', '-output-asm-variant=1',
                            '-mattr=' + MATTR], input=src, capture_output=True, text=True)
        enc = {}
        current = None
        for line in p.stdout.splitlines():
            m = re.match(r'^L(\d+):', line)
            if m:
                current = int(m.group(1))
                continue
            m = re.search(r'encoding: \[([^\]]*)\]', line)
            if m and current is not None:
                b = bytes(int(x, 16) for x in m.group(1).split(',') if x.strip())
                enc[current] = enc.get(current, b'') + b
        bad = set()
        for m in re.finditer(r'^<stdin>:(\d+):\d+: error', p.stderr, re.M):
            bad.add((int(m.group(1)) - 2) // 2)
        for i in range(len(part)):
            out.append((i in enc and i not in bad, enc.get(i, b'')))
    return out


def compare(lines):
    """[(status, fasm bytes, llvm bytes)] for each line."""
    result = []
    for (fo, fb), (lo, lb) in zip(fasm(lines), llvm(lines)):
        if fo and lo:
            result.append(('same' if fb == lb else 'DIFF', fb, lb))
        elif fo:
            result.append(('FASM', fb, lb))
        elif lo:
            result.append(('llvm', fb, lb))
        else:
            result.append(('-', fb, lb))
    return result


if __name__ == '__main__':
    lines = [l.rstrip('\n') for l in open(sys.argv[1]) if l.strip()]
    for line, (st, fb, lb) in zip(lines, compare(lines)):
        print('%-5s %-36s fasm %-28s llvm %s' % (st, line, fb.hex(' '), lb.hex(' ')))
