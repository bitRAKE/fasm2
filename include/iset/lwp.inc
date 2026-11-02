; lwp.inc -- AMD lightweight profiling
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting	CPUID bit		instructions
;	LWP	8000_0001h:ECX[15]	llwpcb slwpcb lwpins lwpval
;
; Valid in 16-, 32- and 64-bit code; 64-bit registers in 64-bit code only.
; XOP-encoded general-purpose instructions: no AVX needed.
;
; llwpcb r / slwpcb r		XOP.09 12 /0 / /1, register in ModRM.rm
; lwpins r, r/m32, imm32	XOP.0A 12 /0 id, r in XOP.vvvv
; lwpval r, r/m32, imm32	XOP.0A 12 /1 id
; XOP.W follows the size of the r operand.

iterate <instr,postbyte>, llwpcb,0, slwpcb,1

	calminstruction instr? src*
		call	x86.require.LWP
		call	x86.parse_operand@src, src
		check	@src.type = 'reg' & (@src.size = 4 | @src.size = 8)
		jyes	operand_ok
		err	'invalid operand'
		exit
	    operand_ok:
		check	@src.size = 8
		jyes	wide
		xcall	AVX.store_gpr_instruction@src, (16),(XOP_09_W0),(12h),(postbyte)
		exit
	    wide:
		call	x86.require.bits64
		xcall	AVX.store_gpr_instruction@src, (16),(XOP_09_W1),(12h),(postbyte)
	end calminstruction

end iterate

iterate <instr,postbyte>, lwpins,0, lwpval,1

	calminstruction instr? dest*,src*,value*
		call	x86.require.LWP
		call	x86.parse_operand@dest, dest
		call	x86.parse_operand@src, src
		call	x86.parse_operand@aux, value
		check	@dest.type = 'reg' & (@src.type = 'reg' | @src.type = 'mem') & @aux.type = 'imm'
		jyes	operands_ok
		err	'invalid combination of operands'
		exit
	    operands_ok:
		check	(@dest.size = 4 | @dest.size = 8) & @src.size and not 4 = 0 & @aux.size and not 4 = 0
		jyes	sizes_ok
		err	'invalid operand size'
		exit
	    sizes_ok:
		check	@dest.size = 8
		jyes	wide
		xcall	AVX.store_gpr_instruction@src, (16),(XOP_0A_W0),(12h),(postbyte),@dest.rm,(4),@aux.imm
		exit
	    wide:
		call	x86.require.bits64
		xcall	AVX.store_gpr_instruction@src, (16),(XOP_0A_W1),(12h),(postbyte),@dest.rm,(4),@aux.imm
	end calminstruction

end iterate
