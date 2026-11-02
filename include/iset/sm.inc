; sm.inc -- ShangMi SM3 hash and SM4 block cipher (VEX)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting	CPUID bit		instructions
;	SM3	7.1:EAX[1]		vsm3msg1 vsm3msg2 vsm3rnds2 (xmm)
;	SM4	7.1:EAX[2]		vsm4key4 vsm4rnds4 (xmm, ymm)
;
; Also needs AVX (1:ECX[28]). Valid in 16-, 32- and 64-bit code.
; VEX only: no masks, broadcast, zmm or xmm16+ (the EVEX SM4 forms belong
; to AVX10.2 and are not implemented).

; VSM3MSG1 / VSM3MSG2 xmm1, xmm2, xmm3/m128
iterate <instr,vex_mpw>, vsm3msg1,VEX_0F38_W0, vsm3msg2,VEX_66_0F38_W0

	calminstruction instr? dest*,src*,src2*
		call	x86.require.SM3
		call	AVX_512.parse_operand@dest, dest
		call	AVX_512.parse_operand@src, src
		call	AVX_512.parse_operand@src2, src2
		check	@dest.type = 'mmreg' & @dest.size = 16 & @src.type = 'mmreg' & @src.size = 16 \
			& (@src2.type = 'mmreg' | @src2.type = 'mem') & @src2.size and not 16 = 0
		jyes	encode
		err	'invalid combination of operands'
		exit
	    encode:
		xcall	AVX.store_instruction@src2, (16),(vex_mpw),(0DAh),@dest.rm,@src.rm
	end calminstruction

end iterate

; VSM3RNDS2 xmm1, xmm2, xmm3/m128, imm8
calminstruction vsm3rnds2? dest*,src*,src2*,round*
	call	x86.require.SM3
	call	AVX_512.parse_operand@dest, dest
	call	AVX_512.parse_operand@src, src
	call	AVX_512.parse_operand@src2, src2
	call	x86.parse_operand@aux, round
	check	@dest.type = 'mmreg' & @dest.size = 16 & @src.type = 'mmreg' & @src.size = 16 \
		& (@src2.type = 'mmreg' | @src2.type = 'mem') & @src2.size and not 16 = 0 & @aux.type = 'imm'
	jyes	operands_ok
	err	'invalid combination of operands'
	exit
    operands_ok:
	check	@aux.size and not 1
	jno	encode
	err	'invalid operand size'
	exit
    encode:
	xcall	AVX.store_instruction@src2, (16),(VEX_66_0F3A_W0),(0DEh),@dest.rm,@src.rm,(1),@aux.imm
end calminstruction

; VSM4KEY4 / VSM4RNDS4 x/ymm1, x/ymm2, x/ymm3/m
iterate <instr,vex_mpw>, vsm4key4,VEX_F3_0F38_W0, vsm4rnds4,VEX_F2_0F38_W0

	calminstruction instr? dest*,src*,src2*
		call	x86.require.SM4
		xcall	AVX_512.calm_basic_instruction_bcst, (vex_mpw),(EVEX_FORBIDDEN),(0DAh),(0),dest,src,src2
	end calminstruction

end iterate
