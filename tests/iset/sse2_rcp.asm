; sse2_rcp.asm -- no RCP/RSQRT in double precision (G16)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc
;	and the SDM); a mismatch stops assembly with 'encoding mismatch'. The
;	failure cases at the end are each assembled alone by _run.ps1
;	(-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 sse2_rcp.asm sse2_rcp.bin
;
; What is resolved
;	sse2.inc reused SSE's iterate list, rsqrt and rcp included, so
;	rsqrtpd, rsqrtsd, rcppd and rcpsd assembled (66h/F2h on 0F 52/53).
;	No such instructions exist; XED refuses the bytes (#UD). The SSE
;	single-precision forms and the rest of the SSE2 list are unchanged.
;
; FAIL 1: 'illegal instruction'
; FAIL 2: 'illegal instruction'
; FAIL 3: 'illegal instruction'
; FAIL 4: 'illegal instruction'

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
use everything

if ~ defined FAIL

	expect <0Fh,53h,0C1h>, rcpps xmm0, xmm1
	expect <0F3h,0Fh,53h,0C1h>, rcpss xmm0, xmm1
	expect <0Fh,52h,0C1h>, rsqrtps xmm0, xmm1
	expect <0F3h,0Fh,52h,0C1h>, rsqrtss xmm0, xmm1
	expect <66h,0Fh,51h,0C1h>, sqrtpd xmm0, xmm1
	expect <0F2h,0Fh,51h,0C1h>, sqrtsd xmm0, xmm1
	expect <66h,0Fh,58h,0C1h>, addpd xmm0, xmm1
	expect <0F2h,0Fh,5Fh,0C1h>, maxsd xmm0, xmm1

else if FAIL = 1
	rcppd xmm0, xmm1

else if FAIL = 2
	rcpsd xmm0, xmm1

else if FAIL = 3
	rsqrtpd xmm0, xmm1

else if FAIL = 4
	rsqrtsd xmm0, xmm1

end if
