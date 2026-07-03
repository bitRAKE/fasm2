; EXACT_MATCH matches sections across objects by their external COMDAT
; symbol name (plus size and checksum), so - like any/samesize/largest -
; it requires a PUBLIC. Without one there is nothing to match on, so the
; assembler rejects it rather than synthesizing a static symbol.

format MS64 COFF

section '.rdata$np' data readable comdat exactmatch
npool dd 42

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	mov	eax,[npool]
	ret
