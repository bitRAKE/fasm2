; avx512_fp16_cvt.asm -- AVX512-FP16 conversions (iset/avx512_fp16.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc,
;	operand shapes from zydis-db); a mismatch stops assembly with 'encoding
;	mismatch'. The failure cases at the end are each assembled alone by
;	_run.ps1 (-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 avx512_fp16_cvt.asm avx512_fp16_cvt.bin
;
; What is new
;	The 34 FP16 conversions. Failure modes this file guards against:
;	- element-width ratios 1, 2 and 4 in both directions: the vector
;	  register sizes and the memory size of each side follow the ratio;
;	- narrowing with a memory source into xmm is ambiguous without an
;	  explicit size (as for vcvtpd2ps);
;	- disp8 scaling by the memory operand's size (half or quarter vector);
;	- scalar forms with 16/32/64-bit memory, GPR forms with W by size.
;	{sae} with register operands: fasm2 writes EVEX.L'L as the vector
;	length (llvm-mc writes 00); the CPU ignores L'L in that form.
;
; FAIL 1: 'invalid operand size'
; FAIL 2: 'operand size not specified'
; FAIL 3: 'invalid operand size'
; FAIL 4: 'invalid operand size'
; FAIL 5: 'AVX512_FP16 required'

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

	; same width
	expect <62h,0F5h,7Dh,48h,7Dh,0CAh>, {everything} vcvtph2w zmm1, zmm2
	expect <62h,0F5h,7Ch,38h,7Dh,08h>, {everything} vcvtph2uw ymm1, [rax]{1to16}
	expect <62h,0F5h,7Eh,18h,7Dh,0CAh>, {everything} vcvtw2ph zmm1, zmm2, {rn-sae}
	expect <62h,0F5h,7Fh,09h,7Dh,0CAh>, {everything} vcvtuw2ph xmm1{k1}, xmm2
	expect <62h,0F5h,7Dh,58h,7Ch,0CAh>, {everything} vcvttph2w zmm1, zmm2, {sae}
	expect <62h,0F5h,7Ch,28h,7Ch,0CAh>, {everything} vcvttph2uw ymm1, ymm2

	; widening x2
	expect <62h,0F5h,7Dh,48h,5Bh,0CAh>, {everything} vcvtph2dq zmm1, ymm2
	expect <62h,0F5h,7Dh,08h,5Bh,0CAh>, {everything} vcvtph2dq xmm1, xmm2
	expect <62h,0F5h,7Dh,38h,5Bh,08h>, {everything} vcvtph2dq ymm1, [rax]{1to8}
	expect <62h,0F5h,7Dh,28h,5Bh,48h,01h>, {everything} vcvtph2dq ymm1, [rax+16]
	expect <62h,0F5h,7Ch,38h,79h,0CAh>, {everything} vcvtph2udq zmm1, ymm2, {rd-sae}
	expect <62h,0F5h,7Eh,58h,5Bh,0CAh>, {everything} vcvttph2dq zmm1, ymm2, {sae}
	expect <62h,0F5h,7Ch,28h,78h,0CAh>, {everything} vcvttph2udq ymm1, xmm2
	expect <62h,0F6h,7Dh,48h,13h,0CAh>, {everything} vcvtph2psx zmm1, ymm2
	expect <62h,0F6h,7Dh,08h,13h,48h,01h>, {everything} vcvtph2psx xmm1, [rax+8]

	; widening x4
	expect <62h,0F5h,7Dh,48h,7Bh,0CAh>, {everything} vcvtph2qq zmm1, xmm2
	expect <62h,0F5h,7Dh,28h,7Bh,0CAh>, {everything} vcvtph2qq ymm1, xmm2
	expect <62h,0F5h,7Dh,08h,7Bh,48h,01h>, {everything} vcvtph2qq xmm1, [rax+4]
	expect <62h,0F5h,7Dh,58h,79h,08h>, {everything} vcvtph2uqq zmm1, [rax]{1to8}
	expect <62h,0F5h,7Dh,58h,7Ah,0CAh>, {everything} vcvttph2qq zmm1, xmm2, {sae}
	expect <62h,0F5h,7Dh,08h,78h,0CAh>, {everything} vcvttph2uqq xmm1, xmm2
	expect <62h,0F5h,7Ch,48h,5Ah,0CAh>, {everything} vcvtph2pd zmm1, xmm2

	; narrowing /2
	expect <62h,0F5h,7Ch,48h,5Bh,0CAh>, {everything} vcvtdq2ph ymm1, zmm2
	expect <62h,0F5h,7Ch,28h,5Bh,0CAh>, {everything} vcvtdq2ph xmm1, ymm2
	expect <62h,0F5h,7Ch,08h,5Bh,08h>, {everything} vcvtdq2ph xmm1, dqword [rax]
	expect <62h,0F5h,7Ch,28h,5Bh,08h>, {everything} vcvtdq2ph xmm1, yword [rax]
	expect <62h,0F5h,7Fh,78h,7Ah,0CAh>, {everything} vcvtudq2ph ymm1, zmm2, {rz-sae}
	expect <62h,0F5h,7Dh,48h,1Dh,0CAh>, {everything} vcvtps2phx ymm1, zmm2

	; narrowing /4
	expect <62h,0F5h,0FCh,48h,5Bh,0CAh>, {everything} vcvtqq2ph xmm1, zmm2
	expect <62h,0F5h,0FCh,28h,5Bh,0CAh>, {everything} vcvtqq2ph xmm1, ymm2
	expect <62h,0F5h,0FCh,48h,5Bh,08h>, {everything} vcvtqq2ph xmm1, zword [rax]
	expect <62h,0F5h,0FFh,58h,7Ah,08h>, {everything} vcvtuqq2ph xmm1, [rax]{1to8}
	expect <62h,0F5h,0FDh,48h,5Ah,0CAh>, {everything} vcvtpd2ph xmm1, zmm2

	; scalar
	expect <62h,0F6h,6Ch,08h,13h,0CBh>, {everything} vcvtsh2ss xmm1, xmm2, xmm3
	expect <62h,0F6h,6Ch,18h,13h,0CBh>, {everything} vcvtsh2ss xmm1, xmm2, xmm3, {sae}
	expect <62h,0F5h,6Eh,08h,5Ah,48h,01h>, {everything} vcvtsh2sd xmm1, xmm2, [rax+2]
	expect <62h,0F5h,6Ch,08h,1Dh,0CBh>, {everything} vcvtss2sh xmm1, xmm2, xmm3
	expect <62h,0F5h,0EFh,18h,5Ah,0CBh>, {everything} vcvtsd2sh xmm1, xmm2, xmm3, {rn-sae}
	expect <62h,0F5h,0EFh,08h,5Ah,48h,01h>, {everything} vcvtsd2sh xmm1, xmm2, [rax+8]

	; general-purpose registers
	expect <62h,0F5h,7Eh,08h,2Dh,0C1h>, {everything} vcvtsh2si eax, xmm1
	expect <62h,0F5h,0FEh,08h,2Dh,0C1h>, {everything} vcvtsh2si rax, xmm1
	expect <62h,0F5h,7Eh,08h,79h,40h,01h>, {everything} vcvtsh2usi eax, [rax+2]
	expect <62h,0F5h,7Eh,18h,2Ch,0C1h>, {everything} vcvttsh2si eax, xmm1, {sae}
	expect <62h,75h,0FEh,08h,78h,0C1h>, {everything} vcvttsh2usi r8, xmm1
	expect <62h,0F5h,6Eh,08h,2Ah,0C8h>, {everything} vcvtsi2sh xmm1, xmm2, eax
	expect <62h,0F5h,0EEh,08h,2Ah,0C8h>, {everything} vcvtsi2sh xmm1, xmm2, rax
	expect <62h,0F5h,6Eh,08h,2Ah,08h>, {everything} vcvtsi2sh xmm1, xmm2, dword [rax]

else if FAIL = 1	; widening x2: the source is half the destination
	use everything
	vcvtph2dq zmm1, zmm2

else if FAIL = 2	; narrowing into xmm from memory: which size?
	use everything
	vcvtqq2ph xmm1, [rax]

else if FAIL = 3	; narrowing /4 never fills a ymm
	use everything
	vcvtpd2ph ymm1, zmm2

else if FAIL = 4	; scalar memory size
	use everything
	vcvtsd2sh xmm1, xmm2, dword [rax]

else if FAIL = 5
	use i386, x64, AVX512F, AVX512VL
	vcvtph2dq zmm1, ymm2

end if
