; Uninitialized (BSS-style) COMDAT section.

format MS64 COFF

section '.bss$scratch' data readable writeable comdat
public scratch
scratch rq 16

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	mov	qword [scratch],42
	mov	eax,dword [scratch]
	ret
