"""_run.py -- run every IEV test with this tree's fasm2 (called by _run.ps1).

    python tests/IEV/_run.py [-v]

  *.asm        Tomasz Grysztar's form: the file includes fasm2.inc and
               iev.alm itself; any "Discrepancy:" or error fails it.
  pass/**.ref  instruction ; HEX BYTES -- assembled with iev.alm; any
               "Discrepancy:" or error fails it.
  fail/**.ref  instruction ; 'error message' -- assembled with fail.alm;
               each such line must stop with an error containing the
               message, and every other line must assemble without one.
Each .ref is assembled on its own (fasm2.inc and the module given on the
command line), so one file's settings cannot leak into the next. Finally
_iset2ref.py --check: pass/iset and fail/iset must match tests/iset.
"""
import os, re, sys, glob, subprocess, tempfile

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.abspath(os.path.join(here, '..', '..'))
FASMG = os.path.join(root, 'fasmg.exe')
ENV = dict(os.environ, INCLUDE=os.path.join(root, 'include') + ';' + here)
OUT = os.path.join(tempfile.gettempdir(), 'fasm2_iev')
os.makedirs(OUT, exist_ok=True)
verbose = '-v' in sys.argv


def fasmg(src, module=None):
    args = [FASMG, '-e10000']
    if module:
        args += ["-iInclude('fasm2.inc')", "-iInclude('%s')" % module]
    out = os.path.join(OUT, os.path.basename(src) + '.bin')
    p = subprocess.run(args + [src, out], capture_output=True, text=True, env=ENV)
    return p.returncode, p.stdout + p.stderr


def errors_by_line(text, src):
    """{line number in src: [messages]}, from fasmg's error reports."""
    name = re.escape(os.path.basename(src))
    found, current = {}, None
    for line in text.splitlines():
        m = re.match(r'^(.*?%s) \[(\d+)\]:' % name, line)
        if m and current is None:
            current = int(m.group(2))
            continue
        m = re.match(r'^(?:Custom )?[Ee]rror: (.*)', line)
        if m:
            found.setdefault(current, []).append(m.group(1))
            current = None
    return found


def report(name, ok, detail, count):
    print('[%s] %s (%d)' % ('pass' if ok else 'FAIL', name, count))
    if not ok or verbose:
        for d in detail:
            print('         ' + d)


def run_bytes(src, module):
    code, text = fasmg(src, module)
    count = sum(1 for l in open(src, encoding='utf-8') if re.search(r';\s*[0-9A-Fa-f]{2}( [0-9A-Fa-f]{2})*\s*$', l))
    ok = code == 0 and 'Discrepancy:' not in text
    detail = [l for l in text.splitlines() if l and not l.startswith('flat assembler')]
    report(os.path.relpath(src, here), ok, detail if not ok else [], count)
    return ok, count


def run_fail(src):
    lines = open(src, encoding='utf-8').read().splitlines()
    want = {}
    for n, l in enumerate(lines, 1):
        m = re.search(r";\s*'([^']*)'\s*$", l)
        if m and not l.lstrip().startswith(';'):
            want[n] = m.group(1)
    code, text = fasmg(src, 'fail.alm')
    got = errors_by_line(text, src)
    detail = []
    for n, msg in sorted(want.items()):
        errs = got.get(n, [])
        if not any(msg in e for e in errs):
            detail.append('line %d: %s' % (n, lines[n - 1].split(';')[0].strip()))
            detail.append('   expected: %s' % msg)
            detail.append('   got:      %s' % ('; '.join(errs) if errs else 'no error (assembled)'))
    for n, errs in sorted(((k, v) for k, v in got.items() if k not in want), key=lambda kv: kv[0] or 0):
        detail.append('line %s: unexpected error: %s' % (n, '; '.join(errs)))
        if n:
            detail.append('   %s' % lines[n - 1].strip())
    ok = not detail
    report(os.path.relpath(src, here), ok, detail, len(want))
    return ok, len(want)


def main():
    failed = checks = 0
    for src in sorted(glob.glob(os.path.join(here, '*.asm'))):
        ok, k = run_bytes(src, None)
        failed += not ok
        checks += k
    for src in sorted(glob.glob(os.path.join(here, 'pass', '**', '*.ref'), recursive=True)):
        ok, k = run_bytes(src, 'iev.alm')
        failed += not ok
        checks += k
    for src in sorted(glob.glob(os.path.join(here, 'fail', '**', '*.ref'), recursive=True)):
        ok, k = run_fail(src)
        failed += not ok
        checks += k
    p = subprocess.run([sys.executable, os.path.join(here, '_iset2ref.py'), '--check'], capture_output=True, text=True)
    print(p.stdout.strip())
    if p.returncode:
        print('[FAIL] pass/iset or fail/iset out of date: run tests\\IEV\\_iset2ref.py')
        failed += 1
    if failed:
        print('%d IEV file(s) failed (%d checks)' % (failed, checks))
        sys.exit(1)
    print('all IEV checks passed (%d checks)' % checks)


if __name__ == '__main__':
    main()
