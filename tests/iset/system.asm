; system.asm -- PTWRITE, PCONFIG, ENCLV, VMFUNC (iset/system.inc, iset/vmx.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 system.asm system.bin
;
; What is new
;	PTWRITE (Intel PT 7.0:EBX[25] and 14h.0:EBX[4], both per XED), PCONFIG
;	(7.0:EDX[18]), ENCLV (12h.0:EAX[5]) and VMFUNC (VMX) were missing.
;	Failure modes this file guards against:
;	- PTWRITE needs both bits;
;	- PTWRITE takes 32- or 64-bit operands, and 66h is not allowed, so it
;	  has no 16-bit-code form and an unsized memory operand is ambiguous;
;	- ENCLV (0F 01 C0) sits next to VMCALL (0F 01 C1).
;
; FAIL 1: 'INTEL_PT or higher required'
; FAIL 2: 'PTWRITE or higher required'
; FAIL 3: 'invalid operand size'
; FAIL 4: 'operand size not specified'
; FAIL 5: 'instruction not available in 16-bit code'
; FAIL 6: 'PCONFIG or higher required'
; FAIL 7: 'ENCLV or higher required'
; FAIL 8: 'VMX or higher required'

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

	use64
	expect <0F3h,0Fh,0AEh,0E0h>, {i386, x64, INTEL_PT, PTWRITE} ptwrite eax
	expect <0F3h,48h,0Fh,0AEh,0E0h>, {i386, x64, INTEL_PT, PTWRITE} ptwrite rax
	expect <0F3h,0Fh,0AEh,20h>, {i386, x64, INTEL_PT, PTWRITE} ptwrite dword [rax]
	expect <0F3h,48h,0Fh,0AEh,20h>, {i386, x64, INTEL_PT, PTWRITE} ptwrite qword [rax]
	expect <0Fh,01h,0C5h>, {i386, x64, PCONFIG} pconfig
	expect <0Fh,01h,0C0h>, {i386, x64, ENCLV} enclv
	expect <0Fh,01h,0D4h>, {i386, x64, VMX} vmfunc
	use32
	expect <0F3h,0Fh,0AEh,0E0h>, {i386, INTEL_PT, PTWRITE} ptwrite eax
	expect <0Fh,01h,0C5h>, {i386, PCONFIG} pconfig
	use16
	expect <0Fh,01h,0C0h>, {i386, ENCLV} enclv

	; neighbour keeps its bytes
	use64
	expect <0Fh,01h,0C1h>, {everything} vmcall

else if FAIL = 1	; PTWRITE alone is not enough
	use64
	use i386, x64, PTWRITE
	ptwrite eax

else if FAIL = 2
	use64
	use i386, x64, INTEL_PT
	ptwrite eax

else if FAIL = 3	; 32- or 64-bit operand
	use64
	ptwrite ax

else if FAIL = 4	; memory needs a size
	use64
	ptwrite [rax]

else if FAIL = 5	; no 16-bit-code form
	use16
	ptwrite eax

else if FAIL = 6
	use64
	use i386, x64, SGX
	pconfig

else if FAIL = 7	; SGX alone does not provide ENCLV
	use64
	use i386, x64, SGX
	enclv

else if FAIL = 8
	use64
	use i386, x64
	vmfunc

end if
