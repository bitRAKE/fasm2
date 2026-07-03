; Duplicate COMDAT, object A. Section and public names identical to
; linkfail_dup_b.asm. Selection is NODUPLICATES (the default), so linking
; A+B must fail with a duplicate-symbol error.

format MS64 COFF

section '.text$dup' code readable executable comdat
public dup_fn
dup_fn:
	mov	eax,1
	ret

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	sub	rsp,40
	call	dup_fn
	add	rsp,40
	ret
