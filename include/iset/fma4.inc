; fma4.inc -- AMD four-operand fused multiply-add
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting	CPUID bit		instructions
;	FMA4	8000_0001h:ECX[16]	vfmadd{ps,pd,ss,sd} vfmsub{..} vfnmadd{..}
;					vfnmsub{..} vfmaddsub{ps,pd} vfmsubadd{ps,pd}
;
; Also needs AVX. Valid in 16-, 32- and 64-bit code. xmm/ymm 0-15 only.
;
; dest, src1, src2, src3 -- VEX.66.0F3A, src1 in VEX.vvvv; one of src2/src3
; in ModRM.rm (it may be memory) and the other in imm8[7:4]; VEX.W says
; which. Packed forms are 128/256-bit, scalar forms 128-bit with a 32/64-bit
; memory operand. Uses XOP.is4_instruction from xop.inc.
;
; The names differ from FMA3 (vfmadd132ps ...), so there is no overlap with
; the Intel instructions in avx.inc.

iterate <instr,opcode>, vfmaddsub,5Ch, vfmsubadd,5Eh, vfmadd,68h, vfmsub,6Ch, vfnmadd,78h, vfnmsub,7Ch

	iterate <type,offset,sizes,msize>, ps,0,16+32,0, pd,1,16+32,0, ss,2,16,4, sd,3,16,8

		if opcode >= 68h | offset < 2	; vfmaddsub/vfmsubadd have packed forms only
			calminstruction instr#type? dest*,src1*,src2*,src3*
				call	x86.require.FMA4
				xcall	XOP.is4_instruction, (VEX_66_0F3A_W0),(opcode+offset),(sizes),(msize),(1),(0),dest,src1,src2,src3
			end calminstruction
		end if

	end iterate

end iterate
