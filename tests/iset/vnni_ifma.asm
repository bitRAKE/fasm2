; vnni_ifma.asm -- VNNI and IFMA in VEX and EVEX form (fix G6; iset/vnni.inc, iset/ifma.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 vnni_ifma.asm vnni_ifma.bin
;
; Failure modes resolved
;	G6  vpdpbusd/vpdpbusds/vpdpwssd/vpdpwssds and vpmadd52luq/vpmadd52huq
;	    were EVEX-only (AVX512_VNNI, AVX512_IFMA). AVX-VNNI (7.1:EAX[4]) and
;	    AVX-IFMA (7.1:EAX[23]) parts, e.g. Alder Lake, have only the VEX
;	    forms, so  use AVX2, AVX_VNNI / vpdpbusd ymm1, ymm2, ymm3  could not
;	    be assembled at all.
;	The families now live in vnni.inc and ifma.inc, which own both gates:
;	- VEX feature only: VEX; masks, broadcast, zmm and xmm16+ are refused;
;	- AVX-512 feature only: EVEX, as before;
;	- both: VEX unless an operand needs EVEX, or {evex} is given.
;	  Behaviour change: under "use everything", vpdpbusd xmm/ymm without
;	  masks is now the shorter VEX form, like vaddps and FMA.
;	Also new: AVX-VNNI-INT8 (7.1:EDX[4]) and AVX-VNNI-INT16 (7.1:EDX[10]),
;	VEX only.
;
; FAIL 1: 'AVX_VNNI or AVX512_VNNI required'
; FAIL 2: 'invalid operand'
; FAIL 3: 'invalid operand'
; FAIL 4: 'AVX_IFMA or AVX512_IFMA required'
; FAIL 5: 'AVX_VNNI_INT8 or higher required'
; FAIL 6: 'AVX_VNNI_INT16 or higher required'
; FAIL 7: 'invalid operand'
; FAIL 8: 'AVX2 or higher required'
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

	; AVX-VNNI only (was impossible): VEX
	expect <0C4h,0E2h,69h,50h,0CBh>, {i386, x64, AVX2, AVX_VNNI} vpdpbusd xmm1, xmm2, xmm3
	expect <0C4h,0E2h,6Dh,50h,0CBh>, {i386, x64, AVX2, AVX_VNNI} vpdpbusd ymm1, ymm2, ymm3
	expect <0C4h,0E2h,6Dh,53h,08h>, {i386, x64, AVX2, AVX_VNNI} vpdpwssds ymm1, ymm2, [rax]

	; AVX512_VNNI only: EVEX, unchanged
	expect <62h,0F2h,6Dh,08h,50h,0CBh>, {i386, x64, AVX512F, AVX512VL, AVX512_VNNI} vpdpbusd xmm1, xmm2, xmm3
	expect <62h,0F2h,6Dh,48h,50h,0CBh>, {i386, x64, AVX512F, AVX512_VNNI} vpdpbusd zmm1, zmm2, zmm3

	; both: VEX by default, EVEX when an operand needs it or on request
	expect <0C4h,0E2h,69h,50h,0CBh>, {everything} vpdpbusd xmm1, xmm2, xmm3
	expect <62h,0F2h,6Dh,08h,50h,0CBh>, {everything, evex} vpdpbusd xmm1, xmm2, xmm3
	expect <62h,0F2h,6Dh,58h,50h,08h>, {everything} vpdpbusd zmm1, zmm2, [rax]{1to16}
	expect <62h,0F2h,6Dh,29h,50h,0CBh>, {everything} vpdpbusd ymm1{k1}, ymm2, ymm3
	expect <62h,0B2h,6Dh,08h,50h,0CBh>, {everything} vpdpbusd xmm1, xmm2, xmm19

	; IFMA
	expect <0C4h,0E2h,0E9h,0B4h,0CBh>, {i386, x64, AVX2, AVX_IFMA} vpmadd52luq xmm1, xmm2, xmm3
	expect <0C4h,0E2h,0EDh,0B5h,0CBh>, {i386, x64, AVX2, AVX_IFMA} vpmadd52huq ymm1, ymm2, ymm3
	expect <62h,0F2h,0EDh,48h,0B4h,0CBh>, {i386, x64, AVX512F, AVX512_IFMA} vpmadd52luq zmm1, zmm2, zmm3
	expect <62h,0F2h,0EDh,58h,0B4h,08h>, {everything} vpmadd52luq zmm1, zmm2, [rax]{1to8}
	expect <0C4h,0E2h,0E9h,0B4h,0CBh>, {everything} vpmadd52luq xmm1, xmm2, xmm3

	; AVX-VNNI-INT8 / INT16
	expect <0C4h,0E2h,6Fh,50h,0CBh>, {i386, x64, AVX2, AVX_VNNI_INT8} vpdpbssd ymm1, ymm2, ymm3
	expect <0C4h,0E2h,6Bh,51h,0CBh>, {i386, x64, AVX2, AVX_VNNI_INT8} vpdpbssds xmm1, xmm2, xmm3
	expect <0C4h,0E2h,6Eh,50h,08h>, {i386, x64, AVX2, AVX_VNNI_INT8} vpdpbsud ymm1, ymm2, [rax]
	expect <0C4h,0E2h,6Ch,51h,0CBh>, {i386, x64, AVX2, AVX_VNNI_INT8} vpdpbuuds ymm1, ymm2, ymm3
	expect <0C4h,42h,1Fh,50h,0DDh>, {i386, x64, AVX2, AVX_VNNI_INT8} vpdpbssd ymm11, ymm12, ymm13
	expect <0C4h,0E2h,6Eh,0D2h,0CBh>, {i386, x64, AVX2, AVX_VNNI_INT16} vpdpwsud ymm1, ymm2, ymm3
	expect <0C4h,0E2h,69h,0D3h,0CBh>, {i386, x64, AVX2, AVX_VNNI_INT16} vpdpwusds xmm1, xmm2, xmm3
	expect <0C4h,0E2h,6Ch,0D2h,0CBh>, {i386, x64, AVX2, AVX_VNNI_INT16} vpdpwuud ymm1, ymm2, ymm3

else if FAIL = 1	; neither VNNI feature
	use i386, x64, AVX2
	vpdpbusd ymm1, ymm2, ymm3

else if FAIL = 2	; AVX_VNNI only: no zmm
	use i386, x64, AVX2, AVX_VNNI
	vpdpbusd zmm1, zmm2, zmm3

else if FAIL = 3	; AVX_VNNI only: no masks
	use i386, x64, AVX2, AVX_VNNI
	vpdpbusd ymm1{k1}, ymm2, ymm3

else if FAIL = 4	; neither IFMA feature
	use i386, x64, AVX2
	vpmadd52luq ymm1, ymm2, ymm3

else if FAIL = 5	; INT8 has its own bit
	use i386, x64, AVX2, AVX_VNNI, AVX_VNNI_INT16
	vpdpbssd ymm1, ymm2, ymm3

else if FAIL = 6	; INT16 has its own bit
	use i386, x64, AVX2, AVX_VNNI, AVX_VNNI_INT8
	vpdpwsud ymm1, ymm2, ymm3

else if FAIL = 7	; INT8 is VEX only, even with AVX-512
	use everything
	vpdpbssd zmm1, zmm2, zmm3

else if FAIL = 8	; 256-bit VEX integer forms need AVX2
	use i386, x64, AVX, AVX_VNNI
	vpdpbusd ymm1, ymm2, ymm3

else if FAIL = 9	; AVX_VNNI only: no xmm16+
	use i386, x64, AVX2, AVX_VNNI
	vpdpbusd xmm1, xmm2, xmm19

end if
