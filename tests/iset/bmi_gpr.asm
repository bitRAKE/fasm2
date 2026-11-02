; bmi_gpr.asm -- VEX-encoded general-purpose instructions do not need AVX (fix G7)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 bmi_gpr.asm bmi_gpr.bin
;
; Failure mode resolved
;	BMI1/BMI2 instructions are VEX-encoded but operate on general-purpose
;	registers; CPUID does not tie them to AVX, and some parts (e.g. Pentium
;	and Celeron models of the Skylake era) have BMI1/BMI2 without AVX. The
;	shared VEX encoder required AVX for every VEX instruction, so
;	use x64, BMI1 / andn eax, ecx, edx  failed with 'AVX or higher
;	required'. GPR instructions now pass VEX_GPR, which skips that check;
;	vector VEX instructions still require AVX.
;
; FAIL 1: 'BMI1 required'
; FAIL 2: 'BMI2 required'
; FAIL 3: 'AVX or higher required'

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

	; BMI1 without AVX (was refused)
	expect <0C4h,0E2h,70h,0F2h,0C2h>, {i386, x64, BMI1} andn eax, ecx, edx
	expect <0C4h,0E2h,0F0h,0F2h,0C2h>, {i386, x64, BMI1} andn rax, rcx, rdx
	expect <0C4h,0E2h,68h,0F7h,0C1h>, {i386, x64, BMI1} bextr eax, ecx, edx
	expect <0C4h,0E2h,78h,0F3h,0C9h>, {i386, x64, BMI1} blsr eax, ecx
	expect <0C4h,0E2h,0F8h,0F3h,1Ah>, {i386, x64, BMI1} blsi rax, [rdx]

	; BMI2 without AVX (was refused)
	expect <0C4h,0E2h,68h,0F5h,0C1h>, {i386, x64, BMI2} bzhi eax, ecx, edx
	expect <0C4h,0E2h,0F3h,0F5h,0C2h>, {i386, x64, BMI2} pdep rax, rcx, rdx
	expect <0C4h,0E2h,72h,0F5h,02h>, {i386, x64, BMI2} pext eax, ecx, [rdx]
	expect <0C4h,0E3h,7Bh,0F0h,0C1h,05h>, {i386, x64, BMI2} rorx eax, ecx, 5
	expect <0C4h,0E2h,6Ah,0F7h,0C1h>, {i386, x64, BMI2} sarx eax, ecx, edx
	expect <0C4h,0E2h,0E9h,0F7h,0C1h>, {i386, x64, BMI2} shlx rax, rcx, rdx
	expect <0C4h,0E2h,73h,0F6h,0C2h>, {i386, x64, BMI2} mulx eax, ecx, edx

	; unchanged with everything enabled
	expect <0C4h,0E2h,70h,0F2h,0C2h>, {everything} andn eax, ecx, edx

else if FAIL = 1	; the feature itself is still required
	use i386, x64, AVX2
	andn eax, ecx, edx

else if FAIL = 2
	use i386, x64, BMI1
	pdep rax, rcx, rdx

else if FAIL = 3	; vector VEX instructions still need AVX
	use i386, x64, BMI1, BMI2
	vpaddd xmm1, xmm2, xmm3

end if
