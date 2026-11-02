; fma4.asm -- AMD four-operand FMA (iset/fma4.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc,
;	operand roles from zydis-db); a mismatch stops assembly with 'encoding
;	mismatch'. The failure cases at the end are each assembled alone by
;	_run.ps1 (-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 fma4.asm fma4.bin
;
; What is new
;	FMA4 (8000_0001h:ECX[16]) was missing. Failure modes this file guards
;	against:
;	- VEX.W choosing which of src2/src3 sits in ModRM.rm (and may be
;	  memory) and which in imm8[7:4]; register-only forms use W1, as llvm-mc
;	  does;
;	- scalar forms with their 32/64-bit memory size, packed forms with
;	  ymm; no scalar forms of vfmaddsub/vfmsubadd;
;	- FMA4 and Intel FMA3 are separate features with separate names.
;
; FAIL 1: 'FMA4 or higher required'
; FAIL 2: 'operand sizes do not match'
; FAIL 3: 'invalid combination of operands'
; FAIL 4: 'illegal instruction'
; FAIL 5: 'invalid combination of operands'

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

	expect <0C4h,0E3h,0F1h,68h,0C3h,20h>, {i386, x64, AVX, FMA4} vfmaddps xmm0, xmm1, xmm2, xmm3
	expect <0C4h,0E3h,0F1h,68h,00h,20h>, {i386, x64, AVX, FMA4} vfmaddps xmm0, xmm1, xmm2, [rax]
	expect <0C4h,0E3h,71h,68h,00h,30h>, {i386, x64, AVX, FMA4} vfmaddps xmm0, xmm1, [rax], xmm3
	expect <0C4h,0E3h,0F5h,69h,0C3h,20h>, {i386, x64, AVX, FMA4} vfmaddpd ymm0, ymm1, ymm2, ymm3
	expect <0C4h,0E3h,0F1h,6Ah,0C3h,20h>, {i386, x64, AVX, FMA4} vfmaddss xmm0, xmm1, xmm2, xmm3
	expect <0C4h,0E3h,0F1h,6Bh,00h,20h>, {i386, x64, AVX, FMA4} vfmaddsd xmm0, xmm1, xmm2, qword [rax]
	expect <0C4h,0E3h,0F1h,5Ch,0C3h,20h>, {i386, x64, AVX, FMA4} vfmaddsubps xmm0, xmm1, xmm2, xmm3
	expect <0C4h,0E3h,0F5h,5Fh,0C3h,20h>, {i386, x64, AVX, FMA4} vfmsubaddpd ymm0, ymm1, ymm2, ymm3
	expect <0C4h,0E3h,71h,7Eh,00h,30h>, {i386, x64, AVX, FMA4} vfnmsubss xmm0, xmm1, dword [rax], xmm3
	expect <0C4h,0E3h,0F1h,7Bh,0C3h,20h>, {i386, x64, AVX, FMA4} vfnmaddsd xmm0, xmm1, xmm2, xmm3
	expect <0C4h,0E3h,0F1h,68h,0C3h,20h>, {everything} vfmaddps xmm0, xmm1, xmm2, xmm3

else if FAIL = 1	; FMA (FMA3) does not provide FMA4
	use i386, x64, AVX, FMA
	vfmaddps xmm0, xmm1, xmm2, xmm3

else if FAIL = 2	; scalar memory is 32/64-bit
	use everything
	vfmaddss xmm0, xmm1, xmm2, qword [rax]

else if FAIL = 3	; scalar forms are xmm only
	use everything
	vfmaddsd ymm0, ymm1, ymm2, ymm3

else if FAIL = 4	; vfmaddsub has no scalar form
	use everything
	vfmaddsubss xmm0, xmm1, xmm2, xmm3

else if FAIL = 5	; one memory operand, never in src1
	use everything
	vfmaddps xmm0, [rax], xmm2, xmm3

end if
