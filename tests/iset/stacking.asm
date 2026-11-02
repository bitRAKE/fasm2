; stacking.asm -- MMX/SSE forms of mnemonics extended by SSE2/SSE4.1 (fix G14)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 stacking.asm stacking.bin
;
; Stacking, and how it went wrong
;	A second definition of a mnemonic stacks on the first. Inside it, the
;	mnemonic's own name means the earlier definition, so a later layer can
;	handle its new forms and say "not for me, try the earlier one" for the
;	rest. In a macro that is an if/else; `exit` exists only in CALM.
;	sse2.inc and sse4.inc redefined movd, movq, pinsrw, pextrw and pmovmskb
;	to add xmm (and memory) forms, but began each with one blanket
;	"require SSE2+" or "require SSE4.1+" and never passed on, so the older
;	forms got the newer gate:
;	  use i386, MMX / movd mm1, eax          ->  'SSE2 or higher required'
;	  use i386, SSE / pextrw eax, mm1, 1     ->  'SSE4.1 or higher required'
;	  use i386, SSE2 / pextrw eax, xmm1, 1   ->  'SSE4.1 or higher required'
;	Now each layer gates only its own forms and passes the others down
;	(movd, pinsrw, pextrw in sse4.inc), or gates per form where it also
;	added forms the earlier layer lacks (movq mm<->r64, pextrw/pmovmskb r64).
;
; FAIL 1: 'SSE2 or higher required'
; FAIL 2: 'SSE4.1 or higher required'
; FAIL 3: 'SSE or higher required'
; FAIL 4: 'SSE2 or higher required'

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

if ~ defined FAIL

	use32
	; MMX
	expect <0Fh,6Eh,0C8h>, {i386, MMX} movd mm1, eax
	expect <0Fh,7Eh,08h>, {i386, MMX} movd [eax], mm1
	expect <0Fh,6Fh,0CAh>, {i386, MMX} movq mm1, mm2
	expect <0Fh,6Fh,08h>, {i386, MMX} movq mm1, [eax]
	expect <0Fh,7Fh,08h>, {i386, MMX} movq [eax], mm1
	; SSE (integer MMX extensions)
	expect <0Fh,0C5h,0C1h,01h>, {i386, SSE} pextrw eax, mm1, 1
	expect <0Fh,0C4h,0C8h,01h>, {i386, SSE} pinsrw mm1, eax, 1
	expect <0Fh,0D7h,0C1h>, {i386, SSE} pmovmskb eax, mm1
	; SSE2
	expect <66h,0Fh,0C5h,0C1h,01h>, {i386, SSE2} pextrw eax, xmm1, 1
	expect <66h,0Fh,0C4h,0C8h,01h>, {i386, SSE2} pinsrw xmm1, eax, 1
	expect <66h,0Fh,6Eh,0C8h>, {i386, SSE2} movd xmm1, eax
	expect <66h,0Fh,7Eh,08h>, {i386, SSE2} movd [eax], xmm1
	expect <0F3h,0Fh,7Eh,0CAh>, {i386, SSE2} movq xmm1, xmm2
	expect <66h,0Fh,0D6h,08h>, {i386, SSE2} movq [eax], xmm1
	expect <66h,0Fh,0D7h,0C1h>, {i386, SSE2} pmovmskb eax, xmm1
	; SSE4.1
	expect <66h,0Fh,3Ah,15h,08h,01h>, {i386, SSE4.1} pextrw [eax], xmm1, 1

	use64
	expect <48h,0Fh,6Eh,0C8h>, {x64, MMX} movq mm1, rax
	expect <66h,48h,0Fh,6Eh,0C8h>, {everything} movq xmm1, rax
	expect <66h,0Fh,0C5h,0C1h,01h>, {everything} pextrw rax, xmm1, 1

else if FAIL = 1	; xmm forms still need SSE2
	use32
	use i386, MMX
	movd xmm1, eax

else if FAIL = 2	; the memory form of pextrw is SSE4.1
	use32
	use i386, SSE2
	pextrw [eax], xmm1, 1

else if FAIL = 3	; pextrw on mm is SSE, not plain MMX
	use32
	use i386, MMX
	pextrw eax, mm1, 1

else if FAIL = 4
	use32
	use i386, MMX
	movq xmm1, xmm2

end if
