; cmpccxadd.asm -- CMPccXADD for every condition code (iset/cmpccxadd.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc,
;	including its alias table); a mismatch stops assembly with 'encoding
;	mismatch'. The failure cases at the end are each assembled alone by
;	_run.ps1 (-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 cmpccxadd.asm cmpccxadd.bin
;
; What is new
;	CMPccXADD (7.1:EAX[7]) was missing. Failure modes this file guards
;	against:
;	- 16 conditions plus 14 aliases must each map to E0+cc;
;	- operand roles: memory in ModRM.rm, first register in ModRM.reg,
;	  second register in VEX.vvvv; VEX.W follows the operand size;
;	- 64-bit only; no AVX needed (VEX_GPR, fix G7).
;
; FAIL 1: 'CMPCCXADD or higher required'
; FAIL 2: 'bits64 or higher required'
; FAIL 3: 'invalid combination of operands'
; FAIL 4: 'invalid operand size'
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

	iterate <cc,code>, o,0E0h, no,0E1h, b,0E2h, c,0E2h, nae,0E2h, nb,0E3h, nc,0E3h, ae,0E3h, \
			   z,0E4h, e,0E4h, nz,0E5h, ne,0E5h, be,0E6h, na,0E6h, nbe,0E7h, a,0E7h, \
			   s,0E8h, ns,0E9h, p,0EAh, pe,0EAh, np,0EBh, po,0EBh, l,0ECh, nge,0ECh, \
			   nl,0EDh, ge,0EDh, le,0EEh, ng,0EEh, nle,0EFh, g,0EFh
		expect <0C4h,0E2h,79h,code,0Ah>, {i386, x64, CMPCCXADD} cmp#cc#xadd [rdx], ecx, eax
	end iterate

	expect <0C4h,42h,0B9h,0E6h,0Ah>, {i386, x64, CMPCCXADD} cmpbexadd [r10], r9, r8
	expect <0C4h,0E2h,79h,0E6h,0Ah>, {i386, x64, CMPCCXADD} cmpbexadd dword [rdx], ecx, eax
	expect <0C4h,0E2h,79h,0E6h,0Ah>, {everything} cmpbexadd [rdx], ecx, eax

else if FAIL = 1
	use i386, x64, AVX2
	cmpexadd [rdx], ecx, eax

else if FAIL = 2
	use32
	cmpexadd [edx], ecx, eax

else if FAIL = 3	; memory destination
	cmpexadd edx, ecx, eax

else if FAIL = 4	; registers of one size, 32 or 64 bits
	cmpexadd [rdx], ecx, rax

else if FAIL = 5
	cmpexadd qword [rdx], ecx, eax

end if
