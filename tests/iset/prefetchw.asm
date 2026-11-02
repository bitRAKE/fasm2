; prefetchw.asm -- PREFETCHW is available with 3DNow or with PRFCHW (fix G4)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 prefetchw.asm prefetchw.bin
;
; Failure mode resolved
;	PREFETCHW (0F 0D /1) is present when CPUID reports either 3DNow
;	(8000_0001h:EDX[31]) or PRFCHW (8000_0001h:ECX[8], Intel since
;	Broadwell). It was gated on 3DNow only, so every Intel target was
;	refused it:  use x64, AVX2 / prefetchw [rax]  ->  '3DNow required'.
;	PREFETCH (0F 0D /0) stays 3DNow-only.
;
; FAIL 1: '3DNow or PRFCHW required'
; FAIL 2: '3DNow or higher required'
; FAIL 3: 'invalid operand'

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
	expect <0Fh,0Dh,08h>, {i386, PRFCHW} prefetchw [eax]	; Intel-style (was refused)
	expect <0Fh,0Dh,08h>, {i386, 3DNow} prefetchw [eax]	; AMD 3DNow
	expect <0Fh,0Dh,00h>, {i386, 3DNow} prefetch [eax]

	use64
	expect <0Fh,0Dh,08h>, {x64, AVX2, PRFCHW} prefetchw [rax]
	expect <0Fh,0Dh,08h>, {everything} prefetchw [rax]

else if FAIL = 1	; neither 3DNow nor PRFCHW
	use32
	use i386
	prefetchw [eax]

else if FAIL = 2	; PRFCHW does not enable the 3DNow-only PREFETCH
	use32
	use i386, PRFCHW
	prefetch [eax]

else if FAIL = 3	; still memory-only
	use32
	use i386, PRFCHW
	prefetchw eax

end if
