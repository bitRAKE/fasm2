; 3dnowext.asm -- 3DNow! extensions: PF2IW, PI2FW, PFNACC, PFPNACC, PSWAPD (iset/3dnow.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 3dnowext.asm 3dnowext.bin
;
; What is new
;	The Athlon 3DNow! extensions (3DNowExt, 8000_0001h:EDX[30]) were
;	missing, except pi2fw, which was gated on plain 3DNow.
;	Behaviour change: pi2fw now needs 3DNowExt (a K6-2 does not have it).
;	"use AMD64" includes 3DNowExt, so code written for that preset keeps
;	assembling. Failure modes this file guards against:
;	- one gate enabling the other's instructions;
;	- the 0F 0F /r suffix byte coming after the ModRM and displacement.
;
; FAIL 1: '3DNowExt or higher required'
; FAIL 2: '3DNowExt or higher required'
; FAIL 3: '3DNow or higher required'
; FAIL 4: 'invalid combination of operands'

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

use32

if ~ defined FAIL

	expect <0Fh,0Fh,0CAh,0BBh>, {i386, MMX, 3DNowExt} pswapd mm1, mm2
	expect <0Fh,0Fh,0CAh,8Ah>, {i386, MMX, 3DNowExt} pfnacc mm1, mm2
	expect <0Fh,0Fh,0CAh,8Eh>, {i386, MMX, 3DNowExt} pfpnacc mm1, mm2
	expect <0Fh,0Fh,0CAh,1Ch>, {i386, MMX, 3DNowExt} pf2iw mm1, mm2
	expect <0Fh,0Fh,08h,0Ch>, {i386, MMX, 3DNowExt} pi2fw mm1, [eax]
	expect <0Fh,0Fh,48h,10h,0BBh>, {i386, MMX, 3DNowExt} pswapd mm1, [eax+10h]
	expect <0Fh,0Fh,0CAh,0Ch>, {AMD64} pi2fw mm1, mm2
	expect <0Fh,0Fh,0CAh,0Dh>, {i386, MMX, 3DNow} pi2fd mm1, mm2

else if FAIL = 1	; 3DNow alone (K6-2) has no pswapd
	use i386, MMX, 3DNow
	pswapd mm1, mm2

else if FAIL = 2	; nor pi2fw (behaviour change)
	use i386, MMX, 3DNow
	pi2fw mm1, mm2

else if FAIL = 3	; 3DNowExt does not imply 3DNow
	use i386, MMX, 3DNowExt
	pfadd mm1, mm2

else if FAIL = 4	; MMX registers
	use everything
	pswapd xmm1, xmm2

end if
