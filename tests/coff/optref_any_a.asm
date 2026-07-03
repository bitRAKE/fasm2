; COMDAT ANY, object A: duplicate definition is allowed, linker picks one.

format MS64 COFF

section '.text$any' code readable executable comdat any
public any_fn
any_fn:
	mov	eax,42
	ret

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	sub	rsp,40
	call	any_fn
	add	rsp,40
	ret
