; avx_operand_state.asm -- vpinsrq and vlddqu parse their memory/GPR operand fully (G20)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'.
;	By hand:  fasm2 avx_operand_state.asm avx_operand_state.bin
;
; What is resolved
;	vpinsrq (its GPR/memory operand) and vlddqu (its memory operand) were
;	parsed with x86.parse_operand instead of AVX_512.parse_operand, so the
;	operand's mask, evex_b and memsize were never set. In a source where no
;	earlier AVX instruction had set them, both failed with 'invalid
;	expression'; after one, they used whatever it had left. Each line below
;	is assembled first in its source on purpose.

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

expect <0C4h,0E3h,0F1h,22h,0C0h,01h>, vpinsrq xmm0, xmm1, rax, 1
expect <0C5h,0FBh,0F0h,00h>, vlddqu xmm0, [rax]
expect <0C4h,0E3h,0F1h,22h,03h,01h>, vpinsrq xmm0, xmm1, [rbx], 1
expect <62h,0C3h,0F5h,08h,22h,0C1h,00h>, vpinsrq xmm16, xmm1, r9, 0
expect <0C4h,0C1h,7Fh,0F0h,04h,48h>, vlddqu ymm0, [r8+rcx*2]