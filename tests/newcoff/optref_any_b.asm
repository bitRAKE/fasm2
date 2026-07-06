; COMDAT ANY, object B: same symbol as optref_any_a.asm, identical semantics.

format MS64 NEWCOFF

section '.text$any' code readable executable comdat any
public any_fn
any_fn:
	mov	eax,42
	ret
