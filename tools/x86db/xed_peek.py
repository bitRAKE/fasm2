"""xed_peek.py -- print XED encoder-table blocks: python tools/x86db/xed_peek.py ICLASS [text [count]]"""
import re, sys
F = r'C:\git\_Intel\xed\obj-wkit\dgen\all-enc-instructions.txt'
iclass = sys.argv[1]
needle = sys.argv[2] if len(sys.argv) > 2 else ''
limit = int(sys.argv[3]) if len(sys.argv) > 3 else 1
text = open(F, encoding='utf-8').read()
shown = 0
for block in re.findall(r'\{\n(.*?)\n\}', text, re.S):
    m = re.search(r'^ICLASS\s*:\s*(\S+)', block, re.M)
    if m and m.group(1) == iclass and needle in block:
        print(block.strip(), '\n----')
        shown += 1
        if shown >= limit:
            break
