; avx512_fp16.asm -- AVX512-FP16 arithmetic, FMA, complex, compare, moves (iset/avx512_fp16.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc,
;	operand shapes from zydis-db); a mismatch stops assembly with 'encoding
;	mismatch'. The failure cases at the end are each assembled alone by
;	_run.ps1 (-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 avx512_fp16.asm avx512_fp16.bin
;
; What is new
;	AVX512_FP16 (7.0:EDX[23]) was missing (conversions: avx512_fp16_cvt.asm).
;	Failure modes this file guards against:
;	- EVEX maps 5 and 6, which nothing used before;
;	- element size 2: {1to8/16/32} broadcast (needs fix G5), disp8 scaled
;	  by 2 for scalar and by the vector for packed memory;
;	- complex forms broadcast 32-bit pairs and require a destination
;	  distinct from the sources;
;	- {er}/{sae}, masking, compare predicates and their aliases;
;	- EVEX only: xmm/ymm forms need AVX512VL, every form AVX512F.
;
; FAIL 1: 'AVX512_FP16 required'
; FAIL 2: 'AVX512VL or higher required'
; FAIL 3: 'destination must differ from the sources'
; FAIL 4: 'invalid operand size'
; FAIL 5: 'invalid combination of operands'
; FAIL 6: 'invalid operand size'

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

	expect <62h,0F5h,6Ch,48h,58h,0CBh>, {i386, x64, AVX512F, AVX512_FP16} vaddph zmm1, zmm2, zmm3
	expect <62h,0F5h,6Ch,89h,58h,0CBh>, {everything} vaddph xmm1{k1}{z}, xmm2, xmm3
	expect <62h,0F5h,6Ch,58h,58h,08h>, {everything} vaddph zmm1, zmm2, [rax]{1to32}
	expect <62h,0F5h,6Ch,18h,58h,0CBh>, {everything} vaddph zmm1, zmm2, zmm3, {rn-sae}
	expect <62h,0F5h,6Eh,08h,58h,0CBh>, {i386, x64, AVX512F, AVX512_FP16} vaddsh xmm1, xmm2, xmm3
	expect <62h,0F5h,6Eh,08h,58h,48h,01h>, {everything} vaddsh xmm1, xmm2, [rax+2]
	expect <62h,0F5h,6Eh,78h,58h,0CBh>, {everything} vaddsh xmm1, xmm2, xmm3, {rz-sae}
	expect <62h,0F5h,6Ch,28h,5Ch,0CBh>, {everything} vsubph ymm1, ymm2, ymm3
	expect <62h,0F5h,6Eh,08h,59h,0CBh>, {everything} vmulsh xmm1, xmm2, xmm3
	expect <62h,0F5h,6Ch,48h,5Eh,48h,01h>, {everything} vdivph zmm1, zmm2, [rax+64]
	; {sae} with registers: fasm2 sets EVEX.L'L to 512 as for vminps (llvm-mc writes 00);
	; the CPU ignores L'L here and XED decodes both
	expect <62h,0F5h,6Ch,58h,5Dh,0CBh>, {everything} vminph zmm1, zmm2, zmm3, {sae}
	expect <62h,0F5h,6Eh,08h,5Fh,0CBh>, {everything} vmaxsh xmm1, xmm2, xmm3
	expect <62h,0F5h,7Ch,48h,51h,0CAh>, {everything} vsqrtph zmm1, zmm2
	expect <62h,0F5h,7Ch,38h,51h,08h>, {everything} vsqrtph ymm1, [rax]{1to16}
	expect <62h,0F5h,6Eh,08h,51h,0CBh>, {everything} vsqrtsh xmm1, xmm2, xmm3
	expect <62h,0F6h,6Dh,48h,98h,0CBh>, {everything} vfmadd132ph zmm1, zmm2, zmm3
	expect <62h,0F6h,6Dh,08h,0B9h,0CBh>, {everything} vfmadd231sh xmm1, xmm2, xmm3
	expect <62h,0F6h,6Dh,18h,0AEh,08h>, {everything} vfnmsub213ph xmm1, xmm2, [rax]{1to8}
	expect <62h,0F6h,6Dh,28h,0B6h,0CBh>, {everything} vfmaddsub231ph ymm1, ymm2, ymm3
	expect <62h,0F6h,6Dh,48h,97h,0CBh>, {everything} vfmsubadd132ph zmm1, zmm2, zmm3
	expect <62h,0F6h,6Eh,48h,0D6h,0CBh>, {everything} vfmulcph zmm1, zmm2, zmm3
	expect <62h,0F6h,6Fh,58h,0D6h,08h>, {everything} vfcmulcph zmm1, zmm2, [rax]{1to16}
	expect <62h,0F6h,6Eh,08h,56h,0CBh>, {everything} vfmaddcph xmm1, xmm2, xmm3
	expect <62h,0F6h,6Fh,08h,57h,0CBh>, {everything} vfcmaddcsh xmm1, xmm2, xmm3
	expect <62h,0F6h,6Eh,08h,0D7h,48h,01h>, {everything} vfmulcsh xmm1, xmm2, [rax+4]
	expect <62h,0F6h,6Dh,48h,2Ch,0CBh>, {everything} vscalefph zmm1, zmm2, zmm3
	expect <62h,0F6h,6Dh,08h,2Dh,0CBh>, {everything} vscalefsh xmm1, xmm2, xmm3
	expect <62h,0F6h,7Dh,48h,4Ch,0CAh>, {everything} vrcpph zmm1, zmm2
	expect <62h,0F6h,6Dh,08h,4Fh,0CBh>, {everything} vrsqrtsh xmm1, xmm2, xmm3
	expect <62h,0F6h,7Dh,48h,42h,0CAh>, {everything} vgetexpph zmm1, zmm2
	expect <62h,0F6h,6Dh,18h,43h,0CBh>, {everything} vgetexpsh xmm1, xmm2, xmm3, {sae}
	expect <62h,0F3h,7Ch,48h,26h,0CAh,04h>, {everything} vgetmantph zmm1, zmm2, 4
	expect <62h,0F3h,6Ch,08h,27h,0CBh,04h>, {everything} vgetmantsh xmm1, xmm2, xmm3, 4
	expect <62h,0F3h,7Ch,58h,56h,08h,04h>, {everything} vreduceph zmm1, [rax]{1to32}, 4
	expect <62h,0F3h,6Ch,08h,0Ah,0CBh,04h>, {everything} vrndscalesh xmm1, xmm2, xmm3, 4
	expect <62h,0F3h,6Ch,48h,0C2h,0CBh,05h>, {everything} vcmpph k1, zmm2, zmm3, 5
	expect <62h,0F3h,6Ch,1Ah,0C2h,08h,05h>, {everything} vcmpph k1{k2}, xmm2, [rax]{1to8}, 5
	expect <62h,0F3h,6Eh,08h,0C2h,0CBh,01h>, {everything} vcmpsh k1, xmm2, xmm3, 1
	expect <62h,0F3h,6Ch,48h,0C2h,0CBh,01h>, {everything} vcmpltph k1, zmm2, zmm3
	expect <62h,0F3h,6Eh,08h,0C2h,0CBh,0Ch>, {everything} vcmpneq_oqsh k1, xmm2, xmm3
	expect <62h,0F5h,7Ch,08h,2Fh,0CAh>, {everything} vcomish xmm1, xmm2
	expect <62h,0F5h,7Ch,08h,2Eh,48h,01h>, {everything} vucomish xmm1, [rax+2]
	expect <62h,0F5h,7Ch,18h,2Fh,0CAh>, {everything} vcomish xmm1, xmm2, {sae}
	expect <62h,0F3h,7Ch,48h,66h,08h,03h>, {everything} vfpclassph k1, zword [rax], 3
	expect <62h,0F3h,7Ch,28h,66h,0CAh,03h>, {everything} vfpclassph k1, ymm2, 3
	expect <62h,0F3h,7Ch,08h,67h,0CAh,03h>, {everything} vfpclasssh k1, xmm2, 3
	expect <62h,0F5h,7Eh,08h,10h,08h>, {everything} vmovsh xmm1, [rax]
	expect <62h,0F5h,7Eh,08h,11h,48h,01h>, {everything} vmovsh [rax+2], xmm1
	expect <62h,0F5h,6Eh,09h,10h,0CBh>, {everything} vmovsh xmm1{k1}, xmm2, xmm3
	expect <62h,0F5h,7Dh,08h,6Eh,0C8h>, {everything} vmovw xmm1, eax
	expect <62h,0F5h,7Dh,08h,6Eh,08h>, {everything} vmovw xmm1, [rax]
	expect <62h,0F5h,7Dh,08h,7Eh,0C8h>, {everything} vmovw eax, xmm1
	expect <62h,0F5h,7Dh,08h,7Eh,48h,01h>, {everything} vmovw [rax+2], xmm1

else if FAIL = 1	; AVX512F alone is not enough
	use i386, x64, AVX512F, AVX512VL, AVX512BW
	vaddph zmm1, zmm2, zmm3

else if FAIL = 2	; xmm/ymm forms need AVX512VL
	use i386, x64, AVX512F, AVX512_FP16
	vaddph ymm1, ymm2, ymm3

else if FAIL = 3	; complex: destination distinct from sources
	use everything
	vfmulcph zmm1, zmm1, zmm3

else if FAIL = 4	; scalar memory is 16-bit
	use everything
	vaddsh xmm1, xmm2, dword [rax]

else if FAIL = 5	; vfpclassph's destination is a mask register
	use everything
	vfpclassph xmm1, xmm2, 3

else if FAIL = 6	; vmovw moves a word: r32 or m16
	use everything
	vmovw xmm1, rax

end if
