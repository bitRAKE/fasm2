; vp2intersect.asm -- VP2INTERSECTD/Q into a mask register pair (iset/vp2intersect.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 vp2intersect.asm vp2i.bin
;
; What is new
;	VP2INTERSECTD/Q (AVX512_VP2INTERSECT, 7.0:EDX[8]) were missing.
;	Failure modes this file guards against:
;	- the destination is a pair k(2n), k(2n+1), named by its even member;
;	  an odd mask register cannot be encoded;
;	- xmm/ymm forms also need AVX512VL (XED lists both bits);
;	- broadcast element size follows D/Q; disp8 scales by the vector.
;
; FAIL 1: 'AVX512_VP2INTERSECT or higher required'
; FAIL 2: 'even mask register expected'
; FAIL 3: 'AVX512VL or higher required'
; FAIL 4: 'invalid combination of operands'
; FAIL 5: 'operand sizes do not match'

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

	expect <62h,0F2h,6Fh,48h,68h,0D3h>, {i386, x64, AVX512F, AVX512_VP2INTERSECT} vp2intersectd k2, zmm2, zmm3
	expect <62h,0F2h,6Fh,28h,68h,0E3h>, {i386, x64, AVX512F, AVX512VL, AVX512_VP2INTERSECT} vp2intersectd k4, ymm2, ymm3
	expect <62h,0F2h,0EFh,58h,68h,30h>, {i386, x64, AVX512F, AVX512_VP2INTERSECT} vp2intersectq k6, zmm2, [rax]{1to8}
	expect <62h,0F2h,6Fh,48h,68h,40h,01h>, {i386, x64, AVX512F, AVX512_VP2INTERSECT} vp2intersectd k0, zmm2, [rax+64]
	expect <62h,0F2h,6Fh,48h,68h,0D3h>, {everything} vp2intersectd k2, zmm2, zmm3

else if FAIL = 1	; AVX512F alone is not enough
	use i386, x64, AVX512F
	vp2intersectd k2, zmm2, zmm3

else if FAIL = 2	; k3 is the odd half of a pair
	use everything
	vp2intersectd k3, zmm2, zmm3

else if FAIL = 3	; 256-bit form needs AVX512VL
	use i386, x64, AVX512F, AVX512_VP2INTERSECT
	vp2intersectd k2, ymm2, ymm3

else if FAIL = 4	; destination is a mask register
	use everything
	vp2intersectd zmm1, zmm2, zmm3

else if FAIL = 5	; sources the same width
	use everything
	vp2intersectq k2, zmm2, ymm3

end if
