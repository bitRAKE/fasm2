; sse3_lddqu.asm -- lddqu parses its destination as the destination (G23)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: GNU as
;	2.47, llvm-mc, XED); a mismatch stops assembly with 'encoding mismatch'.
;	By hand:  fasm2 sse3_lddqu.asm sse3_lddqu.bin
;
; What is resolved
;	sse3.inc parsed lddqu's destination with SSE.parse_operand@src, so
;	@dest kept whatever the previous SSE instruction had left: first in a
;	source, lddqu failed with 'invalid expression'; after addps xmm5, xmm1,
;	lddqu xmm0, [rax] assembled as lddqu xmm5, [rax] (F2 0F F0 28), no
;	error. Found by tests/iset/_apx_gas.py. The first line below is first
;	in its source on purpose.

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

expect <0F2h,0Fh,0F0h,00h>, lddqu xmm0, [rax]
expect <0Fh,58h,0E9h>, addps xmm5, xmm1
expect <0F2h,0Fh,0F0h,00h>, lddqu xmm0, [rax]
expect <0F2h,44h,0Fh,0F0h,3Ch,48h>, lddqu xmm15, [rax+rcx*2]
expect <0F2h,0D5h,90h,0F0h,00h>, lddqu xmm0, [r16]
