; xtest.asm -- XTEST is available with HLE or with RTM (fix G3)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes; a mismatch stops
;	assembly with 'encoding mismatch'. The failure cases at the end are
;	each assembled alone by _run.ps1 (-i"FAIL=n") and must stop with the
;	quoted message. By hand:  fasm2 xtest.asm xtest.bin
;
; Failure mode resolved
;	XTEST is reported by CPUID when either HLE (7.0:EBX[4]) or RTM
;	(7.0:EBX[11]) is present. x86-2.inc defined XTEST twice, once gated on
;	HLE and once on RTM; the later definition silently won, so an HLE-only
;	target was refused:  use i386, HLE / xtest  ->  'RTM required'.
;	There is now a single definition with an any-of gate.
;
; FAIL 1: 'HLE or RTM required'

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

if ~ defined FAIL

	expect <0Fh,01h,0D6h>, {i386, HLE} xtest		; HLE alone (was refused)
	expect <0Fh,01h,0D6h>, {i386, RTM} xtest		; RTM alone
	expect <0Fh,01h,0D6h>, {i386, HLE, RTM} xtest	; both
	expect <0Fh,01h,0D6h>, {everything} xtest

else if FAIL = 1	; neither HLE nor RTM
	use i386
	xtest

end if
