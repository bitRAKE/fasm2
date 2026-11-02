; bf16.asm -- bfloat16 instructions in VEX and EVEX form (iset/bf16.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 bf16.asm bf16.bin
;
; What is new
;	AVX512_BF16 (7.1:EAX[5]: vcvtne2ps2bf16, vdpbf16ps, vcvtneps2bf16) and
;	AVX-NE-CONVERT (7.1:EDX[5]: VEX vcvtneps2bf16 plus six bf16/fp16 loads)
;	were missing. Failure modes this file guards against:
;	- vcvtneps2bf16 is in both features; each gate alone must give its own
;	  encoding, and VEX-only must refuse masks, broadcast and zmm;
;	- it halves the vector, so a memory source with an xmm destination is
;	  ambiguous (128 or 256 bits) and needs an explicit size;
;	- the AVX-NE-CONVERT loads take memory only, of the right size.
;
; FAIL 1: 'AVX512_BF16 or higher required'
; FAIL 2: 'AVX_NE_CONVERT or AVX512_BF16 required'
; FAIL 3: 'invalid operand'
; FAIL 4: 'operand size not specified'
; FAIL 5: 'invalid operand size'
; FAIL 6: 'invalid combination of operands'
; FAIL 7: 'invalid operand size'
; FAIL 8: 'AVX_NE_CONVERT or higher required'
; FAIL 9: 'invalid operand'

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

	; AVX512_BF16
	expect <62h,0F2h,6Fh,48h,72h,0CBh>, {i386, x64, AVX512F, AVX512_BF16} vcvtne2ps2bf16 zmm1, zmm2, zmm3
	expect <62h,0F2h,6Fh,89h,72h,0CBh>, {i386, x64, AVX512F, AVX512VL, AVX512_BF16} vcvtne2ps2bf16 xmm1{k1}{z}, xmm2, xmm3
	expect <62h,0F2h,6Fh,58h,72h,08h>, {i386, x64, AVX512F, AVX512_BF16} vcvtne2ps2bf16 zmm1, zmm2, [rax]{1to16}
	expect <62h,0F2h,6Eh,48h,52h,0CBh>, {i386, x64, AVX512F, AVX512_BF16} vdpbf16ps zmm1, zmm2, zmm3
	expect <62h,0F2h,6Eh,38h,52h,08h>, {i386, x64, AVX512F, AVX512VL, AVX512_BF16} vdpbf16ps ymm1, ymm2, [rax]{1to8}
	expect <62h,0F2h,7Eh,48h,72h,0CAh>, {i386, x64, AVX512F, AVX512_BF16} vcvtneps2bf16 ymm1, zmm2
	expect <62h,0F2h,7Eh,29h,72h,0CAh>, {i386, x64, AVX512F, AVX512VL, AVX512_BF16} vcvtneps2bf16 xmm1{k1}, ymm2
	expect <62h,0F2h,7Eh,28h,72h,08h>, {i386, x64, AVX512F, AVX512VL, AVX512_BF16} vcvtneps2bf16 xmm1, yword [rax]
	expect <62h,0F2h,7Eh,58h,72h,08h>, {i386, x64, AVX512F, AVX512_BF16} vcvtneps2bf16 ymm1, [rax]{1to16}
	expect <62h,0F2h,7Eh,48h,72h,08h>, {i386, x64, AVX512F, AVX512_BF16} vcvtneps2bf16 ymm1, [rax]

	; AVX-NE-CONVERT
	expect <0C4h,0E2h,7Eh,72h,0CAh>, {i386, x64, AVX2, AVX_NE_CONVERT} vcvtneps2bf16 xmm1, ymm2
	expect <0C4h,0E2h,7Ah,72h,0CAh>, {i386, x64, AVX2, AVX_NE_CONVERT} vcvtneps2bf16 xmm1, xmm2
	expect <0C4h,0E2h,7Ah,72h,08h>, {i386, x64, AVX2, AVX_NE_CONVERT} vcvtneps2bf16 xmm1, dqword [rax]
	expect <0C4h,0E2h,7Eh,72h,08h>, {i386, x64, AVX2, AVX_NE_CONVERT} vcvtneps2bf16 xmm1, yword [rax]
	expect <0C4h,0E2h,7Eh,0B1h,08h>, {i386, x64, AVX2, AVX_NE_CONVERT} vbcstnebf162ps ymm1, [rax]
	expect <0C4h,0E2h,79h,0B1h,08h>, {i386, x64, AVX2, AVX_NE_CONVERT} vbcstnesh2ps xmm1, word [rax]
	expect <0C4h,0E2h,7Eh,0B0h,08h>, {i386, x64, AVX2, AVX_NE_CONVERT} vcvtneebf162ps ymm1, [rax]
	expect <0C4h,0E2h,79h,0B0h,08h>, {i386, x64, AVX2, AVX_NE_CONVERT} vcvtneeph2ps xmm1, dqword [rax]
	expect <0C4h,0E2h,7Fh,0B0h,08h>, {i386, x64, AVX2, AVX_NE_CONVERT} vcvtneobf162ps ymm1, yword [rax]
	expect <0C4h,0E2h,7Ch,0B0h,08h>, {i386, x64, AVX2, AVX_NE_CONVERT} vcvtneoph2ps ymm1, [rax]

	; both: VEX unless EVEX is needed or requested
	expect <0C4h,0E2h,7Eh,72h,0CAh>, {everything} vcvtneps2bf16 xmm1, ymm2
	expect <62h,0F2h,7Eh,28h,72h,0CAh>, {everything, evex} vcvtneps2bf16 xmm1, ymm2
	expect <62h,0F2h,7Eh,29h,72h,0CAh>, {everything} vcvtneps2bf16 xmm1{k1}, ymm2

else if FAIL = 1	; vdpbf16ps is AVX512_BF16 only
	use i386, x64, AVX2, AVX_NE_CONVERT
	vdpbf16ps ymm1, ymm2, ymm3

else if FAIL = 2	; neither feature
	use i386, x64, AVX2
	vcvtneps2bf16 xmm1, ymm2

else if FAIL = 3	; AVX_NE_CONVERT only: no masks
	use i386, x64, AVX2, AVX_NE_CONVERT
	vcvtneps2bf16 xmm1{k1}, ymm2

else if FAIL = 4	; xmm destination: 128- or 256-bit source?
	use everything
	vcvtneps2bf16 xmm1, [rax]

else if FAIL = 5	; the destination is half the source
	use everything
	vcvtneps2bf16 ymm1, ymm2

else if FAIL = 6	; the loads take memory only
	use i386, x64, AVX2, AVX_NE_CONVERT
	vcvtneebf162ps ymm1, ymm2

else if FAIL = 7	; broadcast loads read one word
	use i386, x64, AVX2, AVX_NE_CONVERT
	vbcstnebf162ps ymm1, dword [rax]

else if FAIL = 8	; the loads are AVX_NE_CONVERT only
	use everything
	use i386, x64, AVX512F, AVX512_BF16
	vcvtneoph2ps ymm1, [rax]

else if FAIL = 9	; the loads are VEX only: no zmm
	use everything
	vcvtneebf162ps zmm1, [rax]

end if
