; EXACT_MATCH fold with intra-object data references, object A.
;
; Both objects define an identical `xtab` in an EXACT_MATCH COMDAT and each
; references it from its own code. The linker folds the two copies into one;
; the reference in whichever object's copy is discarded must rebind to the
; survivor. That only works if the reference relocates against the external
; symbol `xtab` rather than the static section symbol - which is what the MS
; COFF backend now emits for a reference into a section that has an external
; symbol at offset 0. Exit 42 proves both references resolved.

format MS64 COFF

section '.rdata$xtab' data readable comdat exactmatch align 16
public xtab
xtab db '0123456789abcdef'

section '.text$xref_a' code readable executable comdat
public xref_a
xref_a:
	lea	rax, [xtab]
	movzx	eax, byte [rax + rcx]	; xtab[cl]
	ret
