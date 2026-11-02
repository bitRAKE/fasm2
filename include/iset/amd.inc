; amd.inc -- AMD single-instruction system extensions (APM vol.3)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	MONITORX	8000_0001h:ECX[29]	monitorx mwaitx
;	CLZERO		8000_0008h:EBX[0]	clzero
;	INVLPGB		8000_0008h:EBX[3]	invlpgb tlbsync
;	RDPRU		8000_0008h:EBX[4]	rdpru
;	MCOMMIT		8000_0008h:EBX[8]	mcommit
;
; Valid in 16-, 32- and 64-bit code.
;
; All operands are implicit and optional. When written they must be the
; architectural registers; a rAX that is an address (monitorx, clzero,
; invlpgb) may be narrower than the mode, which selects the address size
; (67h), never the operand size.
;
; Shares 0F 01 FA..FF with mcommit (F3 0F 01 FA) and the SEV-SNP
; instructions in sev.inc (F2/F3 0F 01 FD..FF); the prefix tells them apart.

; MONITORX rAX, ECX, EDX -- arm address monitoring (any privilege level)
calminstruction monitorx? addr,extensions,hints
	call	x86.require.MONITORX
	xcall	x86.implicit_operand, extensions, (1), (4)
	xcall	x86.implicit_operand, hints, (2), (4)
	call	x86.address_operand, addr
	emit	3, 0xFA010F
end calminstruction

; MWAITX EAX, ECX -- wait on the monitored address (EBX holds the timer)
calminstruction mwaitx? hints,extensions
	call	x86.require.MONITORX
	xcall	x86.implicit_operand, hints, (0), (4)
	xcall	x86.implicit_operand, extensions, (1), (4)
	emit	3, 0xFB010F
end calminstruction

; CLZERO rAX -- zero the cache line containing rAX
calminstruction clzero? addr
	call	x86.require.CLZERO
	call	x86.address_operand, addr
	emit	3, 0xFC010F
end calminstruction

; INVLPGB rAX, EDX, ECX -- broadcast TLB invalidation
calminstruction invlpgb? addr,asid,count
	call	x86.require.INVLPGB
	xcall	x86.implicit_operand, asid, (2), (4)
	xcall	x86.implicit_operand, count, (1), (4)
	call	x86.address_operand, addr
	emit	3, 0xFE010F
end calminstruction

iterate <instr,opcode,feature>, \
	tlbsync,0xFF010F,INVLPGB, \
	rdpru,  0xFD010F,RDPRU		; RDPRU: ECX selects the register, EDX:EAX receives it

	calminstruction instr?
		call	x86.require.feature
		emit	3, opcode
	end calminstruction

end iterate

; MCOMMIT -- commit stores to persistent memory
calminstruction mcommit?
	call	x86.require.MCOMMIT
	emit	4, 0xFA010FF3
end calminstruction
