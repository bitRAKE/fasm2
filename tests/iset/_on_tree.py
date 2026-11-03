"""_on_tree.py -- run tests and their FAIL cases with another fasm2 tree.

    python tests/iset/_on_tree.py TREE test.asm ...

Prints what the main block and each FAIL case do there, to show that a
test fails on the tree before its fix (each test here must).
"""
import os, re, subprocess, sys
tree = sys.argv[1]
for test in sys.argv[2:]:
    src = os.path.abspath(test)
    n = len(re.findall(r'^; FAIL \d+:', open(src, encoding='utf-8').read(), re.M))
    env = dict(os.environ, INCLUDE=os.path.join(tree, 'include'))
    for k in [None] + list(range(1, n + 1)):
        args = [os.path.join(tree, 'fasmg.exe'), '-e5', "-iInclude('fasm2.inc')"]
        if k:
            args.append('-iFAIL=%d' % k)
        p = subprocess.run(args + [src, os.devnull], capture_output=True, text=True, env=env)
        err = re.findall(r'Error: (.*)', p.stdout + p.stderr)
        print('%-22s %-7s %s' % (os.path.basename(test), 'FAIL %d' % k if k else 'main', err[0] if err else 'assembles'))
