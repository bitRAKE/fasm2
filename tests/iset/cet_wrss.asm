; cet_wrss.asm -- WRSS/WRUSS take a memory destination only (G19)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc
;	and the SDM); a mismatch stops assembly with 'encoding mismatch'. The
;	failure cases at the end are each assembled alone by _run.ps1
;	(-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 cet_wrss.asm cet_wrss.bin
;
; What is resolved
;	cet.inc accepted a register destination: wrssq rax, r8 assembled as
;	4C 0F 38 F6 C0 (ModRM.mod = 11). The SDM defines only WRSSD/WRSSQ
;	m32/m64, r and WRUSSD/WRUSSQ m32/m64, r. The APX sweep found it once
;	r16-r31 reached these instructions through EVEX map 4.
;
; FAIL 1: 'invalid combination of operands'
; FAIL 2: 'invalid combination of operands'
; FAIL 3: 'invalid combination of operands'
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

use64
use everything

if ~ defined FAIL

	expect <0Fh,38h,0F6h,00h>, wrssd [rax], eax
	expect <4Ch,0Fh,38h,0F6h,00h>, wrssq [rax], r8
	expect <66h,0Fh,38h,0F5h,00h>, wrussd [rax], eax
	expect <66h,4Ch,0Fh,38h,0F5h,00h>, wrussq [rax], r8

else if FAIL = 1
	wrssd eax, ecx

else if FAIL = 2
	wrssq rax, r8

else if FAIL = 3
	wrussd eax, ecx

else if FAIL = 4
	wrussq rax, r8

end if