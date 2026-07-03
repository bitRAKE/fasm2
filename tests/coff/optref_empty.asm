; Zero-length COMDAT section: declared but no data emitted.

format MS64 COFF

section '.rdata$empty' data readable comdat
public empty_mark
empty_mark:

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	mov	eax,42
	ret
