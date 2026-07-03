; 32-bit MS COFF /OPT:REF test: same shape as optref64.asm.

format MS COFF

; Feature record: bit 0 declares the object SafeSEH-compatible (it registers
; no SEH handlers). Without it, lld-link refuses x86 objects by default.
public static 1 as '@feat.00'

section '.text$kept' code readable executable comdat
public kept_fn
kept_fn:
	mov	eax,[kept_data]
	add	eax,eax
	ret

section '.text$drop' code readable executable comdat
public dropped_fn
dropped_fn:
	mov	eax,[dropped_data]
	imul	eax,eax
	ret

section '.rdata$kept' data readable comdat
public kept_data
kept_data dd 21

section '.rdata$drop' data readable comdat
public dropped_data
dropped_data dd 1000

section '.text$main' code readable executable
public mainCRTStartup as '_mainCRTStartup'	; cdecl decoration for x86 linkers
mainCRTStartup:
	call	kept_fn
	ret
