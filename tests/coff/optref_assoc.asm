; COMDAT ASSOCIATIVE: unwind metadata that follows its function.
; .pdata$drop/.xdata$drop are associated with dropped_fn's section, so
; /OPT:REF discards all three together; kept_fn's metadata survives.

format MS64 COFF

section '.text$kept' code readable executable comdat
public kept_fn
kept_fn:
	mov	eax,42
	ret
.end:

section '.text$drop' code readable executable comdat
public dropped_fn
dropped_fn:
	mov	eax,1000
	ret
.end:

section '.xdata$kept' data readable comdat associative kept_fn
unwind_kept:
	db 1,0,0,0

section '.pdata$kept' data readable comdat associative kept_fn
	dd RVA kept_fn
	dd RVA kept_fn.end
	dd RVA unwind_kept

section '.xdata$drop' data readable comdat associative dropped_fn
unwind_drop:
	db 1,0,0,0

section '.pdata$drop' data readable comdat associative dropped_fn
	dd RVA dropped_fn
	dd RVA dropped_fn.end
	dd RVA unwind_drop

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	sub	rsp,40
	call	kept_fn
	add	rsp,40
	ret
