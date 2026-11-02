; xop.inc -- AMD XOP vector instructions (Bulldozer family)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting	CPUID bit		instructions
;	XOP	8000_0001h:ECX[11]	vpcmov vpperm vpcom* vpmacs* vpmadcs* vprot*
;				vpsha* vpshl* vphadd* vphsub* vfrcz* vpermil2p*
;
; Also needs AVX (XOP uses the VEX layout). Valid in 16-, 32- and 64-bit
; code. xmm/ymm 0-15 only; no masks, broadcast or EVEX.
;
; XOP maps 8/9 use an 8Fh lead byte instead of C4h; avx.inc emits it for
; any map >= 8 (8F with a ModRM.reg-style second byte below 8 is pop r/m).
; vpermil2ps/pd use VEX map 0F3A. Four-operand forms place one register in
; the immediate byte (is4); VEX.W selects which of the last two sources is
; in ModRM.rm, the only one that may be memory.

; dest, src1, src2, src3 [, low4]: src1 in VEX.vvvv; src2 or src3 in
; ModRM.rm (the memory one, if any) and the other in imm8[7:4].
;	sizes	allowed vector sizes as a mask: 16, 32 or 16+32
;	msize	memory operand size for scalar forms, 0 for the vector size
;	rr_w1	register-only forms use W1 (src3 in ModRM.rm), as llvm-mc does
calminstruction XOP.is4_instruction vex_mpw*,opcode*,sizes*,msize*,rr_w1*,low4*,dest*,src1*,src2*,src3*
	local	is4, mem, vex_mpw_w1
	call	AVX_512.parse_operand@dest, dest
	call	AVX_512.parse_operand@src, src1
	call	AVX_512.parse_operand@src2, src2
	call	AVX_512.parse_operand@aux, src3
	check	@dest.type = 'mmreg' & @src.type = 'mmreg' & @src.size = @dest.size & @dest.size and sizes
	jno	invalid
	compute mem, msize
	check	mem
	jyes	mem_size_set
	compute mem, @dest.size
    mem_size_set:
	check	@aux.type = 'mem'
	jyes	w1
	check	@aux.type = 'mmreg' & @aux.size = @dest.size & (@src2.type = 'mem' | @src2.type = 'mmreg')
	jno	invalid
	check	@src2.type = 'mmreg' & rr_w1
	jyes	w1
    w0:
	check	@aux.rm < 16
	jno	invalid
	check	@src2.type = 'mmreg' & @src2.size <> @dest.size
	jyes	invalid
	check	@src2.type = 'mem' & @src2.size and not mem
	jyes	size_mismatch
	compute is4, @aux.rm shl 4 or low4
	xcall	AVX.store_instruction@src2, @dest.size,vex_mpw,opcode,@dest.rm,@src.rm,(1),is4
	exit
    w1:
	check	@src2.type = 'mmreg' & @src2.size = @dest.size & @src2.rm < 16
	jno	invalid
	check	@aux.type = 'mmreg' & @aux.size <> @dest.size
	jyes	invalid
	check	@aux.type = 'mem' & @aux.size and not mem
	jyes	size_mismatch
	compute is4, @src2.rm shl 4 or low4
	compute vex_mpw_w1, vex_mpw or 8000h
	xcall	AVX.store_instruction@aux, @dest.size,vex_mpw_w1,opcode,@dest.rm,@src.rm,(1),is4
	exit
    size_mismatch:
	err	'operand sizes do not match'
	exit
    invalid:
	err	'invalid combination of operands'
end calminstruction

; VPCMOV x/ymm, VPPERM xmm: dest, src1, src2, src3 (is4)
iterate <instr,opcode,sizes>, vpcmov,0A2h,16+32, vpperm,0A3h,16

	calminstruction instr? dest*,src1*,src2*,src3*
		call	x86.require.XOP
		xcall	XOP.is4_instruction, (XOP_08_W0),(opcode),(sizes),(0),(0),(0),dest,src1,src2,src3
	end calminstruction

end iterate

; multiply-accumulate: dest, src1, src2/m128, src3 (is4, register only: W0)
iterate <instr,opcode>, vpmacssww,85h, vpmacsswd,86h, vpmacssdql,87h, vpmacssdd,8Eh, vpmacssdqh,8Fh, \
			vpmacsww,95h, vpmacswd,96h, vpmacsdql,97h, vpmacsdd,9Eh, vpmacsdqh,9Fh, \
			vpmadcsswd,0A6h, vpmadcswd,0B6h

	calminstruction instr? dest*,src1*,src2*,src3*
		call	x86.require.XOP
		call	AVX_512.parse_operand@aux, src3
		check	@aux.type = 'mmreg'
		jyes	encode
		err	'invalid combination of operands'
		exit
	    encode:
		xcall	XOP.is4_instruction, (XOP_08_W0),(opcode),(16),(0),(0),(0),dest,src1,src2,src3
	end calminstruction

end iterate

; VPCOMcc: dest, src1, src2/m128, imm8 (predicate); aliases vpcom<pred><type>
calminstruction XOP.compare opcode*,dest*,src1*,src2*,predicate*
	call	x86.require.XOP
	call	AVX_512.parse_operand@dest, dest
	call	AVX_512.parse_operand@src, src1
	call	AVX_512.parse_operand@src2, src2
	call	x86.parse_operand@aux, predicate
	check	@dest.type = 'mmreg' & @dest.size = 16 & @src.type = 'mmreg' & @src.size = 16 \
		& (@src2.type = 'mem' | (@src2.type = 'mmreg' & @src2.size = 16)) & @aux.type = 'imm'
	jyes	operands_ok
	err	'invalid combination of operands'
	exit
    operands_ok:
	check	@src2.size and not 16 | @aux.size and not 1
	jno	encode
	err	'invalid operand size'
	exit
    encode:
	xcall	AVX.store_instruction@src2, (16),(XOP_08_W0),opcode,@dest.rm,@src.rm,(1),@aux.imm
end calminstruction

iterate <type,opcode>, b,0CCh, w,0CDh, d,0CEh, q,0CFh, ub,0ECh, uw,0EDh, ud,0EEh, uq,0EFh

	calminstruction vpcom#type? dest*,src1*,src2*,predicate*
		xcall	XOP.compare, (opcode),dest,src1,src2,predicate
	end calminstruction

	iterate <pred,code>, lt,0, le,1, gt,2, ge,3, eq,4, neq,5, false,6, true,7

		calminstruction vpcom#pred#type? dest*,src1*,src2*
			xcall	XOP.compare, (opcode),dest,src1,src2,<code>
		end calminstruction

	end iterate

end iterate

; VPROT/VPSHA/VPSHL: dest, src, count -- count in a register or memory
; (VEX.W selects which of src and count is in ModRM.rm); VPROT also takes
; an immediate count
calminstruction XOP.shift reg_opcode*,imm_opcode*,dest*,src*,count*
	call	x86.require.XOP
	call	AVX_512.parse_operand@dest, dest
	call	AVX_512.parse_operand@src, src
	call	AVX_512.parse_operand@src2, count
	check	@dest.type = 'mmreg' & @dest.size = 16
	jno	invalid
	check	@src2.type = 'imm'
	jyes	immediate
	check	@src2.type = 'mem'
	jyes	count_in_rm
	check	@src2.type = 'mmreg' & @src2.size = 16 & (@src.type = 'mem' | (@src.type = 'mmreg' & @src.size = 16))
	jno	invalid
	check	@src.size and not 16
	jyes	size_mismatch
	xcall	AVX.store_instruction@src, (16),(XOP_09_W0),reg_opcode,@dest.rm,@src2.rm
	exit
    count_in_rm:
	check	@src.type = 'mmreg' & @src.size = 16
	jno	invalid
	check	@src2.size and not 16
	jyes	size_mismatch
	xcall	AVX.store_instruction@src2, (16),(XOP_09_W1),reg_opcode,@dest.rm,@src.rm
	exit
    immediate:
	check	imm_opcode
	jno	invalid
	check	@src.type = 'mem' | (@src.type = 'mmreg' & @src.size = 16)
	jno	invalid
	check	@src.size and not 16 | @src2.size and not 1
	jyes	size_mismatch
	xcall	AVX.store_instruction@src, (16),(XOP_08_W0),imm_opcode,@dest.rm,(0),(1),@src2.imm
	exit
    size_mismatch:
	err	'invalid operand size'
	exit
    invalid:
	err	'invalid combination of operands'
end calminstruction

iterate <instr,reg_opcode,imm_opcode>, \
	vprotb,90h,0C0h, vprotw,91h,0C1h, vprotd,92h,0C2h, vprotq,93h,0C3h, \
	vpshlb,94h,0, vpshlw,95h,0, vpshld,96h,0, vpshlq,97h,0, \
	vpshab,98h,0, vpshaw,99h,0, vpshad,9Ah,0, vpshaq,9Bh,0

	calminstruction instr? dest*,src*,count*
		xcall	XOP.shift, (reg_opcode),(imm_opcode),dest,src,count
	end calminstruction

end iterate

; two-operand: dest, src/mem (XOP map 9, W0)
;	sizes: allowed vector sizes; msize: memory size (0 = vector size)
iterate <instr,opcode,sizes,msize>, \
	vfrczps,80h,16+32,0, vfrczpd,81h,16+32,0, vfrczss,82h,16,4, vfrczsd,83h,16,8, \
	vphaddbw,0C1h,16,0, vphaddbd,0C2h,16,0, vphaddbq,0C3h,16,0, vphaddwd,0C6h,16,0, \
	vphaddwq,0C7h,16,0, vphadddq,0CBh,16,0, vphaddubw,0D1h,16,0, vphaddubd,0D2h,16,0, \
	vphaddubq,0D3h,16,0, vphadduwd,0D6h,16,0, vphadduwq,0D7h,16,0, vphaddudq,0DBh,16,0, \
	vphsubbw,0E1h,16,0, vphsubwd,0E2h,16,0, vphsubdq,0E3h,16,0

	calminstruction instr? dest*,src*
		local	mem
		call	x86.require.XOP
		call	AVX_512.parse_operand@dest, dest
		call	AVX_512.parse_operand@src, src
		check	@dest.type = 'mmreg' & @dest.size and (sizes) & (@src.type = 'mem' | (@src.type = 'mmreg' & @src.size = @dest.size))
		jyes	operands_ok
		err	'invalid combination of operands'
		exit
	    operands_ok:
		compute mem, msize
		check	mem
		jyes	mem_size_set
		compute mem, @dest.size
	    mem_size_set:
		check	@src.type = 'mem' & @src.size and not mem
		jno	encode
		err	'invalid operand size'
		exit
	    encode:
		xcall	AVX.store_instruction@src, @dest.size,(XOP_09_W0),(opcode),@dest.rm
	end calminstruction

end iterate

; VPERMIL2PS/PD x/ymm: dest, src1, src2, src3, imm (m2z, 0-3)
iterate <instr,opcode>, vpermil2ps,48h, vpermil2pd,49h

	calminstruction instr? dest*,src1*,src2*,src3*,selector*
		local	low4
		call	x86.require.XOP
		call	x86.parse_operand@src2, selector
		check	@src2.type = 'imm' & @src2.imm >= 0 & @src2.imm < 4
		jyes	selector_ok
		err	'invalid operand'
		exit
	    selector_ok:
		compute low4, @src2.imm
		xcall	XOP.is4_instruction, (VEX_66_0F3A_W0),(opcode),(16+32),(0),(0),low4,dest,src1,src2,src3
	end calminstruction

end iterate
