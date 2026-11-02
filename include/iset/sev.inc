; sev.inc -- AMD SEV-SNP instruction encodings (APM vol.3)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	SEV_SNP		8000_001Fh:EAX[4]	pvalidate rmpadjust rmpupdate psmash
;	RMPQUERY	8000_001Fh:EAX[6]	rmpquery
;
; pvalidate is valid in 32- and 64-bit code; the others in 64-bit code only.
; vmgexit (SEV-ES) is in svm.inc, next to vmmcall, whose opcode it shares.
;
; All operands are implicit and optional. When written they must be the
; architectural registers; a rAX that is an address (pvalidate) may be
; narrower than the mode, which selects the address size (67h).
;
; Shares 0F 01 FD/FE/FF with rdpru, invlpgb and tlbsync (amd.inc), which
; have no prefix; the F2/F3 prefix selects these.

; PVALIDATE rAX, ECX, EDX -- validate or rescind a guest page
calminstruction pvalidate? addr,size,validate
	call	x86.require.SEV_SNP
	xcall	x86.implicit_operand, size, <ecx>
	xcall	x86.implicit_operand, validate, <edx>
	call	x86.address_operand, addr
	emit	4, 0xFF010FF2
end calminstruction

; RMPADJUST RAX, RCX, RDX -- adjust RMP permissions of a guest page
calminstruction rmpadjust? page,attributes,size
	call	x86.require.SEV_SNP
	call	x86.require.bits64
	xcall	x86.implicit_operand, page, <rax>
	xcall	x86.implicit_operand, attributes, <rcx>
	xcall	x86.implicit_operand, size, <rdx>
	emit	4, 0xFE010FF3
end calminstruction

; RMPUPDATE RAX, RCX -- write an RMP entry (hypervisor)
calminstruction rmpupdate? page,entry
	call	x86.require.SEV_SNP
	call	x86.require.bits64
	xcall	x86.implicit_operand, page, <rax>
	xcall	x86.implicit_operand, entry, <rcx>
	emit	4, 0xFE010FF2
end calminstruction

; PSMASH RAX -- split a 2MB RMP entry into 4KB entries (hypervisor)
calminstruction psmash? page
	call	x86.require.SEV_SNP
	call	x86.require.bits64
	xcall	x86.implicit_operand, page, <rax>
	emit	4, 0xFF010FF3
end calminstruction

; RMPQUERY -- no operand syntax is defined (RAX in, RCX/RDX out)
calminstruction rmpquery?
	call	x86.require.RMPQUERY
	call	x86.require.bits64
	emit	4, 0xFD010FF3
end calminstruction
