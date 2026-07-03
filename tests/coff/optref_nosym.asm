; COMDAT sections with no PUBLIC at all. The format synthesizes a static
; COMDAT symbol (named after the section) at the end of the symbol table,
; so the object stays valid and /OPT:REF can discard each section
; individually: helper survives (referenced), orphan is dropped.

format MS64 COFF

section '.text$help' code readable executable comdat
helper:				; not public
	mov	eax,42
	ret

section '.rdata$orphan' data readable comdat
orphan	dd 0DEADh		; not public, never referenced

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	sub	rsp,40
	call	helper
	add	rsp,40
	ret
