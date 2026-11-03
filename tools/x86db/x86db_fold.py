"""x86db_fold.py -- factor XED forms into a flat table and a few templates.

    python tools/x86db/x86db_fold.py [build/xed.json] [-v]

x86db_stats.py shows that XED's patterns, taken whole, barely coalesce:
they mix what varies per row (map, prefix, W, L, opcode, ModRM digit,
element size) with the structure of the encoding. Here each form becomes

    row       = mnemonic + parameters (space, map, pp, opcode, W, L, digit,
                tuple type, ISA set, constraints)
    template  = the operand roles and the encoding structure that remain,

and folds are applied one after another, each printed with the number of
templates and rows left:

  1. fields      split the pattern into parameter fields and structure
  2. roles       operands as role:class (R, B, N, M, I, K ...), access,
                 width and element type dropped (rows keep them)
  3. reg/mem     a register form and a memory form that differ only in
                 ModRM.mod and the B operand fold into one r/m row
  4. vector len  rows that differ only in L and xmm/ymm/zmm fold into one
  5. width/W     rows that differ only in W and GPR operand size fold

A template is what one CALM encoder routine must handle; a row is one
line of the generated table.
"""
import os, re, sys, json, collections

here = os.path.dirname(os.path.abspath(__file__))
SPACES = {'VV1': 'VEX', 'EVV': 'EVEX', 'XOPV': 'XOP', 'KVV': 'KNC'}
MAPS = {'V0F': '0F', 'V0F38': '0F38', 'V0F3A': '0F3A', 'MAP4': 'M4', 'MAP5': 'M5', 'MAP6': 'M6',
        'MAP7': 'M7', 'XMAP8': 'X8', 'XMAP9': 'X9', 'XMAPA': 'XA'}
PPS = {'V66': '66', 'VF2': 'F2', 'VF3': 'F3', 'VNP': 'NP', 'osz_refining_prefix': '66',
       'f2_refining_prefix': 'F2', 'f3_refining_prefix': 'F3', 'no_refining_prefix': 'NP',
       'refining_f3': 'F3', 'not_refining_f3': 'nF3', 'not_refining': 'NP', 'REFINING66()': None}
WS = {'W0': 'W0', 'W1': 'W1', 'rexw_prefix': 'W1', 'norexw_prefix': 'W0'}
LS = {'VL128': 128, 'VL256': 256, 'VL512': 512}
IMMS = ('UIMM8()', 'SIMM8()', 'SIMMz()', 'UIMM16()', 'UIMM32()', 'UIMMv()', 'UIMM8_1()',
        'SE_IMM8()', 'BRDISP8()', 'BRDISPz()', 'BRDISP32()', 'BRDISP64()', 'MEMDISPv()', 'ONE()')
# ModRM/SIB plumbing, implied by the ModRM form
PLUMBING = re.compile(r'^(MOD\[.*\]|REG\[rrr\]|RM\[nnn\]|MODRM\(\)|UBIT=1|ZEROING=0|BCRC=0|EVAPX\(\)|'
                      r'EVAPX_SCC\(\)|NOEVSR|NOVSR)$')


def fields(rec):
    pat = rec['pattern']
    f = {'space': next((SPACES[t] for t in pat if t in SPACES), 'LEGACY'), 'map': '', 'pp': '',
         'W': '', 'L': 0, 'modrm': '', 'digit': None, 'rm': None, 'imm': [], 'tuple': [], 'apx': [],
         'mode': [], 'other': []}
    seen_opcode = False
    legacy_map = []
    for t in pat:
        if t in SPACES:
            continue
        if re.fullmatch(r'0x[0-9A-Fa-f]{1,2}', t):
            b = int(t, 16)
            if f['space'] == 'LEGACY' and not seen_opcode and (
                    (b == 0x0F and not legacy_map) or (legacy_map == [0x0F] and b in (0x38, 0x3A, 0x0F))):
                legacy_map.append(b)
                continue
            if not seen_opcode:
                f['opcode'] = b
                seen_opcode = True
            else:
                f['other'].append('IB=%02X' % b)
            continue
        if t in MAPS:
            f['map'] = MAPS[t]
        elif t in PPS:
            if PPS[t]:
                f['pp'] = PPS[t]
        elif t in WS:
            f['W'] = WS[t]
        elif t in LS:
            f['L'] = LS[t]
        elif t == 'MOD=3':
            f['modrm'] = 'r'
        elif t == 'MOD!=3':
            f['modrm'] = 'm'
        elif re.fullmatch(r'REG\[0b([01]{3})\]', t):
            f['digit'] = int(t[6:9], 2)
        elif re.fullmatch(r'RM\[0b([01]{3})\]', t):
            f['rm'] = int(t[5:8], 2)
        elif t in IMMS:
            f['imm'].append(t[:-2])
        elif t.startswith(('ESIZE_', 'NELEM_')):
            f['tuple'].append(t[:-2])
        elif re.match(r'^(ND|NF|SCC\d*|NO_SCC_NF\d)', t) or t.startswith('SCC'):
            f['apx'].append(t)
        elif t in ('mode64', 'not64', 'mode16', 'mode32', 'not16', 'eanot16', 'eamode16', 'eamode32',
                   'eamode64'):
            f['mode'].append(t)
        elif PLUMBING.match(t):
            continue
        else:
            f['other'].append(t)
    if legacy_map:
        f['map'] = ''.join('%02X' % b for b in legacy_map)
    if 'MODRM()' in pat and not f['modrm']:
        f['modrm'] = 'm'
    return f


def role(op):
    """REG0=XMM_R3():w:dq:f32 -> ('R', 'XMM'); MEM0:r:dq -> ('M', 'MEM')"""
    head = op.split(':')[0]
    if head.startswith('MEM'):
        return ('M', 'MEM')
    if head.startswith('AGEN'):
        return ('M', 'AGEN')
    if head.startswith('IMM'):
        return ('I', 'IMM')
    if head.startswith(('RELBR', 'ABSBR', 'PTR')):
        return ('J', head.split('=')[0])
    m = re.match(r'REG\d=(\w+?)(?:_(R|B|N|SE)\d?\w*)?\(\)', head)
    if m:
        cls, r = m.group(1), m.group(2) or '?'
        if cls.startswith('MASK'):
            return ('K', 'MASK') if cls == 'MASK1' else (r, 'MASK')
        return (r, cls)
    if 'SUPP' in op or 'IMPL' in op or 'XED_REG_' in head:
        return ('-', head.split('=')[-1])		# implicit: no encoding role
    return ('?', head)


def vec(cls):
    return re.sub(r'^(XMM|YMM|ZMM)', 'VEC', cls)


def gpr(cls):
    return re.sub(r'^GPR(8|16|32|64|v|z|y)', 'GPR', cls)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    verbose = '-v' in sys.argv
    recs = json.load(open(args[0] if args else os.path.join(here, 'build', 'xed.json')))
    forms = []
    for r in recs:
        f = fields(r)
        ops = tuple(role(o) for o in r['operands'] if not re.search(r':SUPP|:IMPL|XED_REG_', o))
        forms.append((r, f, ops))

    def structure(f, ops, fold_rm=False, fold_vl=False, fold_w=False):
        o = ops
        if fold_vl:
            o = tuple((a, vec(b)) for a, b in o)
        if fold_w:
            o = tuple((a, gpr(b)) for a, b in o)
        if fold_rm and f['modrm'] in ('r', 'm'):
            # the register (B) and memory (M) operand of ModRM.rm are one r/m operand
            o = tuple(('BM', 'RM') if a in ('B', 'M') else (a, b) for a, b in o)
        modrm = '' if fold_rm and f['modrm'] in ('r', 'm') else f['modrm']
        return (f['space'], modrm, f['digit'] is not None, f['rm'] is not None, tuple(f['imm']),
                tuple(sorted(f['apx'])) != (), o)

    def row(r, f, ops, fold_rm=False, fold_vl=False, fold_w=False):
        key = [r['iclass'], f['space'], f['map'], f['pp'], f.get('opcode'), f['digit'], f['rm'],
               tuple(f['imm']), tuple(sorted(f['apx'])), tuple(f['mode']), tuple(sorted(f['other']))]
        if not fold_w:
            key.append(f['W'])
        if not fold_vl:
            key.append(f['L'])
        key.append(structure(f, ops, fold_rm, fold_vl, fold_w))
        if not fold_rm:
            key.append(f['modrm'])
        return tuple(map(str, key))

    steps = [('1 fields', {}), ('3 reg/mem', {'fold_rm': True}),
             ('4 vector length', {'fold_rm': True, 'fold_vl': True}),
             ('5 width/W', {'fold_rm': True, 'fold_vl': True, 'fold_w': True})]
    print('%d forms, %d mnemonics' % (len(forms), len({r['iclass'] for r, f, o in forms})))
    for name, kw in steps:
        rows = {row(r, f, o, **kw) for r, f, o in forms}
        tmpl = collections.Counter(structure(f, o, **kw) for r, f, o in forms)
        acc, k90 = 0, 0
        for i, (t, c) in enumerate(tmpl.most_common(), 1):
            acc += c
            if acc >= .9 * len(forms):
                k90 = i
                break
        print('%-18s rows %5d   templates %4d (90%% of forms in %d)' % (name, len(rows), len(tmpl), k90))
    if verbose:
        kw = steps[-1][1]
        tmpl = collections.Counter(structure(f, o, **kw) for r, f, o in forms)
        examples = {}
        for r, f, o in forms:
            examples.setdefault(structure(f, o, **kw), r['iclass'])
        print('\ntemplates after all folds (forms, example):')
        for t, c in tmpl.most_common(40):
            space, modrm, digit, rm, imm, apx, ops = t
            print('%5d  %-6s %-2s %-5s %-12s %-4s %-50s %s' % (
                c, space, modrm, 'digit' if digit else '', ','.join(imm), 'apx' if apx else '',
                ' '.join('%s:%s' % x for x in ops), examples[t]))
        other = collections.Counter(t for r, f, o in forms for t in f['other'])
        print('\nresidual pattern tokens (per-row constraints):')
        print(' '.join('%s:%d' % kv for kv in other.most_common()))


if __name__ == '__main__':
    main()
