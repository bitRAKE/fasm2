; bf16.inc -- bfloat16 conversion and dot product, VEX and EVEX forms
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	AVX512_BF16	7.1:EAX[5]		vcvtne2ps2bf16 vdpbf16ps vcvtneps2bf16,
;						EVEX (xmm/ymm also need AVX512VL)
;	AVX_NE_CONVERT	7.1:EDX[5]		vcvtneps2bf16, VEX (xmm, ymm source);
;						vbcstnebf162ps vbcstnesh2ps
;						vcvtnee{bf16,ph}2ps vcvtneo{bf16,ph}2ps
;
; VEX forms also need AVX. Valid in 16-, 32- and 64-bit code.
;
; vcvtneps2bf16 exists in both encodings; with both features the shorter
; VEX form is used unless an EVEX-only operand is present or {evex} is
; given. It halves the vector: a memory source needs an explicit size when
; the destination is xmm (it could be 128 or 256 bits).

; EVEX only: dest{k}{z}, src, src2/m/m32bcst
iterate <instr,vex_mpw,opcode>, vcvtne2ps2bf16,VEX_F2_0F38_W0,72h, vdpbf16ps,VEX_F3_0F38_W0,52h

	calminstruction instr? dest*,src*,src2*
		call	x86.require.AVX512_BF16
		xcall	AVX_512.calm_basic_instruction_bcst, (vex_mpw),(EVEX_REQUIRED+EVEX_VL),(opcode),(4),dest,src,src2
	end calminstruction

end iterate

; VCVTNEPS2BF16 dest{k}{z}, src/m/m32bcst -- packed single to packed bfloat16
calminstruction vcvtneps2bf16? dest*,src*
	local	evex_f
	check	x86.ext and x86.AVX_NE_CONVERT.ext
	jyes	vex_available
	check	x86.ext and x86.AVX512_BF16.ext
	jyes	evex_only
	err	'AVX_NE_CONVERT or AVX512_BF16 required'
	exit
    evex_only:
	compute evex_f, EVEX_REQUIRED+EVEX_VL
	jump	operands
    vex_available:
	compute evex_f, EVEX_AS_VEX+EVEX_VL
	check	x86.ext and x86.AVX512_BF16.ext
	jyes	operands
	compute evex_f, EVEX_FORBIDDEN
    operands:
	call	AVX_512.parse_k1z_operand@dest, dest
	xcall	AVX_512.parse_bcst_operand@src, src,(4)
	check	@dest.type = 'mmreg' & (@src.type = 'mem' | @src.type = 'mmreg')
	jyes	sized
	err	'invalid combination of operands'
	exit
    sized:
	check	@src.size
	jyes	size_known
	check	@dest.size = 16
	jno	full_vector
	err	'operand size not specified'
	exit
    full_vector:
	compute @src.size, 64
    size_known:
	check	(@src.size shr 1 - 1) and not 15 + 16 <> @dest.size | @src.size > 64
	jno	encode
	err	'invalid operand size'
	exit
    encode:
	xcall	AVX_512.store_instruction@src, @src.size,(VEX_F3_0F38_W0),evex_f,(72h),@dest.mask,@dest.rm
end calminstruction

; AVX-NE-CONVERT loads: xmm/ymm destination, memory source only
;	vbcstne*: broadcast one 16-bit element (m16)
;	vcvtnee* / vcvtneo*: convert the even / odd 16-bit elements of a
;	vector the size of the destination
iterate <instr,vex_mpw,opcode,msize>, \
	vbcstnebf162ps,VEX_F3_0F38_W0,0B1h,2, \
	vbcstnesh2ps,  VEX_66_0F38_W0,0B1h,2, \
	vcvtneebf162ps,VEX_F3_0F38_W0,0B0h,0, \
	vcvtneeph2ps,  VEX_66_0F38_W0,0B0h,0, \
	vcvtneobf162ps,VEX_F2_0F38_W0,0B0h,0, \
	vcvtneoph2ps,  VEX_0F38_W0,   0B0h,0

	calminstruction instr? dest*,src*
		local	size
		call	x86.require.AVX_NE_CONVERT
		call	AVX_512.parse_operand@dest, dest
		call	AVX_512.parse_operand@src, src
		check	@dest.type = 'mmreg' & @src.type = 'mem'
		jyes	operands_ok
		err	'invalid combination of operands'
		exit
	    operands_ok:
		compute size, msize
		check	size
		jyes	size_set
		compute size, @dest.size
	    size_set:
		check	@src.size and not size | @dest.size > 32
		jno	encode
		err	'invalid operand size'
		exit
	    encode:
		xcall	AVX.store_instruction@src, @dest.size,(vex_mpw),(opcode),@dest.rm
	end calminstruction

end iterate
