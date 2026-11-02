; amd.asm -- AMD single-instruction extensions (iset/amd.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc,
;	and the APM for MCOMMIT; implicit operand lists from zydis-db); a
;	mismatch stops assembly with 'encoding mismatch'. The failure cases at
;	the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must stop
;	with the quoted message. By hand:  fasm2 amd.asm amd.bin
;
; What is new
;	MONITORX/MWAITX, CLZERO, INVLPGB/TLBSYNC, RDPRU and MCOMMIT were missing.
;	Each has its own CPUID bit (8000_0001h:ECX[29], 8000_0008h:EBX[0,3,4,8]).
;	Failure modes this file guards against:
;	- one feature enabling another's instruction;
;	- rAX operands that are addresses (monitorx, clzero, invlpgb) must use
;	  67h for a narrower register, never 66h/REX.W;
;	- mcommit (F3 0F 01 FA) and monitorx (0F 01 FA) differ only by prefix;
;	- written implicit operands must be the architectural registers.
;
; FAIL 1: 'MONITORX or higher required'
; FAIL 2: 'CLZERO or higher required'
; FAIL 3: 'INVLPGB or higher required'
; FAIL 4: 'RDPRU or higher required'
; FAIL 5: 'MCOMMIT or higher required'
; FAIL 6: 'rAX expected'
; FAIL 7: 'invalid implicit operand: rcx, expected ecx'
; FAIL 8: 'invalid implicit operand: ecx, expected edx'
; FAIL 9: 'invalid operand size'

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

	irp mode, use16, use32, use64
		mode
		expect <0Fh,01h,0FAh>, {i386, MONITORX} monitorx
		expect <0Fh,01h,0FBh>, {i386, MONITORX} mwaitx
		expect <0Fh,01h,0FCh>, {i386, CLZERO} clzero
		expect <0Fh,01h,0FEh>, {i386, INVLPGB} invlpgb
		expect <0Fh,01h,0FFh>, {i386, INVLPGB} tlbsync
		expect <0Fh,01h,0FDh>, {i386, RDPRU} rdpru
		expect <0F3h,0Fh,01h,0FAh>, {i386, MCOMMIT} mcommit
	end irp

	use64
	expect <0Fh,01h,0FAh>, {i386, x64, MONITORX} monitorx rax, ecx, edx
	expect <67h,0Fh,01h,0FAh>, {i386, x64, MONITORX} monitorx eax, ecx, edx
	expect <0Fh,01h,0FBh>, {i386, x64, MONITORX} mwaitx eax, ecx
	expect <0Fh,01h,0FCh>, {i386, x64, CLZERO} clzero rax
	expect <67h,0Fh,01h,0FCh>, {i386, x64, CLZERO} clzero eax
	expect <0Fh,01h,0FEh>, {i386, x64, INVLPGB} invlpgb rax, edx, ecx
	expect <67h,0Fh,01h,0FEh>, {i386, x64, INVLPGB} invlpgb eax, edx, ecx
	use32
	expect <0Fh,01h,0FAh>, {i386, MONITORX} monitorx eax, ecx, edx
	expect <67h,0Fh,01h,0FAh>, {i386, MONITORX} monitorx ax, ecx, edx
	use16
	expect <0Fh,01h,0FCh>, {i386, CLZERO} clzero ax
	expect <67h,0Fh,01h,0FCh>, {i386, CLZERO} clzero eax

	; neighbours keep their bytes
	use64
	expect <0Fh,01h,0C8h>, {everything} monitor
	expect <0Fh,01h,0D9h>, {everything} vmmcall

else if FAIL = 1	; each feature gates only its own instructions
	use i386, CLZERO
	monitorx

else if FAIL = 2
	use i386, MONITORX
	clzero

else if FAIL = 3
	use i386, RDPRU
	tlbsync

else if FAIL = 4
	use i386, INVLPGB
	rdpru

else if FAIL = 5	; MONITORX does not provide MCOMMIT
	use i386, MONITORX
	mcommit

else if FAIL = 6	; the address must be rAX
	use64
	clzero rbx

else if FAIL = 7	; MONITORX extensions are in ECX
	use64
	monitorx rax, rcx, edx

else if FAIL = 8	; INVLPGB: rAX, EDX, ECX in that order
	use64
	invlpgb rax, ecx, edx

else if FAIL = 9	; a 64-bit address only exists in long mode
	use32
	clzero rax

end if
