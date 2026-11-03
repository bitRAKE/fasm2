; crc32.asm -- CRC32 r64, r/m8 with REX.W (G18)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc
;	and the SDM); a mismatch stops assembly with 'encoding mismatch'.
;	By hand:  fasm2 crc32.asm crc32.bin
;
; What is resolved
;	crc32 with a 64-bit destination and an 8-bit source wrote no REX.W:
;	crc32 rbx, al was F2 0F 38 F0 D8, the encoding of crc32 ebx, al. The
;	result is the same (a 32-bit destination is zero-extended), but the
;	SDM's form is F2 REX.W 0F 38 F0, as llvm-mc writes it, and APX's EVEX
;	form carries the same W.

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

expect <0F2h,48h,0Fh,38h,0F0h,0D8h>, crc32 rbx, al
expect <0F2h,48h,0Fh,38h,0F0h,18h>, crc32 rbx, byte [rax]
expect <0F2h,4Ch,0Fh,38h,0F0h,0C0h>, crc32 r8, al
expect <0F2h,0Fh,38h,0F0h,0D8h>, crc32 ebx, al
expect <0F2h,48h,0Fh,38h,0F1h,0D8h>, crc32 rbx, rax
expect <66h,0F2h,0Fh,38h,0F1h,0D8h>, crc32 ebx, ax