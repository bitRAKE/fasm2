"""x86db_stats.py -- how far XED's instruction forms coalesce.

    python tools/x86db/x86db_stats.py [build/xed.json] [-v]

Each XED record is split into
    parameters   what varies per instruction: opcode byte, ModRM.reg
                 digit, mnemonic, ISA set;
    shape        the encoding with those abstracted: space (legacy, VEX,
                 EVEX, XOP, ...), map, mandatory prefix, W, L, ModRM
                 form, immediates, mode and APX/EVEX fields;
    signature    the operand list with register numbers and widths kept
                 as XED's nonterminal types (GPRv_R, XMM_B3, MEM0:r:dq ...).
A template is a (shape, signature) pair. The number of templates, and
how few cover most forms, is the measure of how flat a generated table
can be.
"""
import os, re, sys, json, collections

here = os.path.dirname(os.path.abspath(__file__))

SPACES = {'VV1': 'VEX', 'EVV': 'EVEX', 'XOPV': 'XOP', 'KVV': 'KNC'}


def split(rec):
    """(params, shape tokens, operand signature)"""
    pat = rec['pattern']
    space = next((SPACES[t] for t in pat if t in SPACES), 'LEGACY')
    params, shape = {}, [space]
    seen_opcode = False
    for t in pat:
        if t in SPACES:
            continue
        if re.fullmatch(r'0x[0-9A-Fa-f]{1,2}', t):
            b = int(t, 16)
            if space == 'LEGACY' and not seen_opcode and b == 0x0F:
                shape.append('0F')		# escape: part of the map
                continue
            if space == 'LEGACY' and not seen_opcode and shape[-1] == '0F' and b in (0x38, 0x3A, 0x0F):
                shape.append('%02X' % b)
                continue
            if not seen_opcode:
                params['opcode'] = b
                shape.append('OP')
                seen_opcode = True
            else:
                shape.append('IB')		# a second literal: 3DNow suffix, fixed ModRM ...
                params.setdefault('extra', []).append(b)
            continue
        m = re.fullmatch(r'REG\[0b([01]{3})\]', t)
        if m:
            params['digit'] = int(m.group(1), 2)
            shape.append('REG[d]')
            continue
        m = re.fullmatch(r'RM\[0b([01]{3})\]', t)
        if m:
            params['rm'] = int(m.group(1), 2)
            shape.append('RM[d]')
            continue
        m = re.fullmatch(r'SRM\[0b([01]{3})\]', t)		# opcode + register (50+r ...)
        if m:
            params['srm'] = int(m.group(1), 2)
            shape.append('SRM[d]')
            continue
        shape.append(t)
    sig = []
    for o in rec['operands']:
        o = re.sub(r':TXT=\w+', '', o)
        sig.append(o)
    return params, tuple(shape), tuple(sig)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    verbose = '-v' in sys.argv
    src = args[0] if args else os.path.join(here, 'build', 'xed.json')
    recs = json.load(open(src))
    shapes, sigs, templates = collections.Counter(), collections.Counter(), collections.Counter()
    by_space = collections.Counter()
    tokens = collections.Counter()
    for r in recs:
        params, shape, sig = split(r)
        shapes[shape] += 1
        sigs[sig] += 1
        templates[(shape, sig)] += 1
        by_space[shape[0]] += 1
        tokens.update(set(shape))
    n = len(recs)

    def cover(counter, frac):
        acc = 0
        for i, (k, c) in enumerate(counter.most_common(), 1):
            acc += c
            if acc >= frac * n:
                return i
        return len(counter)

    print('%d forms (%d mnemonics, %d ISA sets)' % (n, len({r['iclass'] for r in recs}),
                                                   len({r['isa_set'] for r in recs})))
    print('by space: ' + ', '.join('%s %d' % kv for kv in by_space.most_common()))
    for name, c in (('shapes', shapes), ('operand signatures', sigs), ('templates (shape+signature)', templates)):
        print('%-28s %5d distinct; 50%% of forms in %d, 90%% in %d, 99%% in %d; used once: %d'
              % (name, len(c), cover(c, .5), cover(c, .9), cover(c, .99), sum(1 for v in c.values() if v == 1)))
    print('pattern vocabulary: %d distinct tokens' % len(tokens))
    if verbose:
        print('\nmost common shapes:')
        for s, c in shapes.most_common(25):
            print('%5d  %s' % (c, ' '.join(s)))
        print('\nmost common signatures:')
        for s, c in sigs.most_common(25):
            print('%5d  %s' % (c, ' '.join(s)))
        print('\ntokens:')
        print(' '.join('%s:%d' % kv for kv in sorted(tokens.items(), key=lambda kv: -kv[1])))


if __name__ == '__main__':
    main()
