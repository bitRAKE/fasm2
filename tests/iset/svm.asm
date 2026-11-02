; svm.asm -- AMD-V instructions: implicit operands, address size, gating
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc,
;	and the AMD APM vol.3 for VMGEXIT, which no local tool knows); a
;	mismatch stops assembly with 'encoding mismatch'. The failure cases at
;	the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 svm.asm svm.bin
;
;	This file does not include iset/svm.inc: x86-2.inc does, like every
;	other iset file, and "use SVM" is the only switch.
;
; Failure modes resolved
;	S1  bare INVLPGA in 16/32-bit code failed ('invalid operand size'):
;	    the default operand was rax in every mode.
;	S2  bare VMRUN / VMLOAD / VMSAVE failed ('missing argument'); the APM
;	    writes rAX as implicit.
;	S3  SKINIT was gated on SVM; it has its own bit, 8000_0001h:ECX[12].
;	S4  STGI was gated on SVM; it is present with SVM or SKINIT.
;	    VMGEXIT (F3 0F 01 D9) added, gated on SEV_ES, 8000_001Fh:EAX[3].
;	    svm.inc was the one iset file x86-2.inc did not include.
;	The rAX operand selects the address size (67h), never 66h/REX.W.
;
; FAIL 1: 'SVM or higher required'
; FAIL 2: 'SKINIT or higher required'
; FAIL 3: 'SVM or SKINIT required'
; FAIL 4: 'SEV_ES or higher required'
; FAIL 5: 'SVM or higher required'
; FAIL 6: 'invalid operand size'
; FAIL 7: 'invalid operand size'
; FAIL 8: 'only rAX register supported'
; FAIL 9: 'only rAX register supported'
; FAIL 10: 'register EAX expected'
; FAIL 11: 'invlpga operands must be rAX and ECX'

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

	; S1, S2: implicit rAX in every mode, no prefix
	irp mode, use16, use32, use64
		mode
		expect <0Fh,01h,0D8h>, {SVM} vmrun
		expect <0Fh,01h,0DAh>, {SVM} vmload
		expect <0Fh,01h,0DBh>, {SVM} vmsave
		expect <0Fh,01h,0DFh>, {SVM} invlpga
		expect <0F3h,0Fh,01h,0D9h>, {SEV_ES} vmgexit
	end irp

	; explicit rAX: its width is the address size
	use16
	expect <0Fh,01h,0D8h>, {SVM} vmrun ax
	expect <67h,0Fh,01h,0D8h>, {SVM} vmrun eax
	use32
	expect <0Fh,01h,0D8h>, {SVM} vmrun eax
	expect <67h,0Fh,01h,0D8h>, {SVM} vmrun ax
	expect <67h,0Fh,01h,0DFh>, {SVM} invlpga ax, ecx
	expect <0Fh,01h,0DFh>, {SVM} invlpga eax
	use64
	expect <0Fh,01h,0D8h>, {SVM} vmrun rax
	expect <67h,0Fh,01h,0D8h>, {SVM} vmrun eax
	expect <67h,0Fh,01h,0DBh>, {SVM} vmsave eax
	expect <0Fh,01h,0DFh>, {SVM} invlpga rax, ecx
	expect <67h,0Fh,01h,0DFh>, {SVM} invlpga eax, ecx

	; fixed encodings
	expect <0Fh,01h,0D9h>, {SVM} vmmcall
	expect <0Fh,01h,0DDh>, {SVM} clgi
	expect <0Fh,01h,0DEh>, {i386, SKINIT} skinit		; S3: SKINIT alone
	expect <0Fh,01h,0DEh>, {i386, SKINIT} skinit eax
	expect <0Fh,01h,0DCh>, {i386, SKINIT} stgi		; S4: SKINIT alone
	expect <0Fh,01h,0DCh>, {i386, SVM} stgi			; S4: SVM alone
	expect <0F3h,0Fh,01h,0D9h>, {i386, SEV_ES} vmgexit	; SEV_ES alone
	expect <0Fh,01h,0D8h>, {everything} vmrun

else if FAIL = 1	; SVM instructions need SVM
	use64
	use i386
	vmrun

else if FAIL = 2	; S3: SVM alone does not provide SKINIT
	use32
	use i386, SVM
	skinit

else if FAIL = 3	; S4: neither SVM nor SKINIT
	use32
	use i386
	stgi

else if FAIL = 4	; VMGEXIT is SEV_ES, not SVM
	use64
	use i386, SVM
	vmgexit

else if FAIL = 5	; SEV_ES does not provide VMMCALL
	use64
	use i386, SEV_ES
	vmmcall

else if FAIL = 6	; 16-bit address size does not exist in long mode
	use64
	vmrun ax

else if FAIL = 7	; 64-bit address size only exists in long mode
	use32
	vmrun rax

else if FAIL = 8	; only rAX
	use64
	vmrun rbx

else if FAIL = 9	; a register, not memory
	use64
	vmrun [rax]

else if FAIL = 10	; SKINIT takes EAX only
	use64
	skinit rax

else if FAIL = 11	; the ASID is in ECX
	use64
	invlpga rax, rcx

end if
