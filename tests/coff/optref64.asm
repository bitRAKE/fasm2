; /OPT:REF smoke test (MS64 COFF)
; kept_fn is referenced from entry; dropped_fn is not.
; With /OPT:REF the linker must discard the COMDAT sections of
; dropped_fn and dropped_data, keep kept_fn and kept_data.
; Exit code proves data flow: 2*21 = 42.

format MS64 COFF

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
public mainCRTStartup
mainCRTStartup:
	sub	rsp,40
	call	kept_fn
	add	rsp,40
	ret
