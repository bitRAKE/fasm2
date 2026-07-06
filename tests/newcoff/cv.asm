; cv: CodeView line information - cvline markers become '.debug$S'
; sections synthesized in POSTPONE (associative to the COMDAT code)
format MS64 NEWCOFF
extrn '__imp_ExitProcess' as ExitProcess:qword

section '.text$start' code readable executable comdat align 16
public mainCRTStartup
mainCRTStartup:
cvproc mainCRTStartup
	cvline
	sub	rsp, 40
cvframe 40
	; a named rsp-relative slot: S_REGREL32 in the PDB (watch window)
	virtual at rsp + 32
		exitcode dq ?
	end virtual
cvlocal exitcode
	cvline
	mov	ecx, 97
	mov	[exitcode], rcx
	cvline
	mov	rcx, [exitcode]
	call	[ExitProcess]
cvendp

section '.text$other' code readable executable comdat align 16
public helper
helper:
	cvline
	ret
