; sm.asm -- SM3 and SM4 instructions (iset/sm.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 sm.asm sm.bin
;
; What is new
;	VSM3MSG1/MSG2/RNDS2 (SM3, 7.1:EAX[1]) and VSM4KEY4/RNDS4 (SM4,
;	7.1:EAX[2]) were missing. Failure modes this file guards against:
;	- SM3 and SM4 are separate bits;
;	- SM3 is 128-bit only, SM4 is 128- or 256-bit; both are VEX only;
;	- vsm3rnds2 takes an 8-bit immediate;
;	- SM3 sits at bit 1 of the 7.1:EAX window, where XSAVEC used to be.
;
; FAIL 1: 'SM3 or higher required'
; FAIL 2: 'SM4 or higher required'
; FAIL 3: 'invalid combination of operands'
; FAIL 4: 'invalid operand'
; FAIL 5: 'invalid operand size'
; FAIL 6: 'SM3 or higher required'

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

	expect <0C4h,0E2h,68h,0DAh,0CBh>, {i386, x64, AVX, SM3} vsm3msg1 xmm1, xmm2, xmm3
	expect <0C4h,0E2h,68h,0DAh,08h>, {i386, x64, AVX, SM3} vsm3msg1 xmm1, xmm2, [rax]
	expect <0C4h,0E2h,69h,0DAh,0CBh>, {i386, x64, AVX, SM3} vsm3msg2 xmm1, xmm2, xmm3
	expect <0C4h,0E3h,69h,0DEh,0CBh,05h>, {i386, x64, AVX, SM3} vsm3rnds2 xmm1, xmm2, xmm3, 5
	expect <0C4h,0E3h,69h,0DEh,08h,05h>, {i386, x64, AVX, SM3} vsm3rnds2 xmm1, xmm2, dqword [rax], 5
	expect <0C4h,0E2h,6Ah,0DAh,0CBh>, {i386, x64, AVX, SM4} vsm4key4 xmm1, xmm2, xmm3
	expect <0C4h,0E2h,6Eh,0DAh,0CBh>, {i386, x64, AVX, SM4} vsm4key4 ymm1, ymm2, ymm3
	expect <0C4h,0E2h,6Fh,0DAh,08h>, {i386, x64, AVX, SM4} vsm4rnds4 ymm1, ymm2, [rax]
	expect <0C4h,0E2h,6Fh,0DAh,08h>, {everything} vsm4rnds4 ymm1, ymm2, [rax]

else if FAIL = 1	; SM4 does not enable SM3
	use i386, x64, AVX, SM4
	vsm3msg1 xmm1, xmm2, xmm3

else if FAIL = 2	; SM3 does not enable SM4
	use i386, x64, AVX, SM3
	vsm4key4 xmm1, xmm2, xmm3

else if FAIL = 3	; SM3 is 128-bit only
	use everything
	vsm3msg2 ymm1, ymm2, ymm3

else if FAIL = 4	; SM4 has no EVEX form here: no zmm
	use everything
	vsm4rnds4 zmm1, zmm2, zmm3

else if FAIL = 5	; one-byte immediate
	use everything
	vsm3rnds2 xmm1, xmm2, xmm3, word 5

else if FAIL = 6	; XSAVEC, once in the same window, does not enable it
	use i386, x64, AVX, XSAVE, XSAVEC
	vsm3msg1 xmm1, xmm2, xmm3

end if
