"""xed_extract.py -- XED's instruction tables as one flat record list.

    python tools/x86db/xed_extract.py [XED_OBJ_DIR] [-o OUT.json]

XED's build merges its datafiles (datafiles/*/*.txt, one directory per
extension) into dgen/all-enc-instructions.txt: one {...} block per
instruction, each with one or more PATTERN/OPERANDS(/IFORM) triples. This
reads that file and writes one record per triple:

    iclass, iform, extension, isa_set, category, cpl, attributes,
    exceptions, flags, pattern (token list), operands (token list)

idata.txt (iform -> iclass, category, extension, isa_set) fills an IFORM
the block leaves out. The output is the input to x86db_stats.py and,
later, to the generator; it is a build product, not checked in.
"""
import os, re, sys, json

DEFAULT_OBJ = os.path.join('C:' + os.sep, 'git', '_Intel', 'xed', 'obj-wkit')
FIELDS = ('ICLASS', 'UNAME', 'CPL', 'CATEGORY', 'EXTENSION', 'ISA_SET', 'ATTRIBUTES',
          'EXCEPTIONS', 'FLAGS', 'COMMENT', 'VERSION', 'REAL_OPCODE', 'DISASM', 'DISASM_INTEL',
          'DISASM_ATTSV')


def blocks(text):
    for body in re.findall(r'^\{\s*\n(.*?)^\}', text, re.S | re.M):
        yield body


def parse_block(body):
    head, forms, cur = {}, [], None
    for raw in body.splitlines():
        line = raw.split('#', 1)[0].rstrip()
        m = re.match(r'^\s*([A-Z_]+)\s*:\s*(.*)$', line)
        if not m:
            continue
        key, val = m.group(1), m.group(2).strip()
        if key == 'PATTERN':
            cur = {'pattern': val.split(), 'operands': [], 'iform': None}
            forms.append(cur)
        elif key == 'OPERANDS' and cur is not None:
            cur['operands'] = val.split()
        elif key == 'IFORM' and cur is not None:
            cur['iform'] = val or None
        elif key in FIELDS:
            head[key] = val
    return head, forms


def main():
    args = [a for a in sys.argv[1:]]
    out = None
    if '-o' in args:
        i = args.index('-o')
        out = args[i + 1]
        del args[i:i + 2]
    obj = args[0] if args else DEFAULT_OBJ
    text = open(os.path.join(obj, 'dgen', 'all-enc-instructions.txt'), encoding='utf-8').read()
    records = []
    for body in blocks(text):
        head, forms = parse_block(body)
        if 'ICLASS' not in head:
            continue
        for f in forms:
            records.append({
                'iclass': head['ICLASS'],
                'iform': f['iform'],
                'extension': head.get('EXTENSION'),
                'isa_set': head.get('ISA_SET'),
                'category': head.get('CATEGORY'),
                'cpl': head.get('CPL'),
                'attributes': head.get('ATTRIBUTES', '').split(),
                'exceptions': head.get('EXCEPTIONS'),
                'flags': head.get('FLAGS'),
                'real_opcode': head.get('REAL_OPCODE', 'Y'),
                'pattern': f['pattern'],
                'operands': f['operands'],
            })
    out = out or os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build', 'xed.json')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    json.dump(records, open(out, 'w'), indent=0)
    print('%d records, %d iclasses, %d iforms -> %s' % (
        len(records), len({r['iclass'] for r in records}),
        len({r['iform'] for r in records if r['iform']}), out))


if __name__ == '__main__':
    main()
