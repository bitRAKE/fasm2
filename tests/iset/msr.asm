; msr.asm -- RDMSRLIST/WRMSRLIST, WRMSRNS, URDMSR/UWRMSR (iset/msr.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc;
;	modes from XED); a mismatch stops assembly with 'encoding mismatch'.
;	The failure cases at the end are each assembled alone by _run.ps1
;	(-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 msr.asm msr.bin
;
; What is new
;	MSRLIST (7.1:EAX[27]), WRMSRNS (7.1:EAX[19]) and USER_MSR (7.1:EDX[15])
;	were missing. Failure modes this file guards against:
;	- urdmsr's register form (F2 0F 38 F8, ModRM.mod = 11) shares bytes
;	  with enqcmd (memory form): memory operands must be refused, and
;	  enqcmd must keep its bytes;
;	- the immediate forms are VEX map 7 but need neither AVX nor anything
;	  but USER_MSR; the MSR index is an unsigned 32-bit value;
;	- rdmsrlist and the user-MSR instructions are 64-bit only; wrmsrns is
;	  valid in every mode.
;
; FAIL 1: 'MSRLIST or higher required'
; FAIL 2: 'bits64 or higher required'
; FAIL 3: 'USER_MSR or higher required'
; FAIL 4: 'invalid combination of operands'
; FAIL 5: 'invalid combination of operands'
; FAIL 6: 'value out of range'
; FAIL 7: 'WRMSRNS or higher required'

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
	expect <0F2h,0Fh,01h,0C6h>, {i386, x64, MSRLIST} rdmsrlist
	expect <0F3h,0Fh,01h,0C6h>, {i386, x64, MSRLIST} wrmsrlist
	expect <0Fh,01h,0C6h>, {i386, x64, WRMSRNS} wrmsrns
	expect <0F2h,0Fh,38h,0F8h,0C1h>, {i386, x64, USER_MSR} urdmsr rax, rcx
	expect <0F3h,0Fh,38h,0F8h,0C8h>, {i386, x64, USER_MSR} uwrmsr rcx, rax
	expect <0F2h,45h,0Fh,38h,0F8h,0C1h>, {i386, x64, USER_MSR} urdmsr r8, r9
	expect <0C4h,0E7h,7Bh,0F8h,0C0h,78h,56h,34h,12h>, {i386, x64, USER_MSR} urdmsr rax, 12345678h
	expect <0C4h,0E7h,7Ah,0F8h,0C0h,78h,56h,34h,12h>, {i386, x64, USER_MSR} uwrmsr 12345678h, rax
	expect <0C4h,0C7h,7Bh,0F8h,0C1h,78h,56h,34h,12h>, {i386, x64, USER_MSR} urdmsr r9, 12345678h
	expect <0C4h,0E7h,7Bh,0F8h,0C0h,0FFh,0FFh,0FFh,0FFh>, {i386, x64, USER_MSR} urdmsr rax, 0FFFFFFFFh
	use32
	expect <0Fh,01h,0C6h>, {i386, WRMSRNS} wrmsrns
	use16
	expect <0Fh,01h,0C6h>, {i386, WRMSRNS} wrmsrns

	; neighbour keeps its bytes
	use64
	expect <0F2h,0Fh,38h,0F8h,01h>, {everything} enqcmd rax, [rcx]

else if FAIL = 1	; WRMSRNS does not provide the lists
	use64
	use i386, x64, WRMSRNS
	rdmsrlist

else if FAIL = 2
	use32
	wrmsrlist

else if FAIL = 3
	use64
	use i386, x64, MSRLIST
	urdmsr rax, rcx

else if FAIL = 4	; register operands only (memory would be enqcmd)
	use64
	urdmsr rax, [rcx]

else if FAIL = 5	; 64-bit registers
	use64
	uwrmsr ecx, eax

else if FAIL = 6	; unsigned 32-bit MSR index
	use64
	urdmsr rax, -1

else if FAIL = 7
	use32
	use i386
	wrmsrns

end if
