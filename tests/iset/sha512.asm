; sha512.asm -- SHA-512 instructions (iset/sha512.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 sha512.asm sha512.bin
;
; What is new
;	VSHA512MSG1/MSG2/RNDS2 (SHA512, 7.1:EAX[0]; XED also lists AVX) were
;	missing. Failure modes this file guards against:
;	- the operands mix widths (ymm, xmm) and are register-only;
;	- SHA512 sits at bit 0 of the 7.1:EAX window, where XSAVEOPT used to
;	  be (fix G1): the two must not enable each other;
;	- the older SHA gate (SHA-1/SHA-256) does not enable SHA-512.
;
; FAIL 1: 'SHA512 or higher required'
; FAIL 2: 'SHA512 or higher required'
; FAIL 3: 'invalid combination of operands'
; FAIL 4: 'invalid combination of operands'
; FAIL 5: 'invalid operand'
; FAIL 6: 'XSAVEOPT required'

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

	expect <0C4h,0E2h,7Fh,0CCh,0CAh>, {i386, x64, AVX, SHA512} vsha512msg1 ymm1, xmm2
	expect <0C4h,0E2h,7Fh,0CDh,0CAh>, {i386, x64, AVX, SHA512} vsha512msg2 ymm1, ymm2
	expect <0C4h,0E2h,6Fh,0CBh,0CBh>, {i386, x64, AVX, SHA512} vsha512rnds2 ymm1, ymm2, xmm3
	expect <0C4h,42h,1Fh,0CBh,0DDh>, {i386, x64, AVX, SHA512} vsha512rnds2 ymm11, ymm12, xmm13
	expect <0C4h,0E2h,7Fh,0CCh,0CAh>, {everything} vsha512msg1 ymm1, xmm2

else if FAIL = 1	; SHA (SHA-1/SHA-256) is a different feature
	use i386, x64, AVX, SHA
	vsha512msg1 ymm1, xmm2

else if FAIL = 2	; XSAVEOPT, once in the same window, does not enable it
	use i386, x64, AVX, XSAVE, XSAVEOPT
	vsha512msg2 ymm1, ymm2

else if FAIL = 3	; register operands only
	use everything
	vsha512msg1 ymm1, [rax]

else if FAIL = 4	; the third operand is xmm
	use everything
	vsha512rnds2 ymm1, ymm2, ymm3

else if FAIL = 5	; VEX only: no ymm16+
	use everything
	vsha512msg2 ymm1, ymm17

else if FAIL = 6	; and SHA512 does not enable XSAVEOPT
	use i386, x64, AVX, XSAVE, SHA512
	xsaveopt [rax]

end if
