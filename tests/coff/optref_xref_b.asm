; EXACT_MATCH fold with intra-object data references, object B.
; See optref_xref_a.asm. Provides the entry point.

format MS64 COFF

section '.rdata$xtab' data readable comdat exactmatch align 16
public xtab
xtab db '0123456789abcdef'

section '.text$xref_b' code readable executable comdat
public xref_b
xref_b:
	lea	rax, [xtab]
	movzx	eax, byte [rax + rcx]
	ret

section '.text$main' code readable executable
public mainCRTStartup
extrn xref_a
mainCRTStartup:
	sub	rsp, 40
	mov	ecx, 10
	call	xref_a			; xtab[10] = 'a'
	cmp	al, 'a'
	jne	.bad
	mov	ecx, 11
	call	xref_b			; xtab[11] = 'b'
	cmp	al, 'b'
	jne	.bad
	mov	eax, 42
	add	rsp, 40
	ret
    .bad:
	mov	eax, 7
	add	rsp, 40
	ret
