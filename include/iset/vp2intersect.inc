; vp2intersect.inc -- compute intersection into a pair of mask registers (EVEX)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting			CPUID bit	instructions
;	AVX512_VP2INTERSECT	7.0:EDX[8]	vp2intersectd vp2intersectq
;
; Also needs AVX512F; xmm/ymm sources also need AVX512VL. Valid in 16-, 32-
; and 64-bit code.
;
; The destination is an even mask register k0, k2, k4 or k6; the
; instruction writes it and the next one (k1, k3, k5, k7). No masking.

iterate <instr,vex_mpw,unit>, vp2intersectd,VEX_F2_0F38_W0,4, vp2intersectq,VEX_F2_0F38_W1,8

	calminstruction instr? dest*,src*,src2*
		call	x86.require.AVX512_VP2INTERSECT
		call	AVX_512.parse_operand@dest, dest
		call	AVX_512.parse_operand@src, src
		xcall	AVX_512.parse_bcst_operand@src2, src2,(unit)
		check	@dest.type = 'maskreg' & @src.type = 'mmreg' & (@src2.type = 'mem' | @src2.type = 'mmreg')
		jyes	operands_ok
		err	'invalid combination of operands'
		exit
	    operands_ok:
		check	@dest.rm and 1
		jno	pair_ok
		err	'even mask register expected'
		exit
	    pair_ok:
		check	@src2.size and not @src.size
		jno	encode
		err	'operand sizes do not match'
		exit
	    encode:
		xcall	AVX_512.store_instruction@src2, @src.size,(vex_mpw),(EVEX_REQUIRED+EVEX_VL),(68h),(0),@dest.rm,@src.rm
	end calminstruction

end iterate
