; msr.inc -- MSR list access, non-serializing write, user-mode MSR access
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	MSRLIST		7.1:EAX[27]		rdmsrlist wrmsrlist (64-bit code only)
;	WRMSRNS		7.1:EAX[19]		wrmsrns
;	USER_MSR	7.1:EDX[15]		urdmsr uwrmsr (64-bit code only)
;
; wrmsrns is valid in 16-, 32- and 64-bit code.
;
; urdmsr r64, r64	F2 0F 38 F8 /r	(ModRM.reg = destination)
; uwrmsr r64, r64	F3 0F 38 F8 /r	(ModRM.reg = MSR index, rm = value)
; urdmsr r64, imm32	VEX.128.F2.MAP7.W0 F8 /0 id
; uwrmsr imm32, r64	VEX.128.F3.MAP7.W0 F8 /0 id
; The register forms take registers only: F2 0F 38 F8 with a memory
; operand is enqcmd. The VEX forms are general-purpose instructions and do
; not need AVX. APX (APX_F): with r16-r31 or {evex} the register forms take
; EVEX map 4 (x86.store_instruction) and the immediate forms EVEX map 7.
; The MSR-IMM rdmsr/wrmsrns forms are not implemented.

iterate <instr,opcode>, rdmsrlist,0xC6010FF2, wrmsrlist,0xC6010FF3

	calminstruction instr?
		call	x86.require.MSRLIST
		call	x86.require.bits64
		emit	4, opcode
	end calminstruction

end iterate

calminstruction wrmsrns?
	call	x86.require.WRMSRNS
	emit	3, 0xC6010F
end calminstruction

; URDMSR r64, r64/imm32 -- read the MSR named by the source into the destination
; UWRMSR r64/imm32, r64 -- write the second operand to the MSR named by the first
iterate <instr,prefix,pp,order>, urdmsr,0F2h,7Bh,0, uwrmsr,0F3h,7Ah,1

	calminstruction instr? arg1*,arg2*
		local	msr, reg
		call	x86.require.USER_MSR
		call	x86.require.bits64
		call	x86.parse_operand@dest, arg1
		call	x86.parse_operand@src, arg2
		check	order
		jyes	write
		check	@dest.type = 'reg' & @dest.size = 8
		jno	invalid
		check	@src.type = 'reg' & @src.size = 8
		jyes	registers
		check	@src.type = 'imm'
		jno	invalid
		compute reg, @dest.rm
		compute msr, @src.imm
		jump	immediate
	    write:
		check	@src.type = 'reg' & @src.size = 8
		jno	invalid
		check	@dest.type = 'reg' & @dest.size = 8
		jyes	registers
		check	@dest.type = 'imm'
		jno	invalid
		compute reg, @src.rm
		compute msr, @dest.imm
		jump	immediate
	    registers:
		compute @src.opcode_prefix, prefix
		xcall	x86.store_instruction@src, <0Fh,38h,0F8h>,@dest.rm
		exit
	    immediate:
		check	msr >= 0 & msr < 1 shl 32
		jyes	range_ok
		err	'value out of range'
		exit
	    range_ok:
		check	reg and x86.EGPR | (x86.vexsize = 4 & x86.ext and x86.APX_F.ext)
		jno	encode
		call	x86.require.APX_F	; EVEX.128.F2/F3.MAP7.W0 F8 /0 id
		emit	1, 62h
		emit	1, (0F7h xor ((reg and 1000b) shl 2)) or ((reg and 10000b) shr 1)
		emit	1, 7Ch or (pp and 11b)
		emit	1, 08h
		emit	1, 0F8h
		emit	1, 0C0h + reg and 111b
		emit	4, msr
		exit
	    encode:
		emit	1, 0C4h
		emit	1, 0E7h and not ((reg and 1000b) shl 2)
		emit	1, pp
		emit	1, 0F8h
		emit	1, 0C0h + reg and 111b
		emit	4, msr
		exit
	    invalid:
		err	'invalid combination of operands'
	end calminstruction

end iterate
