; COMDAT sections that export nothing: a `public static` symbol after the
; section statement provides the required COMDAT symbol while keeping the
; name internal. /OPT:REF still discards the unreferenced orphan data.

format MS64 COFF

section '.text$help' code readable executable comdat
public static helper
helper:
	mov	eax,42
	ret

section '.rdata$orphan' data readable comdat
public static orphan
orphan	dd 0DEADh

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	sub	rsp,40
	call	helper
	add	rsp,40
	ret
