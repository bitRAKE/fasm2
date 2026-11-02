; redefinition.asm -- mnemonics shadowed by a second definition (fixes G12, G13)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 redefinition.asm redef.bin
;
; The fasmg rule behind both defects
;	A symbol defined exactly once (constant, macro, CALM instruction) is
;	global: it can be used before its definition, at the cost of a pass.
;	A second definition turns it into a variable: each use sees the
;	definition in effect at that point, and forward uses fail. For
;	mnemonics that means the later definition silently wins for all code
;	that follows -- including all user code after the includes.
;
; Failure modes resolved
;	G12 avx.inc listed vpmuldq in two iterate blocks. The later one is the
;	    EVEX-only qword group, so the VEX forms (AVX, AVX2) were refused:
;	    use x64, AVX2 / vpmuldq ymm1, ymm2, ymm3  ->  'invalid operand'.
;	G13 avx.inc defined vcvtph2ps and vcvtps2ph twice: a VEX+EVEX version
;	    without an F16C gate, then an F16C VEX-only version that won, so
;	    the AVX-512 forms (zmm, masks, {sae}) were refused. Now there is one
;	    definition: VEX needs F16C, EVEX needs AVX512F.
;	Also: vpmullq was listed both in the AVX512F qword group and, later,
;	with its correct AVX512DQ gate; the dead first listing is removed.
;	tests/iset/_dupnames.py now fails on any new name defined twice.
;
; FAIL 1: 'F16C required'
; FAIL 2: 'invalid operand'
; FAIL 3: 'AVX2 or higher required'
; FAIL 4: 'AVX512DQ or higher required'

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

	; G12
	expect <0C4h,0E2h,6Dh,28h,0CBh>, {i386, x64, AVX2} vpmuldq ymm1, ymm2, ymm3
	expect <0C4h,0E2h,69h,28h,0CBh>, {i386, x64, AVX} vpmuldq xmm1, xmm2, xmm3
	expect <62h,0F2h,0EDh,48h,28h,0CBh>, {everything} vpmuldq zmm1, zmm2, zmm3
	expect <62h,0F2h,0EDh,19h,28h,08h>, {everything} vpmuldq xmm1{k1}, xmm2, [rax]{1to2}
	expect <62h,0F2h,0EDh,48h,40h,0CBh>, {everything} vpmullq zmm1, zmm2, zmm3	; group neighbour

	; G13
	expect <0C4h,0E2h,7Dh,13h,0CAh>, {i386, x64, AVX, F16C} vcvtph2ps ymm1, xmm2
	expect <0C4h,0E3h,7Dh,1Dh,0D1h,04h>, {i386, x64, AVX, F16C} vcvtps2ph xmm1, ymm2, 4
	expect <62h,0F2h,7Dh,48h,13h,0CAh>, {i386, x64, AVX512F} vcvtph2ps zmm1, ymm2
	; {sae} with registers: fasm2 writes EVEX.L'L as the vector length (llvm-mc 00)
	expect <62h,0F2h,7Dh,58h,13h,0CAh>, {everything} vcvtph2ps zmm1, ymm2, {sae}
	expect <62h,0F3h,7Dh,48h,1Dh,0D1h,04h>, {everything} vcvtps2ph ymm1, zmm2, 4
	expect <0C4h,0E2h,7Dh,13h,0CAh>, {everything} vcvtph2ps ymm1, xmm2	; VEX when possible

else if FAIL = 1	; neither F16C nor AVX-512
	use i386, x64, AVX
	vcvtph2ps ymm1, xmm2

else if FAIL = 2	; F16C alone has no zmm form
	use i386, x64, AVX, F16C
	vcvtph2ps zmm1, ymm2

else if FAIL = 3	; 256-bit vpmuldq needs AVX2
	use i386, x64, AVX
	vpmuldq ymm1, ymm2, ymm3

else if FAIL = 4	; vpmullq is AVX512DQ, not AVX512F
	use i386, x64, AVX512F
	vpmullq zmm1, zmm2, zmm3

end if
