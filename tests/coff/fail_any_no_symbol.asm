; A COMDAT selection other than the default NODUPLICATES exists to match
; sections across objects by their (external) COMDAT symbol name. Without
; a PUBLIC there is no name to match on, so this must be rejected instead
; of synthesizing a meaningless static symbol.

format MS64 COFF

section '.rdata$pool' data readable comdat any
pool	dd 42

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	mov	eax,[pool]
	ret
