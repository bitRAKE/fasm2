; EXACT_MATCH needs the auxiliary CheckSum (CRC-32 of final section
; contents); the format does not compute it, and emitting 0 would make
; every same-named section "match". Explicitly unsupported.

format MS64 COFF

section '.rdata$pool' data readable comdat exactmatch
public pool
pool	dd 42

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	mov	eax,[pool]
	ret
