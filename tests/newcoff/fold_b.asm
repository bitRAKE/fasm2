; fold_b: defines the same tab (folds with t2a's), compares both references
format MS64 NEWCOFF
extrn get_a
extrn '__imp_ExitProcess' as ExitProcess:qword
section '.rdata$tab' data readable comdat exactmatch align 16
public tab
tab db '0123456789abcdef'
section '.text$start' code readable executable comdat align 16
public mainCRTStartup
mainCRTStartup:
	sub	rsp, 40
	call	get_a			; other object's view of tab
	movzx	ecx, byte [tab + 10]	; our view - same survivor after fold
	cmp	eax, ecx
	je	.ok
	mov	ecx, 1
    .ok:
	call	[ExitProcess]
