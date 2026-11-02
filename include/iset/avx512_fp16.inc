; avx512_fp16.inc -- AVX-512 half-precision (FP16) instructions
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	AVX512_FP16	7.0:EDX[23]		all ...ph / ...sh instructions here, vmovw
;
; Also needs AVX512F (the EVEX encoder checks it); xmm/ymm packed forms also
; need AVX512VL. EVEX only. Valid in 16-, 32- and 64-bit code.
;
; Encoding: EVEX maps 5 and 6 (new with FP16) and 0F3A, W0. Element size 2:
; broadcasts are {1to8}/{1to16}/{1to32} (the complex instructions broadcast
; a 32-bit pair), disp8 scales by the element or vector as in every AVX-512
; form. Scalar memory operands are 16-bit.
;
; This file follows avx.inc's single-precision definitions one for one and
; reuses its macro shape helpers (AVX_512.basic_instruction_bcst_er, ...),
; with unit 2 and the map-5/6 constants below. Rewriting those helpers in
; CALM is a separate step; new shapes here would be CALM.

VEX_MAP5_W0	= 101b
VEX_66_MAP5_W0	= 101b + 1 shl 8
VEX_F3_MAP5_W0	= 101b + 10b shl 8
VEX_F2_MAP5_W0	= 101b + 11b shl 8
VEX_MAP5_W1	= VEX_MAP5_W0 or 8000h
VEX_66_MAP5_W1	= VEX_66_MAP5_W0 or 8000h
VEX_F2_MAP5_W1	= VEX_F2_MAP5_W0 or 8000h
VEX_MAP6_W0	= 110b
VEX_66_MAP6_W0	= 110b + 1 shl 8
VEX_F3_MAP6_W0	= 110b + 10b shl 8
VEX_F2_MAP6_W0	= 110b + 11b shl 8

; --- arithmetic, map 5 ---------------------------------------------------

iterate <instr,opcode>, add,58h, mul,59h, sub,5Ch, div,5Eh

	macro v#instr#ph? dest*,src*,src2*&
		require AVX512_FP16
		AVX_512.basic_instruction_bcst_er VEX_MAP5_W0,EVEX_REQUIRED+EVEX_VL,opcode,2,dest,src,src2
	end macro

	macro v#instr#sh? dest*,src*,src2*&
		require AVX512_FP16
		AVX_512.basic_instruction_er VEX_F3_MAP5_W0,EVEX_REQUIRED,opcode,2,dest,src,src2
	end macro

end iterate

iterate <instr,opcode>, min,5Dh, max,5Fh

	macro v#instr#ph? dest*,src*,src2*&
		require AVX512_FP16
		AVX_512.basic_instruction_bcst_sae VEX_MAP5_W0,EVEX_REQUIRED+EVEX_VL,opcode,2,dest,src,src2
	end macro

	macro v#instr#sh? dest*,src*,src2*&
		require AVX512_FP16
		AVX_512.basic_instruction_sae VEX_F3_MAP5_W0,EVEX_REQUIRED,opcode,2,dest,src,src2
	end macro

end iterate

macro vsqrtph? dest*,src*&
	require AVX512_FP16
	AVX_512.single_source_instruction_bcst_er VEX_MAP5_W0,EVEX_REQUIRED+EVEX_VL,51h,2,dest,src
end macro

macro vsqrtsh? dest*,src*,src2*&
	require AVX512_FP16
	AVX_512.basic_instruction_er VEX_F3_MAP5_W0,EVEX_REQUIRED,51h,2,dest,src,src2
end macro

; --- fused multiply-add, map 6 (same opcode scheme as FMA3) --------------

iterate <instr,lcode>, vfmaddsub,6, vfmsubadd,7, vfmadd,8, vfmsub,0Ah, vfnmadd,0Ch, vfnmsub,0Eh

	iterate <order,hcode>, 132,90h, 213,0A0h, 231,0B0h

		macro instr#order#ph? dest*,src*,src2*&
			require AVX512_FP16
			AVX_512.basic_instruction_bcst_er VEX_66_MAP6_W0,EVEX_REQUIRED+EVEX_VL,hcode+lcode,2,dest,src,src2
		end macro

		if lcode > 7

			macro instr#order#sh? dest*,src*,src2*&
				require AVX512_FP16
				AVX_512.basic_instruction_er VEX_66_MAP6_W0,EVEX_REQUIRED,hcode+lcode+1,2,dest,src,src2
			end macro

		end if

	end iterate

end iterate

; --- complex multiply / multiply-add (pairs of FP16 as one 32-bit unit) ---
; The destination must differ from both sources (the CPU raises #UD).

iterate <instr,vex_mpw,opcode>, vfmulcph,VEX_F3_MAP6_W0,0D6h, vfcmulcph,VEX_F2_MAP6_W0,0D6h, \
				vfmaddcph,VEX_F3_MAP6_W0,56h, vfcmaddcph,VEX_F2_MAP6_W0,56h

	macro instr? dest*,src*,src2*&
		require AVX512_FP16
		AVX_512.basic_instruction_bcst_er vex_mpw,EVEX_REQUIRED+EVEX_VL,opcode,4,dest,src,src2
		if @dest.rm = @src.rm | (@src2.type = 'mmreg' & @dest.rm = @src2.rm)
			err 'destination must differ from the sources'
		end if
	end macro

end iterate

iterate <instr,vex_mpw,opcode>, vfmulcsh,VEX_F3_MAP6_W0,0D7h, vfcmulcsh,VEX_F2_MAP6_W0,0D7h, \
				vfmaddcsh,VEX_F3_MAP6_W0,57h, vfcmaddcsh,VEX_F2_MAP6_W0,57h

	macro instr? dest*,src*,src2*&
		require AVX512_FP16
		AVX_512.basic_instruction_er vex_mpw,EVEX_REQUIRED,opcode,4,dest,src,src2
		if @dest.rm = @src.rm | (@src2.type = 'mmreg' & @dest.rm = @src2.rm)
			err 'destination must differ from the sources'
		end if
	end macro

end iterate

; --- scale, reciprocal, exponent, mantissa, reduce, round ----------------

macro vscalefph? dest*,src*,src2*&
	require AVX512_FP16
	AVX_512.basic_instruction_bcst_er VEX_66_MAP6_W0,EVEX_REQUIRED+EVEX_VL,2Ch,2,dest,src,src2
end macro

macro vscalefsh? dest*,src*,src2*&
	require AVX512_FP16
	AVX_512.basic_instruction_er VEX_66_MAP6_W0,EVEX_REQUIRED,2Dh,2,dest,src,src2
end macro

iterate <instr,opcode>, vrcpph,4Ch, vrsqrtph,4Eh

	macro instr? dest*,src*
		require AVX512_FP16
		AVX_512.single_source_instruction_bcst VEX_66_MAP6_W0,EVEX_REQUIRED+EVEX_VL,opcode,2,dest,src
	end macro

end iterate

iterate <instr,opcode>, vrcpsh,4Dh, vrsqrtsh,4Fh

	macro instr? dest*,src*,src2*
		require AVX512_FP16
		AVX_512.basic_instruction VEX_66_MAP6_W0,EVEX_REQUIRED,opcode,2,dest,src,src2
	end macro

end iterate

macro vgetexpph? dest*,src*&
	require AVX512_FP16
	AVX_512.single_source_instruction_bcst_sae VEX_66_MAP6_W0,EVEX_REQUIRED+EVEX_VL,42h,2,dest,src
end macro

macro vgetexpsh? dest*,src*,src2*&
	require AVX512_FP16
	AVX_512.basic_instruction_sae VEX_66_MAP6_W0,EVEX_REQUIRED,43h,2,dest,src,src2
end macro

iterate <instr,opcode_ph,opcode_sh>, getmant,26h,27h, reduce,56h,57h, rndscale,8,0Ah

	macro v#instr#ph? dest*,src*,aux*&
		require AVX512_FP16
		AVX_512.single_source_instruction_bcst_sae_imm8 VEX_0F3A_W0,EVEX_REQUIRED+EVEX_VL,opcode_ph,2,dest,src,aux
	end macro

	macro v#instr#sh? dest*,src*,src2*,aux*&
		require AVX512_FP16
		AVX_512.basic_instruction_sae_imm8 VEX_0F3A_W0,EVEX_REQUIRED,opcode_sh,2,dest,src,src2,aux
	end macro

end iterate

; --- compare, classify --------------------------------------------------

macro vcmpph? dest*,src*,src2*,aux*&
	require AVX512_FP16
	AVX_512.parse_k1_operand@dest dest
	AVX_512.parse_operand@src src
	AVX_512.parse_bcst_operand@src2 src2,2
	match sae=,imm, aux
		AVX_512.parse_sae@src2 sae
		x86.parse_operand@aux imm
	else
		x86.parse_operand@aux aux
	end match
	if @dest.type = 'maskreg' & @src.type = 'mmreg' & (@src2.type = 'mem' | @src2.type = 'mmreg') & @aux.type = 'imm'
		if @aux.size and not 1
			err 'invalid operand size'
		else if @src2.size and not @src.size
			err 'operand sizes do not match'
		end if
		AVX_512.store_instruction@src2 @src.size,VEX_0F3A_W0,EVEX_REQUIRED+EVEX_VL,0C2h,@dest.mask,@dest.rm,@src.rm,1,@aux.imm
	else
		err 'invalid combination of operands'
	end if
end macro

macro vcmpsh? dest*,src*,src2*,aux*&
	require AVX512_FP16
	AVX_512.parse_k1_operand@dest dest
	AVX_512.parse_operand@src src
	AVX_512.parse_operand@src2 src2
	match sae=,imm, aux
		AVX_512.parse_sae@src2 sae
		x86.parse_operand@aux imm
	else
		x86.parse_operand@aux aux
	end match
	if @dest.type = 'maskreg' & @src.type = 'mmreg' & (@src2.type = 'mem' | @src2.type = 'mmreg') & @aux.type = 'imm'
		if @src.size <> 16 | (@src2.type = 'mem' & @src2.size and not 2) | @aux.size and not 1
			err 'invalid operand size'
		else if @src2.type = 'mmreg' & @src2.size <> @src.size
			err 'operand sizes do not match'
		end if
		@src2.memsize = 2
		AVX_512.store_instruction@src2 16,VEX_F3_0F3A_W0,EVEX_REQUIRED,0C2h,@dest.mask,@dest.rm,@src.rm,1,@aux.imm
	else
		err 'invalid combination of operands'
	end if
end macro

iterate <cond,code>, eq,0, lt,1, le,2, unord,3, neq,4, nlt,5, nle,6, ord,7, \
		     eq_uq,8, nge,9, ngt,0Ah, false,0Bh, neq_oq,0Ch, ge,0Dh, gt,0Eh, true,0Fh, \
		     eq_os,10h, lt_oq,11h, le_oq,12h, unord_s,13h, neq_us,14h, nlt_uq,15h, nle_uq,16h, ord_s,17h, \
		     eq_us,18h, nge_uq,19h, ngt_uq,1Ah, false_os,1Bh, neq_os,1Ch, ge_oq,1Dh, gt_oq,1Eh, true_us,1Fh

	macro vcmp#cond#ph? dest*,src*,src2*&
		vcmpph dest,src,src2,code
	end macro

	macro vcmp#cond#sh? dest*,src*,src2*&
		vcmpsh dest,src,src2,code
	end macro

end iterate

iterate <instr,opcode>, vcomish,2Fh, vucomish,2Eh

	macro instr? dest*,src_sae*&
		require AVX512_FP16
		AVX_512.parse_k1z_operand@dest dest
		match src=,sae, src_sae
			AVX_512.parse_operand@src src
			AVX_512.parse_sae@src sae
		else
			AVX_512.parse_operand@src src_sae
		end match
		if @dest.type = 'mmreg' & (@src.type = 'mem' | @src.type = 'mmreg')
			if @dest.size <> 16 | (@src.type = 'mem' & @src.size and not 2) | (@src.type = 'mmreg' & @src.size <> 16)
				err 'invalid operand size'
			end if
			@src.memsize = 2
			AVX_512.store_instruction@src 16,VEX_MAP5_W0,EVEX_REQUIRED,opcode,@dest.mask,@dest.rm
		else
			err 'invalid combination of operands'
		end if
	end macro

end iterate

macro vfpclassph? dest*,src*,aux*
	require AVX512_FP16
	AVX_512.parse_k1_operand@dest dest
	AVX_512.parse_bcst_operand@src src,2
	x86.parse_operand@aux aux
	if @dest.type = 'maskreg' & (@src.type = 'mem' | @src.type = 'mmreg') & @aux.type = 'imm'
		if @src.size = 0
			err 'operand size not specified'
		else if (@src.size <> 16 & @src.size <> 32 & @src.size <> 64) | @aux.size and not 1
			err 'invalid operand size'
		end if
		AVX_512.store_instruction@src @src.size,VEX_0F3A_W0,EVEX_REQUIRED+EVEX_VL,66h,@dest.mask,@dest.rm,,1,@aux.imm
	else
		err 'invalid combination of operands'
	end if
end macro

macro vfpclasssh? dest*,src*,aux*
	require AVX512_FP16
	AVX_512.parse_k1_operand@dest dest
	AVX_512.parse_operand@src src
	x86.parse_operand@aux aux
	if @dest.type = 'maskreg' & (@src.type = 'mem' | @src.type = 'mmreg') & @aux.type = 'imm'
		if (@src.type = 'mem' & @src.size and not 2) | (@src.type = 'mmreg' & @src.size <> 16) | @aux.size and not 1
			err 'invalid operand size'
		end if
		@src.memsize = 2
		AVX_512.store_instruction@src 16,VEX_0F3A_W0,EVEX_REQUIRED,67h,@dest.mask,@dest.rm,,1,@aux.imm
	else
		err 'invalid combination of operands'
	end if
end macro

; --- moves ---------------------------------------------------------------

macro vmovsh? dest*,src*,src2
	require AVX512_FP16
	AVX_512.parse_k1z_operand@dest dest
	AVX_512.parse_operand@src src
	match , src2
		if @dest.type = 'mmreg' & @src.type = 'mem'
			if @dest.size <> 16 | @src.size and not 2
				err 'invalid operand size'
			end if
			@src.memsize = 2
			AVX_512.store_instruction@src 16,VEX_F3_MAP5_W0,EVEX_REQUIRED,10h,@dest.mask,@dest.rm
		else if @dest.type = 'mem' & @src.type = 'mmreg'
			if @dest.size and not 2 | @src.size <> 16
				err 'invalid operand size'
			end if
			@dest.memsize = 2
			AVX_512.store_instruction@dest 16,VEX_F3_MAP5_W0,EVEX_REQUIRED,11h,@dest.mask,@src.rm
		else
			err 'invalid combination of operands'
		end if
	else
		AVX_512.parse_operand@src2 src2
		if @dest.type = 'mmreg' & @src.type = 'mmreg' & @src2.type = 'mmreg'
			if @dest.size <> 16 | @src.size <> 16 | @src2.size <> 16
				err 'invalid operand size'
			end if
			AVX_512.store_instruction@src2 16,VEX_F3_MAP5_W0,EVEX_REQUIRED,10h,@dest.mask,@dest.rm,@src.rm
		else
			err 'invalid combination of operands'
		end if
	end match
end macro

; VMOVW xmm, r32/m16 (6E) / VMOVW r32/m16, xmm (7E)
macro vmovw? dest*,src*
	require AVX512_FP16
	AVX_512.parse_operand@dest dest
	AVX_512.parse_operand@src src
	if @dest.type = 'mmreg' & (@src.type = 'reg' | @src.type = 'mem')
		if @dest.size <> 16 | (@src.type = 'reg' & @src.size <> 4) | (@src.type = 'mem' & @src.size and not 2)
			err 'invalid operand size'
		end if
		@src.memsize = 2
		AVX_512.store_instruction@src 16,VEX_66_MAP5_W0,EVEX_REQUIRED,6Eh,0,@dest.rm
	else if (@dest.type = 'reg' | @dest.type = 'mem') & @src.type = 'mmreg'
		if @src.size <> 16 | (@dest.type = 'reg' & @dest.size <> 4) | (@dest.type = 'mem' & @dest.size and not 2)
			err 'invalid operand size'
		end if
		@dest.memsize = 2
		AVX_512.store_instruction@dest 16,VEX_66_MAP5_W0,EVEX_REQUIRED,7Eh,0,@src.rm
	else
		err 'invalid combination of operands'
	end if
end macro

; --- conversions ------------------------------------------------------------

VEX_F3_MAP5_W1	= VEX_F3_MAP5_W0 or 8000h

; same element width: dest{k}{z}, src/m/m16bcst [, {er} or {sae}]
iterate <instr,vex_mpw,opcode,rounding>, \
	vcvtph2w,VEX_66_MAP5_W0,7Dh,er, vcvtph2uw,VEX_MAP5_W0,7Dh,er, \
	vcvtw2ph,VEX_F3_MAP5_W0,7Dh,er, vcvtuw2ph,VEX_F2_MAP5_W0,7Dh,er, \
	vcvttph2w,VEX_66_MAP5_W0,7Ch,sae, vcvttph2uw,VEX_MAP5_W0,7Ch,sae

	macro instr? dest*,src*&
		require AVX512_FP16
		AVX_512.single_source_instruction_bcst_#rounding vex_mpw,EVEX_REQUIRED+EVEX_VL,opcode,2,dest,src
	end macro

end iterate

; widening by ratio 2 (16 -> 32 bits) or 4 (16 -> 64 bits): the source is
; the destination's size divided by the ratio (at least an xmm register)
macro FP16.convert_widen vex_mpw,opcode,ratio,rounding,dest,src_r&
	AVX_512.parse_k1z_operand@dest dest
	match src=,r, src_r
		AVX_512.parse_operand@src src
		match =er, rounding
			AVX_512.parse_er@src r,64/ratio
		else
			AVX_512.parse_sae@src r
		end match
	else
		AVX_512.parse_bcst_operand@src src_r,2
	end match
	if @dest.type = 'mmreg' & (@src.type = 'mem' | @src.type = 'mmreg')
		if (@src.type = 'mem' & @src.size and not (@dest.size / ratio)) | (@src.type = 'mmreg' & (@dest.size / ratio - 1) and not 15 + 16 <> @src.size)
			err 'invalid operand size'
		end if
		if @src.memsize = 0
			@src.memsize = @dest.size / ratio
		end if
		AVX_512.store_instruction@src @dest.size,vex_mpw,EVEX_REQUIRED+EVEX_VL,opcode,@dest.mask,@dest.rm
	else
		err 'invalid combination of operands'
	end if
end macro

iterate <instr,vex_mpw,opcode,ratio,rounding>, \
	vcvtph2dq,VEX_66_MAP5_W0,5Bh,2,er, vcvtph2udq,VEX_MAP5_W0,79h,2,er, \
	vcvttph2dq,VEX_F3_MAP5_W0,5Bh,2,sae, vcvttph2udq,VEX_MAP5_W0,78h,2,sae, \
	vcvtph2psx,VEX_66_MAP6_W0,13h,2,sae, \
	vcvtph2qq,VEX_66_MAP5_W0,7Bh,4,er, vcvtph2uqq,VEX_66_MAP5_W0,79h,4,er, \
	vcvttph2qq,VEX_66_MAP5_W0,7Ah,4,sae, vcvttph2uqq,VEX_66_MAP5_W0,78h,4,sae, \
	vcvtph2pd,VEX_MAP5_W0,5Ah,4,sae

	macro instr? dest*,src*&
		require AVX512_FP16
		FP16.convert_widen vex_mpw,opcode,ratio,rounding,dest,src
	end macro

end iterate

; narrowing by ratio 2 (32 -> 16 bits) or 4 (64 -> 16 bits): the destination
; is the source's size divided by the ratio (at least an xmm register); a
; memory source needs an explicit size when that leaves it ambiguous
macro FP16.convert_narrow vex_mpw,opcode,ratio,unit,dest,src_er&
	AVX_512.parse_k1z_operand@dest dest
	match src=,er, src_er
		AVX_512.parse_operand@src src
		AVX_512.parse_er@src er
	else
		AVX_512.parse_bcst_operand@src src_er,unit
	end match
	if @dest.type = 'mmreg' & (@src.type = 'mem' | @src.type = 'mmreg')
		if @src.size = 0
			if @dest.size = 16
				err 'operand size not specified'
			else
				@src.size = 64
			end if
		end if
		if (@src.size / ratio - 1) and not 15 + 16 <> @dest.size | @src.size > 64
			err 'invalid operand size'
		end if
		AVX_512.store_instruction@src @src.size,vex_mpw,EVEX_REQUIRED+EVEX_VL,opcode,@dest.mask,@dest.rm
	else
		err 'invalid combination of operands'
	end if
end macro

iterate <instr,vex_mpw,opcode,ratio,unit>, \
	vcvtdq2ph,VEX_MAP5_W0,5Bh,2,4, vcvtudq2ph,VEX_F2_MAP5_W0,7Ah,2,4, vcvtps2phx,VEX_66_MAP5_W0,1Dh,2,4, \
	vcvtqq2ph,VEX_MAP5_W1,5Bh,4,8, vcvtuqq2ph,VEX_F2_MAP5_W1,7Ah,4,8, vcvtpd2ph,VEX_66_MAP5_W1,5Ah,4,8

	macro instr? dest*,src*&
		require AVX512_FP16
		FP16.convert_narrow vex_mpw,opcode,ratio,unit,dest,src
	end macro

end iterate

; scalar FP16 <-> FP32/FP64: dest, src1, src2/m (the scalar's size)
iterate <instr,vex_mpw,opcode,msize,rounding>, \
	vcvtsh2ss,VEX_MAP6_W0,13h,2,sae, vcvtsh2sd,VEX_F3_MAP5_W0,5Ah,2,sae, \
	vcvtss2sh,VEX_MAP5_W0,1Dh,4,er, vcvtsd2sh,VEX_F2_MAP5_W1,5Ah,8,er

	macro instr? dest*,src*,src2*&
		require AVX512_FP16
		AVX_512.basic_instruction_#rounding vex_mpw,EVEX_REQUIRED,opcode,msize,dest,src,src2
	end macro

end iterate

; FP16 scalar -> general-purpose register: r32/r64, xmm/m16 [, {er}/{sae}]
iterate <instr,opcode,rounding>, vcvtsh2si,2Dh,er, vcvtsh2usi,79h,er, vcvttsh2si,2Ch,sae, vcvttsh2usi,78h,sae

	macro instr? dest*,src_r*&
		require AVX512_FP16
		x86.parse_operand@dest dest
		match src=,r, src_r
			AVX_512.parse_operand@src src
			match =er, rounding
				AVX_512.parse_er@src r,16
			else
				AVX_512.parse_sae@src r
			end match
		else
			AVX_512.parse_operand@src src_r
		end match
		if @dest.type = 'reg' & (@src.type = 'mem' | @src.type = 'mmreg')
			if (@dest.size <> 4 & @dest.size <> 8) | (@src.type = 'mem' & @src.size and not 2) | (@src.type = 'mmreg' & @src.size <> 16)
				err 'invalid operand size'
			end if
			@src.memsize = 2
			if @dest.size = 8
				if x86.mode < 64
					err 'instruction requires long mode'
				end if
				AVX_512.store_instruction@src 16,VEX_F3_MAP5_W1,EVEX_REQUIRED,opcode,0,@dest.rm
			else
				AVX_512.store_instruction@src 16,VEX_F3_MAP5_W0,EVEX_REQUIRED,opcode,0,@dest.rm
			end if
		else
			err 'invalid combination of operands'
		end if
	end macro

end iterate

; general-purpose register -> FP16 scalar: xmm, xmm, r/m32 or r/m64 [, {er}]
iterate <instr,opcode>, vcvtsi2sh,2Ah, vcvtusi2sh,7Bh

	macro instr? dest*,src*,src_er*&
		require AVX512_FP16
		AVX_512.parse_operand@dest dest
		AVX_512.parse_operand@src src
		match src2=,er, src_er
			AVX_512.parse_operand@src2 src2
			AVX_512.parse_er@src2 er,@src2.size
		else
			AVX_512.parse_operand@src2 src_er
		end match
		if @dest.type = 'mmreg' & @src.type = 'mmreg' & (@src2.type = 'reg' | @src2.type = 'mem')
			if @src2.size = 0
				err 'operand size not specified'
			else if @dest.size <> 16 | @src.size <> 16 | (@src2.size <> 4 & @src2.size <> 8)
				err 'invalid operand size'
			end if
			if @src2.size = 8
				if x86.mode < 64
					err 'instruction requires long mode'
				end if
				AVX_512.store_instruction@src2 16,VEX_F3_MAP5_W1,EVEX_REQUIRED,opcode,0,@dest.rm,@src.rm
			else
				AVX_512.store_instruction@src2 16,VEX_F3_MAP5_W0,EVEX_REQUIRED,opcode,0,@dest.rm,@src.rm
			end if
		else
			err 'invalid combination of operands'
		end if
	end macro

end iterate
