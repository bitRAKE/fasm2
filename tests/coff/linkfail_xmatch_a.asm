; EXACT_MATCH mismatch, object A. Same COMDAT name and size as
; linkfail_xmatch_b.asm but DIFFERENT content, so the CheckSums differ
; and the linker must refuse to merge them (duplicate symbol / LNK2005),
; proving the checksum actually gates the match.

format MS64 COFF

section '.rdata$conf' data readable comdat exactmatch
public conf
conf dd 42

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	mov	eax,[conf]
	ret
