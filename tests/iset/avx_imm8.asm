; avx_imm8.asm -- an imm8 operand must be an immediate (G15)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc
;	and the SDM); a mismatch stops assembly with 'encoding mismatch'. The
;	failure cases at the end are each assembled alone by _run.ps1
;	(-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 avx_imm8.asm avx_imm8.bin
;
; What is resolved
;	AVX_512.single_source_instruction_imm8 (vroundpd/ps, vaeskeygenassist,
;	vpcmp[ei]str[im]) and vcvtps2ph never checked that the imm8
;	operand is an immediate. After a line that left @aux.imm defined,
;	vroundpd xmm0, xmm1, [rax] assembled with that stale value as imm8:
;	the bytes of vroundpd xmm0, xmm1, 3, no error. Each FAIL case below
;	follows a valid line, so the stale value is there to be misused.
;	tests/iset/_imm_probe.py runs the same check over every mnemonic.
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

	expect <0C4h,0E3h,79h,09h,0C1h,03h>, vroundpd xmm0, xmm1, 3
	expect <0C4h,0E3h,79h,09h,00h,04h>, vroundpd xmm0, [rax], 4
	expect <62h,0F3h,0FDh,48h,00h,0C1h,1Bh>, vpermq zmm0, zmm1, 1Bh
	expect <0C4h,0E3h,79h,1Dh,0C8h,02h>, vcvtps2ph xmm0, xmm1, 2
	expect <0C4h,0E3h,79h,1Dh,08h,02h>, vcvtps2ph [rax], xmm1, 2

else if FAIL = 1
	vroundpd xmm0, xmm1, 3
	vroundpd xmm0, xmm1, [rax]

else if FAIL = 2
	vpcmpestri xmm0, xmm1, 5
	vpcmpestri xmm0, xmm1, [rax]

else if FAIL = 3
	vcvtps2ph xmm0, xmm1, 2
	vcvtps2ph xmm0, xmm1, [rax]

else if FAIL = 4
	vaeskeygenassist xmm0, xmm1, 1
	vaeskeygenassist xmm0, xmm1, [rax]

end if
