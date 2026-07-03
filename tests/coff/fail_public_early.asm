; Even with a proper PUBLIC after the SECTION statement, another PUBLIC
; into the same COMDAT section placed BEFORE it puts a symbol carrying
; that section number in front of the section symbol - MSVC link rejects
; the object (LNK1143), so the assembler must too.

format MS64 COFF

public early_alias

section '.text$kept' code readable executable comdat
public kept_fn
kept_fn:
early_alias:
	mov	eax,42
	ret

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	sub	rsp,40
	call	kept_fn
	add	rsp,40
	ret
