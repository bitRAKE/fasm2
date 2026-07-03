; EXACT_MATCH dedup, object A: defines an exactmatch COMDAT whose raw
; content is identical to optref_xmatch_b.asm. Same name, same size,
; equal CheckSum -> the linker folds the two into one. Exit 42 proves
; the surviving copy is used.

format MS64 COFF

section '.rdata$shared' data readable comdat exactmatch
public shared
shared dd 42

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	mov	eax,[shared]
	ret
