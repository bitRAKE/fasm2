; cv: CodeView line information and symbols - cvline markers become
; '.debug$S' sections synthesized in POSTPONE (associative to the COMDAT
; code), proc locals are harvested without manual cvlocal, and opt-in
; constants/data markers become named debugger symbols.
NEWCOFF.DEBUG := 1
format MS64 NEWCOFF
include 'win64a.inc'
extrn '__imp_ExitProcess' as ExitProcess:qword

CV_EXIT_CODE = 97
cvconst CV_EXIT_CODE, CV_T_UINT4

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
	mov	ecx, CV_EXIT_CODE - 1
	call	debug_locals
cvline
	add	eax, [cv_local_delta]
	cmp	eax, [cv_global_exit_code]
	cmove	eax, [cv_global_exit_code]
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

section '.data' data readable writeable align 4
cv_global_exit_code:
cvgdata cv_global_exit_code, CV_T_UINT4
	dd CV_EXIT_CODE
cv_local_delta:
cvldata cv_local_delta, CV_T_UINT4
	dd 0
