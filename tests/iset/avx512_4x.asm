; avx512_4x.asm -- 4VNNIW disp8 scaling (fix G8) and AVX512_4FMAPS (iset/avx.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 avx512_4x.asm avx512_4x.bin
;
; Failure mode resolved
;	G8  vp4dpwssd/vp4dpwssds read a 128-bit memory operand (EVEX tuple
;	    Tuple1_4X), so a compressed disp8 is scaled by 16. The encoder
;	    scaled it by the 64-byte vector instead:
;	      vp4dpwssd zmm1, zmm4+3, [rax+40h]  gave disp8 = 1, which the CPU
;	      reads as [rax+10h] -- wrong address, no error;
;	      [rax+10h] fell back to a 32-bit displacement.
;	New: v4fmaddps, v4fnmaddps, v4fmaddss, v4fnmaddss (AVX512_4FMAPS,
;	7.0:EDX[3]; Knights Mill), same operand shape and disp8 rule.
;
; FAIL 1: 'AVX512_4FMAPS or higher required'
; FAIL 2: 'invalid operand size'
; FAIL 3: 'invalid combination of operands'
; FAIL 4: 'invalid operand size'

macro expect bytes*, line&
	local n, got, k, want
	virtual at 0
		line
		n = $
		load got:n from 0
	end virtual
	virtual at 0
		db bytes
		k = $
		load want:k from 0
	end virtual
	if n <> k | got <> want
		display 'FAIL: ', `line, 13, 10
		err 'encoding mismatch'
	end if
end macro

use64

if ~ defined FAIL

	; G8: disp8 scales by 16
	expect <62h,0F2h,5Fh,48h,52h,48h,01h>, {everything} vp4dpwssd zmm1, zmm4+3, [rax+10h]
	expect <62h,0F2h,5Fh,48h,52h,48h,04h>, {everything} vp4dpwssd zmm1, zmm4+3, [rax+40h]

	; 4FMAPS
	expect <62h,0F2h,5Fh,48h,9Ah,48h,01h>, {i386, x64, AVX512F, AVX512_4FMAPS} v4fmaddps zmm1, zmm4+3, [rax+10h]
	expect <62h,0F2h,5Fh,0C9h,9Ah,08h>, {i386, x64, AVX512F, AVX512_4FMAPS} v4fmaddps zmm1{k1}{z}, zmm4+3, [rax]
	expect <62h,0F2h,3Fh,48h,0AAh,08h>, {i386, x64, AVX512F, AVX512_4FMAPS} v4fnmaddps zmm1, zmm8+3, [rax]
	expect <62h,0F2h,5Fh,08h,9Bh,48h,01h>, {i386, x64, AVX512F, AVX512_4FMAPS} v4fmaddss xmm1, xmm4+3, [rax+10h]
	expect <62h,0F2h,5Fh,0Ah,0ABh,08h>, {i386, x64, AVX512F, AVX512_4FMAPS} v4fnmaddss xmm1{k2}, xmm4+3, dqword [rax]
	expect <62h,0F2h,5Fh,48h,9Ah,08h>, {everything} v4fmaddps zmm1, zmm4, [rax]

else if FAIL = 1	; 4VNNIW does not provide 4FMAPS
	use i386, x64, AVX512F, AVX512_4VNNIW
	v4fmaddps zmm1, zmm4+3, [rax]

else if FAIL = 2	; packed forms are 512-bit only
	use everything
	v4fmaddps ymm1, ymm4+3, [rax]

else if FAIL = 3	; the last operand is memory
	use everything
	v4fmaddps zmm1, zmm4+3, zmm8

else if FAIL = 4	; it is 128 bits
	use everything
	v4fmaddss xmm1, xmm4+3, qword [rax]

end if
