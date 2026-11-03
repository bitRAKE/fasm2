; vpbroadcastq.asm -- EVEX vpbroadcastq is W1; masked xmm/ymm need AVX512VL (G21)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc;
;	decoded with XED and ZydisInfo); a mismatch stops assembly with
;	'encoding mismatch'. The failure case is assembled alone by _run.ps1
;	(-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 vpbroadcastq.asm vpbroadcastq.bin
;
; What is resolved
;	vpbroadcastq from xmm/m64 used VEX_66_0F38_W0 for both encodings. VEX
;	is W0, but EVEX.66.0F38.W0 59 is vbroadcasti32x2 (AVX512DQ): fasm2's
;	vpbroadcastq zmm0, [rax] decoded as vbroadcasti32x2 zmm0, [rax], which
;	loads the same 8 bytes but masks per dword. The EVEX form is now W1.
;	vpbroadcastd/q xmm/ymm with a mask also did not require AVX512VL.
;
; FAIL 1: 'AVX512VL or higher required'

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

	use everything
	expect <62h,0F2h,0FDh,48h,59h,00h>, vpbroadcastq zmm0, [rax]
	expect <62h,0F2h,0FDh,48h,59h,0C1h>, vpbroadcastq zmm0, xmm1
	expect <62h,0E2h,0FDh,08h,59h,00h>, vpbroadcastq xmm16, [rax]
	expect <62h,0F2h,0FDh,09h,59h,00h>, vpbroadcastq xmm0{k1}, [rax]
	expect <0C4h,0E2h,79h,59h,00h>, vpbroadcastq xmm0, [rax]	; VEX stays W0
	expect <0C4h,0E2h,7Dh,59h,0C1h>, vpbroadcastq ymm0, xmm1
	expect <62h,0F2h,7Dh,48h,58h,00h>, vpbroadcastd zmm0, [rax]
	expect <62h,0F2h,7Dh,09h,58h,0C1h>, vpbroadcastd xmm0{k1}, xmm1

else if FAIL = 1
	use i386, x64, AVX512F
	vpbroadcastd xmm0{k1}, xmm1

end if