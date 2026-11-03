"""_apx_sweep.py -- no instruction may accept an APX extended register silently.

    python tests/iset/_apx_sweep.py [-v]

r16-r31 are defined as register elements (x86-2.inc), so every mnemonic can
see them. An encoder that does not know them masks the register number and
writes another register (add rax, r16 -> add rax, rax). Phase 0 of
include/iset/apx-plan.md makes the operand parsers refuse them; this sweep
checks that nothing gets past.

Every instruction name (from _dupnames.py's scan of x86-2.inc and iset/*.inc,
plus the mnemonics in zydis-db) is first probed bare; names fasm2 does not
define are dropped. Each defined name is then assembled, under
`use everything` in 64-bit mode, with each operand pattern in PATTERNS, one
line per form. Every line names an extended register, so every line must
fail. fasmg reports each failing line; a line it does not report assembled,
and is a leak.

ALLOWED lists (mnemonic, pattern) pairs that later APX phases encode for
real; it starts empty and grows only with tested APX forms.

-v also prints how many lines failed with each error message.
"""
import os, re, sys, json, subprocess, collections, tempfile

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(os.path.dirname(here))
sys.path.insert(0, here)
from _dupnames import defs	# noqa: E402

ZYDIS_DB = r'C:\git\zyantific\zydis-db\datafiles\instructions.json'

# Intel operand order; r16 stands for every extended register (the check is
# on the register number, so r16 and r31 take the same path)
PATTERNS = [
    # GPR alone and in pairs, each size
    'r16', 'r16d', 'r16w', 'r16b', 'r16l',
    'r16, rax', 'rax, r16', 'r16d, eax', 'eax, r16d', 'r16w, ax', 'ax, r16w',
    'r16b, al', 'al, r16b', 'r16, 1', 'r16d, 1', 'r16, cl',
    'r16, rax, 1', 'rax, r16, 1', 'r16, [rax], 1', 'rax, r16, cl',
    # three-operand GPR (BMI, CMPccXADD, NDD-shaped)
    'r16, rax, rbx', 'rax, r16, rbx', 'rax, rbx, r16',
    'r16d, eax, ebx', 'eax, r16d, ebx', 'eax, ebx, r16d',
    'r16, [rax], rbx', 'rax, [r16], rbx', 'rax, rbx, [r16]', '[r16], rax, rbx',
    # memory: base, index, both sizes, with register and immediate
    '[r16]', 'qword [r16]', 'dword [r16]', 'word [r16]', 'byte [r16]', 'tword [r16]',
    '[rax+r16]', '[r16+rax*2]', '[rax+r16*8+8]', '[r16d]', '[eax+r16d]', '[r16w]',
    'rax, [r16]', '[r16], rax', 'eax, [r16]', '[r16], eax', 'ax, [r16]',
    'al, [r16]', '[r16], al', '[r16], 1', 'byte [r16], 1', 'qword [r16], 1',
    'r16, [rax]', '[rax], r16', 'r16d, [rax]', '[rax], r16d',
    'byte [r16], byte [rax]', 'byte [rax], byte [r16]', 'far [r16]', 'fs:[r16]',
    # system registers
    'r16, cr0', 'cr0, r16', 'r16, dr0', 'dr0, r16', 'es, r16w', 'r16w, es',
    # MMX/SSE/AVX with GPR or address operands
    'mm0, [r16]', '[r16], mm0', 'mm0, r16d', 'r16d, mm0', 'mm0, r16d, 1', 'r16d, mm0, 1',
    'xmm0, [r16]', '[r16], xmm0', 'ymm0, [r16]', '[r16], ymm0', 'zmm0, [r16]', '[r16], zmm0',
    'xmm0, xmm1, [r16]', 'ymm0, ymm1, [r16]', 'zmm0, zmm1, [r16]',
    'xmm0, [r16], 1', 'xmm0, xmm1, [r16], 1', 'xmm0, xmm1, [r16], xmm2',
    'xmm0, r16d', 'r16d, xmm0', 'xmm0, r16', 'r16, xmm0', 'xmm0, xmm1, r16d', 'xmm0, xmm1, r16',
    'r16d, xmm0, 1', 'xmm0, r16d, 1', 'xmm0, xmm1, r16d, 1', '[r16], xmm0, 1',
    'xmm0, [r16]{1to4}', 'zmm0, zmm1, [r16]{1to16}', 'zmm0{k1}, [r16]',
    'k1, r16d', 'r16d, k1', 'k1, r16', 'r16, k1', 'k1, [r16]', '[r16], k1',
    # VSIB, MIB and AMX
    'zmm0{k1}, [r16+zmm1]', '[r16+zmm1]{k1}, zmm0', '[r16+zmm1]{k1}',
    'xmm0, [r16+xmm1], xmm2', 'ymm0, [r16+ymm1], ymm2',
    'bnd0, [r16]', 'bnd0, [r16,rax]', '[r16,rax], bnd0', 'bnd0, r16',
    'tmm0, [r16,rax]', 'tmm0, [rax+r16]', '[r16,rax], tmm0', '[r16]',
    # FPU memory forms
    'st0, [r16]', 'qword [r16], st0',
]
PATTERNS = list(dict.fromkeys(PATTERNS))

ALLOWED = set()	# (mnemonic, pattern) pairs encoded by an APX phase

# not instructions: settings, prefixes that take an instruction, directives
SKIP = re.compile(r'^(use\w*|require\w*|xacquire|xrelease)$')


def names():
    out = set(k for k in defs if re.fullmatch(r'[a-z_][a-z0-9_]*', k))
    if os.path.exists(ZYDIS_DB):
        for rec in json.load(open(ZYDIS_DB, encoding='utf-8')):
            m = rec.get('mnemonic', '').lower()
            if re.fullmatch(r'[a-z_][a-z0-9_]*', m):
                out.add(m)
    return sorted(n for n in out if not SKIP.match(n))


def assemble(lines):
    """Return {line number: [error messages]} for a source of `lines`."""
    fd, src = tempfile.mkstemp(suffix='.asm')
    with os.fdopen(fd, 'w') as f:
        f.write('use64\nuse everything\n' + '\n'.join(lines) + '\n')
    env = dict(os.environ, INCLUDE=os.path.join(root, 'include') + ';' + os.environ.get('INCLUDE', ''))
    p = subprocess.run([os.path.join(root, 'fasmg.exe'), "-iInclude('fasm2.inc')", '-e1000000',
                        src, src + '.bin'], capture_output=True, text=True, env=env)
    errors = collections.defaultdict(list)
    current = None
    tag = os.path.basename(src)
    for line in (p.stdout + p.stderr).splitlines():
        m = re.match(r'.*%s \[(\d+)\]:' % re.escape(tag), line)
        if m:
            current = int(m.group(1)) - 2	# two header lines
            continue
        m = re.match(r'(?:Custom )?[Ee]rror: (.*)', line)
        if m and current is not None:
            errors[current].append(m.group(1).rstrip('.'))
    for f in (src, src + '.bin'):
        if os.path.exists(f):
            os.remove(f)
    return errors


def main():
    verbose = '-v' in sys.argv
    candidates = names()
    probe = assemble(candidates)
    defined = [n for i, n in enumerate(candidates, 1)
               if 'illegal instruction' not in probe.get(i, [])]
    lines, forms = [], []
    for n in defined:
        for pat in PATTERNS:
            if (n, pat) in ALLOWED:
                continue
            lines.append('%s %s' % (n, pat))
            forms.append((n, pat))
    leaks, messages = [], collections.Counter()
    chunk = 20000
    for start in range(0, len(lines), chunk):
        errs = assemble(lines[start:start + chunk])
        for i, form in enumerate(forms[start:start + chunk], 1):
            if i not in errs:
                leaks.append(form)
            elif verbose:
                messages[errs[i][-1]] += 1
    print('%d names, %d defined by fasm2, %d forms tried, %d allowed'
          % (len(candidates), len(defined), len(lines), len(ALLOWED)))
    if verbose:
        for msg, count in messages.most_common():
            print('%7d  %s' % (count, msg))
    for n, pat in leaks:
        print('LEAK  %s %s' % (n, pat))
    print('%d leak(s)' % len(leaks))
    sys.exit(1 if leaks else 0)


if __name__ == '__main__':
    main()
