; EXACT_MATCH mismatch, object B. Same name and size as
; linkfail_xmatch_a.asm, different content -> different CheckSum. See
; linkfail_xmatch_a.asm.

format MS64 COFF

section '.rdata$conf' data readable comdat exactmatch
public conf
conf dd 99
