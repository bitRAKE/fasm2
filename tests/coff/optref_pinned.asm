; Non-COMDAT .pdata that references a COMDAT function.
; The relocation is a hard reference, so /OPT:REF can never discard
; dropped_fn: exception metadata pins the code it describes unless the
; .pdata entry itself lives in an (associative) COMDAT section.

format MS64 COFF

section '.text$drop' code readable executable comdat
public dropped_fn
dropped_fn:
	mov	eax,1000
	ret
.end:

section '.pdata' data readable
	dd RVA dropped_fn
	dd RVA dropped_fn.end
	dd RVA unwind_drop

section '.xdata' data readable
unwind_drop:
	db 1,0,0,0

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	mov	eax,42
	ret
