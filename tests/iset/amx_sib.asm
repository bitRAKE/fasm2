; amx_sib.asm -- a {settings} prefix on an AMX tile load/store does not leak {sib} (G24)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: GNU as
;	2.47, llvm-mc, XED); a mismatch stops assembly with 'encoding mismatch'.
;	By hand:  fasm2 amx_sib.asm amx_sib.bin
;
; What is resolved
;	AMX.parse_sib_operand forces a SIB byte with TAKE / COMPUTE / TAKE on
;	x86.usesib. Under a {settings} prefix (which pushes a copy of every
;	setting and pops it after the instruction) the first TAKE popped that
;	copy, COMPUTE overwrote the saved value with 1, and the prefix's
;	restore left x86.usesib = 1: every later memory operand got a SIB byte
;	(lea eax, [rax] = 8D 04 20). Valid bytes, but not the ones asked for.
;	Found by tests/iset/_apx_gas.py ({evex} tilestored in LLVM's APX
;	corpus, then lea r16d, [rax]).

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

expect <0C4h,0E2h,7Bh,4Bh,0Ch,20h>, {vex3} tileloadd tmm1, [rax]
expect <8Dh,00h>, lea eax, [rax]
expect <62h,0F2h,7Eh,08h,4Bh,74h,98h,7Bh>, {evex} tilestored [rax+4*rbx+123], tmm6
expect <0D5h,40h,8Dh,00h>, lea r16d, [rax]
expect <0C4h,0E2h,7Bh,4Bh,0Ch,20h>, {imm8} tileloadd tmm1, [rax]
expect <03h,03h>, add eax, [rbx]
expect <0C4h,0E2h,7Bh,4Bh,0Ch,20h>, tileloadd tmm1, [rax]
expect <8Dh,00h>, lea eax, [rax]
expect <8Dh,04h,20h>, {sib} lea eax, [rax]
expect <8Dh,00h>, lea eax, [rax]
