; PUBLIC declared before its COMDAT section exists.
; Symbol table order becomes: external symbol, then section symbol+aux.
; PE/COFF spec expects the section symbol to precede the COMDAT symbol.

format MS64 COFF

public kept_fn
public mainCRTStartup

section '.text$kept' code readable executable comdat
kept_fn:
	mov	eax,42
	ret

section '.text$main' code readable executable
mainCRTStartup:
	sub	rsp,40
	call	kept_fn
	add	rsp,40
	ret
