; sev.asm -- AMD SEV-SNP instructions (iset/sev.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc;
;	implicit operand lists from zydis-db); a mismatch stops assembly with
;	'encoding mismatch'. The failure cases at the end are each assembled
;	alone by _run.ps1 (-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 sev.asm sev.bin
;
; What is new
;	PVALIDATE, RMPADJUST, RMPUPDATE, PSMASH (SEV_SNP, 8000_001Fh:EAX[4]) and
;	RMPQUERY (RMPQUERY, 8000_001Fh:EAX[6]) were missing. Failure modes this
;	file guards against:
;	- the F2/F3 prefix is what separates these from rdpru, invlpgb and
;	  tlbsync (0F 01 FD..FF); a lost prefix would silently encode those;
;	- PVALIDATE's rAX is an address, so eAX in 64-bit code means 67h, while
;	  RMPADJUST/RMPUPDATE/PSMASH take RAX as a value: eAX is refused;
;	- all but PVALIDATE are 64-bit only;
;	- written implicit operands must be the architectural registers.
;
; FAIL 1: 'SEV_SNP or higher required'
; FAIL 2: 'RMPQUERY or higher required'
; FAIL 3: 'SEV_SNP or higher required'
; FAIL 4: 'bits64 or higher required'
; FAIL 5: 'bits64 or higher required'
; FAIL 6: 'invalid implicit operand: eax, expected rax'
; FAIL 7: 'invalid implicit operand: rcx, expected ecx'
; FAIL 8: 'rAX expected'
; FAIL 9: 'extra characters on line'
; FAIL 10: 'invalid operand size'

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
	expect <0F2h,0Fh,01h,0FFh>, {i386, x64, SEV_SNP} pvalidate
	expect <0F2h,0Fh,01h,0FFh>, {i386, x64, SEV_SNP} pvalidate rax, ecx, edx
	expect <67h,0F2h,0Fh,01h,0FFh>, {i386, x64, SEV_SNP} pvalidate eax, ecx, edx
	expect <0F3h,0Fh,01h,0FEh>, {i386, x64, SEV_SNP} rmpadjust
	expect <0F3h,0Fh,01h,0FEh>, {i386, x64, SEV_SNP} rmpadjust rax, rcx, rdx
	expect <0F2h,0Fh,01h,0FEh>, {i386, x64, SEV_SNP} rmpupdate rax, rcx
	expect <0F3h,0Fh,01h,0FFh>, {i386, x64, SEV_SNP} psmash rax
	expect <0F3h,0Fh,01h,0FFh>, {i386, x64, SEV_SNP} psmash
	expect <0F3h,0Fh,01h,0FDh>, {i386, x64, RMPQUERY} rmpquery
	expect <0F3h,0Fh,01h,0FDh>, {everything} rmpquery

	use32
	expect <0F2h,0Fh,01h,0FFh>, {i386, SEV_SNP} pvalidate eax, ecx, edx
	expect <67h,0F2h,0Fh,01h,0FFh>, {i386, SEV_SNP} pvalidate ax, ecx, edx

	; neighbours without a prefix keep their bytes
	use64
	expect <0Fh,01h,0FDh>, {everything} rdpru
	expect <0Fh,01h,0FEh>, {everything} invlpgb
	expect <0Fh,01h,0FFh>, {everything} tlbsync

else if FAIL = 1	; SEV_SNP is required
	use64
	use i386, x64, SEV_ES
	pvalidate

else if FAIL = 2	; RMPQUERY has its own bit
	use64
	use i386, x64, SEV_SNP
	rmpquery

else if FAIL = 3	; RMPQUERY does not imply SEV_SNP
	use64
	use i386, x64, RMPQUERY
	psmash

else if FAIL = 4	; RMPADJUST is 64-bit only
	use32
	rmpadjust

else if FAIL = 5	; RMPQUERY is 64-bit only
	use32
	rmpquery

else if FAIL = 6	; RMPADJUST takes RAX as a value, not an address
	use64
	rmpadjust eax, rcx, rdx

else if FAIL = 7	; PVALIDATE's page size is in ECX
	use64
	pvalidate rax, rcx, edx

else if FAIL = 8	; PVALIDATE's address is rAX
	use64
	pvalidate rbx, ecx, edx

else if FAIL = 9	; PSMASH has one operand
	use64
	psmash rax, rcx

else if FAIL = 10	; a 16-bit address does not exist in long mode
	use64
	pvalidate ax, ecx, edx

end if
