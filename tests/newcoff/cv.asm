; cv: CodeView line information - cvline markers become '.debug$S'
; sections synthesized in POSTPONE (associative to the COMDAT code)
include 'newcoff.inc'
format MS64 NEWCOFF
extrn '__imp_ExitProcess' as ExitProcess:qword

section '.text$start' code readable executable comdat align 16
public mainCRTStartup
mainCRTStartup:
	cvline
	sub	rsp, 40
	cvline
	mov	ecx, 97
	cvline
	call	[ExitProcess]

section '.text$other' code readable executable comdat align 16
public helper
helper:
	cvline
	ret
