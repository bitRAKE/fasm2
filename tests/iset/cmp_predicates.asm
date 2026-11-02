; cmp_predicates.asm -- the NEQ_OQ compare predicate name (fix G9)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure case
;	at the end is assembled alone by _run.ps1 (-i"FAIL=n") and must stop
;	with the quoted message. By hand:  fasm2 cmp_predicates.asm cmp.bin
;
; Failure mode resolved
;	G9  avx.inc spelled predicate 0Ch "neq_qq", so the SDM names
;	    vcmpneq_oqps/pd/ss/sd did not exist ('illegal instruction'). The
;	    correct name is added; neq_qq stays as an alias so existing sources
;	    keep assembling.
;
; FAIL 1: 'illegal instruction'

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

	expect <0C5h,0E8h,0C2h,0CBh,0Ch>, {everything} vcmpneq_oqps xmm1, xmm2, xmm3
	expect <62h,0F1h,0EDh,48h,0C2h,0CBh,0Ch>, {everything} vcmpneq_oqpd k1, zmm2, zmm3
	expect <0C5h,0E8h,0C2h,0CBh,0Ch>, {everything} vcmpneq_qqps xmm1, xmm2, xmm3	; old alias
	expect <0C5h,0E8h,0C2h,0CBh,0Dh>, {everything} vcmpgeps xmm1, xmm2, xmm3	; neighbour

else if FAIL = 1	; not a predicate name
	use everything
	vcmpneq_xxps xmm1, xmm2, xmm3

end if
