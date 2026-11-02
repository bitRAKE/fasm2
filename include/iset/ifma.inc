; ifma.inc -- 52-bit integer multiply-add, VEX and EVEX forms
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	AVX_IFMA	7.1:EAX[23]		vpmadd52luq vpmadd52huq, VEX (xmm, ymm)
;	AVX512_IFMA	7.0:EBX[21]		vpmadd52luq vpmadd52huq, EVEX (masks,
;						broadcast, zmm, xmm16+; xmm/ymm also
;						need AVX512VL)
;
; VEX forms also need AVX (AVX2 for ymm). Valid in 16-, 32- and 64-bit code.
;
; With both features enabled the shorter VEX form is used unless an
; EVEX-only operand is present or {evex} is given. With only AVX_IFMA,
; EVEX-only operands are refused; with only AVX512_IFMA, every form is EVEX.

iterate <instr,opcode>, vpmadd52luq,0B4h, vpmadd52huq,0B5h

	calminstruction instr? dest*,src*,src2*
		local	evex_f
		check	x86.ext and x86.AVX_IFMA.ext
		jyes	vex_available
		check	x86.ext and x86.AVX512_IFMA.ext
		jyes	evex_only
		err	'AVX_IFMA or AVX512_IFMA required'
		exit
	    evex_only:
		compute evex_f, EVEX_REQUIRED+EVEX_VL
		jump	encode
	    vex_available:
		compute evex_f, EVEX_AS_VEX+EVEX_VL+EVEX_VL_AVX2
		check	x86.ext and x86.AVX512_IFMA.ext
		jyes	encode
		compute evex_f, EVEX_FORBIDDEN+EVEX_VL_AVX2
	    encode:
		xcall	AVX_512.calm_basic_instruction_bcst, (VEX_66_0F38_W1),evex_f,(opcode),(8),dest,src,src2
	end calminstruction

end iterate
