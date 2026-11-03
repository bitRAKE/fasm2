; cmpxchg16b.asm -- CMPXCHG16B with r8-r15 in the address (G17)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases at
;	the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must stop
;	with the quoted message. By hand:  fasm2 cmpxchg16b.asm cmpxchg16b.bin
;
; What is resolved
;	cmpxchg16b wrote its REX.W as a separate byte before the instruction's
;	own REX: cmpxchg16b [r8] was 48 41 0F C7 08. Only a REX directly before
;	the opcode counts, so the processor ignored the 48h and executed
;	cmpxchg8b [r8], an 8-byte compare-exchange where 16 bytes were meant.
;	With rax-rdi in the address there was no second REX and the bytes were
;	right. W now goes into the one REX (or REX2) with the address bits.
;
; FAIL 1: 'bits64 or higher required'
; FAIL 2: 'invalid operand size'

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

	expect <48h,0Fh,0C7h,08h>, cmpxchg16b [rax]
	expect <49h,0Fh,0C7h,08h>, cmpxchg16b [r8]
	expect <4Bh,0Fh,0C7h,0Ch,48h>, cmpxchg16b [r8+r9*2]
	expect <4Ah,0Fh,0C7h,0Ch,38h>, cmpxchg16b [rax+r15]
	expect <49h,0Fh,0C7h,0Fh>, cmpxchg16b dqword [r15]
	expect <0F0h,49h,0Fh,0C7h,08h>, lock cmpxchg16b [r8]
	; cmpxchg8b keeps its bytes
	expect <41h,0Fh,0C7h,08h>, cmpxchg8b [r8]

else if FAIL = 1	; REX.W exists only in long mode
	use32
	cmpxchg16b [eax]

else if FAIL = 2	; m128 only
	cmpxchg16b qword [rax]

end if
