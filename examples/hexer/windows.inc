; code/data model

format MS64 COFF
include 'win64a.inc'

; Keep proc bodies stable across invoke/fastcall calls.
prologue@proc	equ static_rsp_prologue
epilogue@proc	equ static_rsp_epilogue
close@proc	equ static_rsp_close procname
