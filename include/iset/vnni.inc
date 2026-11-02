; vnni.inc -- vector neural network instructions, VEX and EVEX forms
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	AVX_VNNI	7.1:EAX[4]		vpdpbusd(s) vpdpwssd(s), VEX (xmm, ymm)
;	AVX512_VNNI	7.0:ECX[11]		vpdpbusd(s) vpdpwssd(s), EVEX (masks,
;						broadcast, zmm, xmm16+; xmm/ymm also
;						need AVX512VL)
;	AVX_VNNI_INT8	7.1:EDX[4]		vpdpb{ss,su,uu}d(s), VEX only
;	AVX_VNNI_INT16	7.1:EDX[10]		vpdpw{su,us,uu}d(s), VEX only
;
; VEX forms also need AVX (AVX2 for ymm). Valid in 16-, 32- and 64-bit code.
;
; vpdpbusd and friends exist in both encodings. With both features enabled
; the shorter VEX form is used unless an EVEX-only operand is present or
; {evex} is given. With only AVX_VNNI, EVEX-only operands are refused; with
; only AVX512_VNNI, every form is EVEX.

iterate <instr,opcode>, vpdpbusd,50h, vpdpbusds,51h, vpdpwssd,52h, vpdpwssds,53h

	calminstruction instr? dest*,src*,src2*
		local	evex_f
		check	x86.ext and x86.AVX_VNNI.ext
		jyes	vex_available
		check	x86.ext and x86.AVX512_VNNI.ext
		jyes	evex_only
		err	'AVX_VNNI or AVX512_VNNI required'
		exit
	    evex_only:
		compute evex_f, EVEX_REQUIRED+EVEX_VL
		jump	encode
	    vex_available:
		compute evex_f, EVEX_AS_VEX+EVEX_VL+EVEX_VL_AVX2
		check	x86.ext and x86.AVX512_VNNI.ext
		jyes	encode
		compute evex_f, EVEX_FORBIDDEN+EVEX_VL_AVX2
	    encode:
		xcall	AVX_512.calm_basic_instruction_bcst, (VEX_66_0F38_W0),evex_f,(opcode),(4),dest,src,src2
	end calminstruction

end iterate

; AVX-VNNI-INT8 / AVX-VNNI-INT16: VEX only, W0, xmm or ymm
iterate <instr,vex_mpw,opcode,feature>, \
	vpdpbssd, VEX_F2_0F38_W0,50h,AVX_VNNI_INT8,  vpdpbssds,VEX_F2_0F38_W0,51h,AVX_VNNI_INT8, \
	vpdpbsud, VEX_F3_0F38_W0,50h,AVX_VNNI_INT8,  vpdpbsuds,VEX_F3_0F38_W0,51h,AVX_VNNI_INT8, \
	vpdpbuud, VEX_0F38_W0,   50h,AVX_VNNI_INT8,  vpdpbuuds,VEX_0F38_W0,   51h,AVX_VNNI_INT8, \
	vpdpwsud, VEX_F3_0F38_W0,0D2h,AVX_VNNI_INT16, vpdpwsuds,VEX_F3_0F38_W0,0D3h,AVX_VNNI_INT16, \
	vpdpwusd, VEX_66_0F38_W0,0D2h,AVX_VNNI_INT16, vpdpwusds,VEX_66_0F38_W0,0D3h,AVX_VNNI_INT16, \
	vpdpwuud, VEX_0F38_W0,   0D2h,AVX_VNNI_INT16, vpdpwuuds,VEX_0F38_W0,   0D3h,AVX_VNNI_INT16

	calminstruction instr? dest*,src*,src2*
		call	x86.require.feature
		xcall	AVX_512.calm_basic_instruction_bcst, (vex_mpw),(EVEX_FORBIDDEN+EVEX_VL_AVX2),(opcode),(0),dest,src,src2
	end calminstruction

end iterate
