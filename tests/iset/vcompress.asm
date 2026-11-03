; vcompress.asm -- vcompressps/vcompresspd are in map 2 (G22)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc;
;	decoded with XED and ZydisInfo); a mismatch stops assembly with
;	'encoding mismatch'.
;	By hand:  fasm2 vcompress.asm vcompress.bin
;
; What is resolved
;	vcompressps and vcompresspd were declared VEX_66_0F_W0/W1 (map 1)
;	instead of VEX_66_0F38 (map 2): vcompresspd [rax], xmm0 assembled as
;	62 F1 FD 08 8A 00, which XED cannot decode (#UD). vpcompressd/q were
;	right.

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

expect <62h,0F2h,0FDh,08h,8Ah,00h>, vcompresspd [rax], xmm0
expect <62h,0F2h,0FDh,48h,8Ah,00h>, vcompresspd [rax], zmm0
expect <62h,0E2h,7Dh,08h,8Ah,00h>, vcompressps [rax], xmm16
expect <62h,0F2h,0FDh,29h,8Ah,00h>, vcompresspd [rax]{k1}, ymm0
expect <62h,0F2h,7Dh,48h,8Ah,0D1h>, vcompressps zmm1, zmm2
expect <62h,0F2h,7Dh,48h,8Bh,00h>, vpcompressd [rax], zmm0