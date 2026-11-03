"""_iset2ref.py -- write the iset tests in IEV form: pass/ and fail/.

    python tests/IEV/_iset2ref.py [--check]

tests/iset/*.asm check bytes with `expect <bytes>, instruction` inside
VIRTUAL, and errors with `FAIL n` cases assembled one per run. This writes
both as IEV references, one file per test and kind:

    pass/iset/<test>.ref:   instruction		; HEX BYTES
    fail/iset/<test>.ref:   instruction		; 'error message'

pass: each test's main block (before its FAIL cases), at top level only:
`use` lines and `expect` lines with literal bytes. Lines in iterate/irp/
repeat/macro blocks, or using the test's own helper macros, stay iset-only.

fail: each `FAIL n` case whose lines are plain (no nested blocks): the
test's header `use` lines and a reset of the encoding preferences, the
case's lines as setup, and the message from `; FAIL n: '...'` on its last
line. Cases share one file, so the reset keeps them independent.

With --check nothing is written; the exit code is the number of files that
differ from what the iset tests give, so the forms cannot drift apart.
"""
import os, re, sys, glob

here = os.path.dirname(os.path.abspath(__file__))
iset = os.path.join(here, '..', 'iset')
PASS_DIR = os.path.join(here, 'pass', 'iset')
FAIL_DIR = os.path.join(here, 'fail', 'iset')

BLOCK = r'(irp|irps|irpv|iterate|repeat|rept|macro|calminstruction|while|virtual|namespace|struct|if|match)'
OPEN = re.compile(r'^\s*%s\b' % BLOCK, re.I)
CLOSE = re.compile(r'^\s*end\s+%s\b' % BLOCK, re.I)
EXPECT = re.compile(r'^\s*expect\s+<([^>]*)>\s*,\s*(.*?)\s*$')
USE = re.compile(r'^\s*(use\w*\b.*?)\s*(;.*)?$', re.I)
FAIL_MSG = re.compile(r"^;\s*FAIL\s+(\d+):\s*'([^']*)'")
FAIL_CASE = re.compile(r'^\s*else\s+if\s+FAIL\s*=\s*(\d+)\s*(;\s*(.*))?$')
RESET = 'use vexauto, rexauto'


def hexbyte(tok):
    t = tok.strip()
    m = re.fullmatch(r'0*([0-9A-Fa-f]{1,2})h', t) or re.fullmatch(r'0x([0-9A-Fa-f]{1,2})', t)
    if m:
        return '%02X' % int(m.group(1), 16)
    if re.fullmatch(r'\d+', t) and int(t) < 256:
        return '%02X' % int(t)
    return None


def code_of(raw):
    return raw.split(';', 1)[0].strip()


def convert(path):
    lines = open(path, encoding='utf-8').read().splitlines()
    local = {m.group(1).lower() for m in
             (re.match(r'^\s*(?:macro|calminstruction)\s+([\w.]+)', l) for l in lines) if m}
    messages = {int(m.group(1)): m.group(2) for m in (FAIL_MSG.match(l) for l in lines) if m}
    passes, header, cases, skipped = [], [], [], 0
    # header, main, fail; a test without FAIL cases is all main block
    has_fail = any(re.match(r'^\s*if\s+~\s*defined\s+FAIL\b', l) for l in lines)
    section = 'header' if has_fail else 'main'
    depth = 0
    case = None
    for raw in lines:
        s = code_of(raw)
        if re.match(r'^if\s+~\s*defined\s+FAIL\b', s):
            section = 'main'
            continue
        m = FAIL_CASE.match(raw)
        if m and depth == 0:
            section = 'fail'
            case = {'n': int(m.group(1)), 'why': (m.group(3) or '').strip(), 'lines': [], 'plain': True}
            cases.append(case)
            continue
        if section == 'fail' and depth == 0 and re.match(r'^end\s+if\b', s):
            section = 'done'
            continue
        if CLOSE.match(s):
            depth -= 1
            if section == 'fail' and case:
                case['plain'] = False
            continue
        if OPEN.match(s):
            depth += 1
            if section == 'fail' and case:
                case['plain'] = False
            continue
        if section == 'header':
            if depth == 0 and USE.match(s):
                header.append(USE.match(s).group(1))
                passes.append(USE.match(s).group(1))
            continue
        if section == 'fail':
            if s and case:
                case['lines'].append(s)
            continue
        if section != 'main':
            continue
        if depth:
            if EXPECT.match(raw):
                skipped += 1
            continue
        m = EXPECT.match(raw)
        if m:
            data, instr = m.group(1), re.sub(r'\s*;.*$', '', m.group(2))
            bs = [hexbyte(b) for b in data.split(',')]
            mnemonic = re.sub(r'^\{[^}]*\}\s*', '', instr).split()[0].lower() if instr else ''
            if None in bs or not instr or mnemonic in local:
                skipped += 1
                continue
            passes.append('%s\t\t; %s' % (instr, ' '.join(bs)))
            continue
        m = USE.match(s)
        if m:
            passes.append(m.group(1))
    fails, fail_skipped = [], 0
    for c in cases:
        msg = messages.get(c['n'])
        body = [l for l in c['lines']]
        if not c['plain'] or not msg or not body or any(l.split()[0].lower() in local for l in body):
            fail_skipped += 1
            continue
        last = len(body) - 1
        while last >= 0 and USE.match(body[last]):
            last -= 1
        if last < 0:
            fail_skipped += 1
            continue
        fails.append('')
        fails.append('; FAIL %d%s' % (c['n'], (': ' + c['why']) if c['why'] else ''))
        fails += header + [RESET]
        fails += body[:last]
        fails.append("%s\t\t; '%s'" % (body[last], msg))
        fails += body[last + 1:]
    return passes, skipped, fails, fail_skipped


def emit(target, name, kind, body, check):
    text = ('; %s.ref -- %s of tests/iset/%s.asm in IEV form\n'
            '; generated by tests/IEV/_iset2ref.py; edit the iset test, not this file\n\n'
            % (name, kind, name)) + '\n'.join(body).lstrip('\n') + '\n'
    old = open(target, encoding='utf-8').read() if os.path.exists(target) else None
    if old == text:
        return 0
    if check:
        print('stale: %s' % os.path.relpath(target, here))
    else:
        open(target, 'w', encoding='utf-8', newline='\n').write(text)
    return 1


def main():
    check = '--check' in sys.argv
    for d in (PASS_DIR, FAIL_DIR):
        os.makedirs(d, exist_ok=True)
    stale = n_pass = n_fail = skipped_pass = skipped_fail = 0
    for path in sorted(glob.glob(os.path.join(iset, '*.asm'))):
        name = os.path.splitext(os.path.basename(path))[0]
        passes, sp, fails, sf = convert(path)
        skipped_pass += sp
        skipped_fail += sf
        k = sum(1 for l in passes if ';' in l)
        if k:
            n_pass += k
            stale += emit(os.path.join(PASS_DIR, name + '.ref'), name, 'byte checks', passes, check)
        k = sum(1 for l in fails if re.search(r";\s*'", l))
        if k:
            n_fail += k
            stale += emit(os.path.join(FAIL_DIR, name + '.ref'), name, 'error checks', fails, check)
    print('%d pass and %d fail references from tests/iset (%d expect lines and %d FAIL cases left '
          'iset-only); %d file(s) %s' % (n_pass, n_fail, skipped_pass, skipped_fail, stale,
                                         'stale' if check else 'written'))
    sys.exit(min(stale, 255) if check else 0)


if __name__ == '__main__':
    main()
