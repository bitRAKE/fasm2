; lzcnt.asm -- LZCNT is gated on its own CPUID bit, not on BMI1 (fix G2)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 lzcnt.asm lzcnt.bin
;
; Failure mode resolved
;	LZCNT is reported by CPUID 8000_0001h:ECX[5] (AMD "ABM"; Intel since
;	Haswell). TZCNT is BMI1 (7.0:EBX[3]). Both were gated on BMI1, so:
;	- an AMD K10 target (use LZCNT, no BMI1) was refused lzcnt, and
;	- a target with BMI1 but no LZCNT was allowed it.
;	Behaviour change: "use BMI1" alone no longer enables lzcnt; add LZCNT.
;
; FAIL 1: 'LZCNT required'
; FAIL 2: 'BMI1 required'
; FAIL 3: 'LZCNT required'

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
	expect <0F3h,0Fh,0BDh,0C1h>, {i386, LZCNT} lzcnt eax, ecx	; K10-style: LZCNT without BMI1
	expect <66h,0F3h,0Fh,0BDh,0C1h>, {i386, LZCNT} lzcnt ax, cx
	expect <0F3h,0Fh,0BCh,0C1h>, {i386, BMI1} tzcnt eax, ecx

	use64
	expect <0F3h,48h,0Fh,0BDh,0C1h>, {x64, LZCNT} lzcnt rax, rcx
	expect <0F3h,0Fh,0BDh,02h>, {x64, LZCNT} lzcnt eax, [rdx]
	expect <0F3h,0Fh,0BDh,0C1h>, {everything} lzcnt eax, ecx

else if FAIL = 1	; BMI1 alone no longer allows lzcnt
	use32
	use i386, BMI1
	lzcnt eax, ecx

else if FAIL = 2	; LZCNT does not imply tzcnt
	use32
	use i386, LZCNT
	tzcnt eax, ecx

else if FAIL = 3	; nothing enabled
	use32
	use i386
	lzcnt eax, ecx

end if
