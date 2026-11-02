; avx512_sae.asm -- {sae} on vcvttps2qq / vcvttps2uqq (fix G10)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure case
;	at the end is assembled alone by _run.ps1 (-i"FAIL=n") and must stop
;	with the quoted message. By hand:  fasm2 avx512_sae.asm sae.bin
;
; Failure mode resolved
;	G10 The {sae} path of vcvttps2qq/vcvttps2uqq called
;	    "AVX_512.parse_sae @src,sae" (a space before the context), which is
;	    not a defined instruction, so  vcvttps2qq zmm1, ymm2, {sae}  failed
;	    with 'illegal instruction'. Without {sae} they worked.
;	{sae} with registers: fasm2 writes EVEX.L'L as the vector length
;	(llvm-mc writes 00); the CPU ignores L'L in that form.
;
; FAIL 1: 'invalid operand size'

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

	expect <62h,0F1h,7Dh,58h,7Ah,0CAh>, {everything} vcvttps2qq zmm1, ymm2, {sae}
	expect <62h,0F1h,7Dh,58h,78h,0CAh>, {everything} vcvttps2uqq zmm1, ymm2, {sae}
	expect <62h,0F1h,7Dh,48h,7Ah,0CAh>, {everything} vcvttps2qq zmm1, ymm2

else if FAIL = 1	; the source is half the destination
	use everything
	vcvttps2qq zmm1, zmm2, {sae}

end if
