; code/data model

include '..\newcoff.inc'
format MS64 NEWCOFF
include 'win64a.inc'

; Keep proc bodies stable across invoke/fastcall calls. With NEWCOFF.DEBUG
; the newcoffcv wrappers make PROC/ENDP mark themselves: procedure symbols,
; frame size and USES registers for the unwind codes, all captured.
if definite NEWCOFF.DEBUG & NEWCOFF.DEBUG
	prologue@proc	equ newcoff_debug_prologue
	close@proc	equ newcoff_debug_close
else
	prologue@proc	equ static_rsp_prologue
	close@proc	equ static_rsp_close procname
end if
epilogue@proc	equ static_rsp_epilogue
