; COMDAT section with no external (public) symbol at all.
; helper is referenced only through the section symbol (section+offset reloc).
; Per spec a COMDAT with selection != ASSOCIATIVE is keyed by "the COMDAT
; symbol" = the symbol immediately after the aux record; here there is none.

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
