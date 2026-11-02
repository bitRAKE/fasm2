; amx.asm -- AMX-FP16 and AMX-COMPLEX tile products (iset/amx.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 amx.asm amx.bin
;
; What is new
;	TDPFP16PS (AMX_FP16, 7.1:EAX[21]) and TCMMIMFP16PS/TCMMRLFP16PS
;	(AMX_COMPLEX, 7.1:EDX[8]) were missing. Failure modes this file guards
;	against:
;	- each needs AMX_TILE as well as its own bit (XED lists both);
;	- the three tile operands must be distinct (the CPU raises #UD);
;	- AMX executes in 64-bit mode only;
;	- like every VEX instruction here they also need AVX;
;	- the existing tdpbf16ps (same opcode, F3 instead of F2) is unchanged.
;
; FAIL 1: 'AMX_FP16 or higher required'
; FAIL 2: 'AMX_TILE or higher required'
; FAIL 3: 'AMX_COMPLEX or higher required'
; FAIL 4: 'tile registers must be distinct'
; FAIL 5: 'bits64 or higher required'
; FAIL 6: 'invalid operand'

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

	expect <0C4h,0E2h,63h,5Ch,0CAh>, {i386, x64, AVX, AMX_TILE, AMX_FP16} tdpfp16ps tmm1, tmm2, tmm3
	expect <0C4h,0E2h,61h,6Ch,0CAh>, {i386, x64, AVX, AMX_TILE, AMX_COMPLEX} tcmmimfp16ps tmm1, tmm2, tmm3
	expect <0C4h,0E2h,60h,6Ch,0CAh>, {i386, x64, AVX, AMX_TILE, AMX_COMPLEX} tcmmrlfp16ps tmm1, tmm2, tmm3
	expect <0C4h,0E2h,62h,5Ch,0CAh>, {everything} tdpbf16ps tmm1, tmm2, tmm3
	expect <0C4h,0E2h,63h,5Ch,0CAh>, {everything} tdpfp16ps tmm1, tmm2, tmm3

else if FAIL = 1	; AMX_BF16 does not enable FP16
	use i386, x64, AMX_TILE, AMX_BF16
	tdpfp16ps tmm1, tmm2, tmm3

else if FAIL = 2	; the tile feature is required too
	use i386, x64, AMX_FP16
	tdpfp16ps tmm1, tmm2, tmm3

else if FAIL = 3	; AMX_FP16 does not enable the complex products
	use i386, x64, AMX_TILE, AMX_FP16
	tcmmimfp16ps tmm1, tmm2, tmm3

else if FAIL = 4	; distinct tiles
	use everything
	tcmmrlfp16ps tmm1, tmm2, tmm1

else if FAIL = 5	; 64-bit mode only
	use32
	use everything
	tdpfp16ps tmm1, tmm2, tmm3

else if FAIL = 6	; tile registers only
	use everything
	tdpfp16ps tmm1, tmm2, zmm3

end if
