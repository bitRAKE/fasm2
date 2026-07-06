; ovfl: more than 65535 relocations in one section exercises
; IMAGE_SCN_LNK_NRELOC_OVFL (real count+1 in an extra first relocation)
format MS64 NEWCOFF
extrn '__imp_ExitProcess' as ExitProcess:qword

section '.rdata$t' data readable comdat exactmatch align 16
public tab
tab db '0123456789abcdef'

section '.data' data readable writeable
ptrs:
repeat 65600
	dq tab
end repeat

section '.text$start' code readable executable comdat align 16
public mainCRTStartup
mainCRTStartup:
	sub	rsp, 40
	mov	rax, [ptrs]		; first slot
	mov	rdx, [ptrs + 65599*8]	; last slot - same folded tab
	movzx	ecx, byte [rax + 10]	; 'a' = 97
	cmp	rax, rdx
	je	.ok
	mov	ecx, 1
    .ok:
	call	[ExitProcess]
