; EXACT_MATCH COMDAT baseline. The section's aux CheckSum carries a
; CRC-32 of its raw data (seed 0, no final inversion - the parameters
; MSVC/lld use, matching a clang-cl-produced object byte for byte).
; A single object links and runs like any other COMDAT; the checksum
; only matters when a same-named section appears in another object
; (see optref_xmatch_* and linkfail_xmatch_*).

format MS64 COFF

section '.rdata$xm' data readable comdat exactmatch
public value
value dd 21

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	mov	eax,[value]
	add	eax,eax
	ret
