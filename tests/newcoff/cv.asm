; cv: CodeView line information and S_REGREL32 symbols - cvline markers
; become '.debug$S' sections synthesized in POSTPONE (associative to
; the COMDAT code), and proc locals are harvested without manual cvlocal.
NEWCOFF.DEBUG := 1
format MS64 NEWCOFF
include 'win64a.inc'
extrn '__imp_ExitProcess' as ExitProcess:qword

prologue@proc	equ newcoff_debug_prologue
epilogue@proc	equ static_rsp_epilogue
close@proc	equ newcoff_debug_close
newcoff_debug_procs

section '.text$start' code readable executable comdat align 16
public mainCRTStartup
mainCRTStartup:
cvproc mainCRTStartup
	cvline
	sub	rsp, 40
cvframe 40
	cvline
	mov	ecx, 96
	call	debug_locals
cvline
	mov	ecx, eax
	call	[ExitProcess]
cvendp

proc debug_locals seed
	locals
		saved_seed	dq ?
		exit_code	dd ?
	endl
	mov	[saved_seed], rcx
	mov	dword [exit_code], 97
	mov	eax, [exit_code]
	ret
endp

section '.text$other' code readable executable comdat align 16
public helper
helper:
	cvline
	ret
