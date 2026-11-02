; sha512.inc -- SHA-512 message schedule and rounds (VEX)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting	CPUID bit		instructions
;	SHA512	7.1:EAX[0]		vsha512msg1 vsha512msg2 vsha512rnds2
;
; Also needs AVX (1:ECX[28]). Valid in 16-, 32- and 64-bit code.
; Register operands only; ymm0-15 / xmm0-15 (VEX, no EVEX form).
; Not related to sha.inc (SHA-1/SHA-256, SSE encoding, gate SHA).

; VSHA512MSG1 ymm1, xmm2
calminstruction vsha512msg1? dest*,src*
	call	x86.require.SHA512
	call	AVX_512.parse_operand@dest, dest
	call	AVX_512.parse_operand@src, src
	check	@dest.type = 'mmreg' & @dest.size = 32 & @src.type = 'mmreg' & @src.size = 16
	jyes	encode
	err	'invalid combination of operands'
	exit
    encode:
	xcall	AVX.store_instruction@src, (32),(VEX_F2_0F38_W0),(0CCh),@dest.rm
end calminstruction

; VSHA512MSG2 ymm1, ymm2
calminstruction vsha512msg2? dest*,src*
	call	x86.require.SHA512
	call	AVX_512.parse_operand@dest, dest
	call	AVX_512.parse_operand@src, src
	check	@dest.type = 'mmreg' & @dest.size = 32 & @src.type = 'mmreg' & @src.size = 32
	jyes	encode
	err	'invalid combination of operands'
	exit
    encode:
	xcall	AVX.store_instruction@src, (32),(VEX_F2_0F38_W0),(0CDh),@dest.rm
end calminstruction

; VSHA512RNDS2 ymm1, ymm2, xmm3
calminstruction vsha512rnds2? dest*,src*,src2*
	call	x86.require.SHA512
	call	AVX_512.parse_operand@dest, dest
	call	AVX_512.parse_operand@src, src
	call	AVX_512.parse_operand@src2, src2
	check	@dest.type = 'mmreg' & @dest.size = 32 & @src.type = 'mmreg' & @src.size = 32 \
		& @src2.type = 'mmreg' & @src2.size = 16
	jyes	encode
	err	'invalid combination of operands'
	exit
    encode:
	xcall	AVX.store_instruction@src2, (32),(VEX_F2_0F38_W0),(0CBh),@dest.rm,@src.rm
end calminstruction
