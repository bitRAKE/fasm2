; PUBLIC declared before its COMDAT section exists. The external symbol
; would precede the section symbol in the table, which MSVC link rejects
; as a corrupt object (LNK1143) - the assembler errors at the SECTION
; statement instead.

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
