; EXACT_MATCH dedup, object B: byte-for-byte identical COMDAT content to
; optref_xmatch_a.asm, so the linker deduplicates it silently.

format MS64 COFF

section '.rdata$shared' data readable comdat exactmatch
public shared
shared dd 42
