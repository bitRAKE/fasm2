; Duplicate COMDAT, object B. See linkfail_dup_a.asm.

format MS64 COFF

section '.text$dup' code readable executable comdat
public dup_fn
dup_fn:
	mov	eax,2
	ret
