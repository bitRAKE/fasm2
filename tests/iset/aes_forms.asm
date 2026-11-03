; aes_forms.asm -- aesimc and vaeskeygenassist assemble (G26, G27)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc
;	and the SDM); a mismatch stops assembly with 'encoding mismatch'.
;	By hand:  fasm2 aes_forms.asm aes_forms.bin
;
; What is resolved
;	aes.inc's iterate blocks name their opcode parameter supp in one and
;	opcode in the other, and two macros used the other block's name:
;	aesimc (SSE) failed with "symbol 'supp' is undefined" and
;	vaeskeygenassist (AVX) with "symbol 'opcode' is undefined", for
;	every operand. Upstream's "correction in AVX AES instructions"
;	(d4be069) fixed the same swap in vaesenc/vaesdec.

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

	expect <66h,0Fh,38h,0DBh,0C1h>, aesimc xmm0, xmm1
	expect <66h,0Fh,38h,0DBh,00h>, aesimc xmm0, [rax]
	expect <0C4h,0E2h,79h,0DBh,0C1h>, vaesimc xmm0, xmm1
	expect <66h,0Fh,3Ah,0DFh,0C1h,01h>, aeskeygenassist xmm0, xmm1, 1
	expect <0C4h,0E3h,79h,0DFh,0C1h,01h>, vaeskeygenassist xmm0, xmm1, 1
	expect <0C4h,0E3h,79h,0DFh,00h,1Bh>, vaeskeygenassist xmm0, [rax], 1Bh
	expect <66h,0Fh,38h,0DCh,0C1h>, aesenc xmm0, xmm1
	expect <0C4h,0E2h,71h,0DCh,0C2h>, vaesenc xmm0, xmm1, xmm2
