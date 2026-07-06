; weak_c: calls through the weak name; the linker resolves it to the tag
format MS64 NEWCOFF
extrn maybe_get
extrn '__imp_ExitProcess' as ExitProcess:qword
section '.text$start' code readable executable comdat align 16
public mainCRTStartup
mainCRTStartup:
	sub	rsp, 40
	call	maybe_get
	mov	ecx, eax
	call	[ExitProcess]
