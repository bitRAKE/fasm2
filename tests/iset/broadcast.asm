; broadcast.asm -- {1to32} embedded broadcast for 16-bit elements (fix G5)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 broadcast.asm broadcast.bin
;
;	No 16-bit-element instruction exists in include/iset yet, so the test
;	defines its own stand-in for VADDPH (AVX512_FP16: EVEX.NP.MAP5.W0 58 /r)
;	on top of the existing AVX-512 helper. It is assembled with
;	"use everything", which this file never narrows.
;
; Failure mode resolved
;	AVX_512.parse_bcst_operand accepted {1to2} .. {1to16} only. A 512-bit
;	vector of 16-bit elements (AVX512_FP16, BF16 inputs) broadcasts one
;	word 32 times, so  vaddph zmm1, zmm2, [rax]{1to32}  failed with
;	'invalid broadcast'. {1to32} must still be refused where 32 elements do
;	not fill the vector (dwords, or words in a ymm); the size check refuses it.
;
; FAIL 1: 'operand sizes do not match'
; FAIL 2: 'operand sizes do not match'

EVEX_MAP5_W0 = 101b		; not yet part of avx.inc

macro test_vaddph? dest*,src*,src2*&
	AVX_512.basic_instruction_bcst_er EVEX_MAP5_W0,EVEX_REQUIRED+EVEX_VL,58h,2,dest,src,src2
end macro

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

	; word elements: 1to8 / 1to16 / 1to32 for xmm / ymm / zmm
	expect <62h,0F5h,6Ch,58h,58h,08h>, test_vaddph zmm1, zmm2, [rax]{1to32}
	expect <62h,0F5h,6Ch,38h,58h,08h>, test_vaddph ymm1, ymm2, [rax]{1to16}
	expect <62h,0F5h,6Ch,18h,58h,08h>, test_vaddph xmm1, xmm2, [rax]{1to8}
	; disp8*N scales by the element (2), not the vector: 64 -> 20h
	expect <62h,0F5h,6Ch,58h,58h,48h,20h>, test_vaddph zmm1, zmm2, [rax+64]{1to32}

	; existing dword/qword broadcasts are unchanged
	expect <62h,0F1h,6Ch,58h,58h,08h>, vaddps zmm1, zmm2, [rax]{1to16}
	expect <62h,0F1h,0EDh,58h,58h,08h>, vaddpd zmm1, zmm2, [rax]{1to8}

else if FAIL = 1	; 32 dwords is not a zmm
	vaddps zmm1, zmm2, [rax]{1to32}

else if FAIL = 2	; 32 words is not a ymm
	test_vaddph ymm1, ymm2, [rax]{1to32}

end if
