; tbm.inc -- AMD trailing bit manipulation
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting	CPUID bit		instructions
;	TBM	8000_0001h:ECX[21]	blcfill blci blcic blcmsk blcs blsfill blsic
;					t1mskc tzmsk, and bextr r, r/m, imm32
;
; Valid in 16-, 32- and 64-bit code; 64-bit operands in 64-bit code only.
; XOP-encoded general-purpose instructions: no AVX needed.
;
; dest, src -- XOP.09 01/02 /digit: dest in XOP.vvvv, src in ModRM.rm,
; ModRM.reg selects the operation; XOP.W follows the operand size.
; bextr's immediate form shares its mnemonic with BMI1, so it is defined
; in place in bmi.inc (an immediate third operand selects TBM).

iterate <instr,opcode,postbyte>, \
	blcfill,1,1, blsfill,1,2, blcs,1,3, tzmsk,1,4, blcic,1,5, blsic,1,6, t1mskc,1,7, \
	blcmsk,2,1, blci,2,6

	calminstruction instr? dest*,src*
		call	x86.require.TBM
		call	x86.parse_operand@dest, dest
		call	x86.parse_operand@src, src
		check	@dest.type = 'reg' & (@src.type = 'reg' | @src.type = 'mem')
		jyes	operands_ok
		err	'invalid combination of operands'
		exit
	    operands_ok:
		check	@dest.size = 4 | @dest.size = 8
		jyes	register_ok
		err	'invalid operand size'
		exit
	    register_ok:
		check	@src.size and not @dest.size
		jno	sizes_ok
		err	'operand sizes do not match'
		exit
	    sizes_ok:
		check	@dest.size = 8
		jyes	wide
		xcall	AVX.store_gpr_instruction@src, (16),(XOP_09_W0),(opcode),(postbyte),@dest.rm
		exit
	    wide:
		call	x86.require.bits64
		xcall	AVX.store_gpr_instruction@src, (16),(XOP_09_W1),(opcode),(postbyte),@dest.rm
	end calminstruction

end iterate
