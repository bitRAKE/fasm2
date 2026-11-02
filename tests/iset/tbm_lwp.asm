; tbm_lwp.asm -- AMD TBM and LWP (iset/tbm.inc, iset/lwp.inc, bextr in iset/bmi.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc,
;	operand roles from zydis-db); a mismatch stops assembly with 'encoding
;	mismatch'. The failure cases at the end are each assembled alone by
;	_run.ps1 (-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 tbm_lwp.asm tbm_lwp.bin
;
; What is new
;	TBM (8000_0001h:ECX[21]) and LWP (8000_0001h:ECX[15]) were missing.
;	Both are XOP-encoded general-purpose instructions. Failure modes this
;	file guards against:
;	- nine TBM operations share two opcodes and differ in ModRM.reg;
;	- bextr has a BMI1 form (register control) and a TBM form (imm32
;	  control): one mnemonic, two gates; the BMI1 form keeps its bytes;
;	- neither needs AVX (VEX_GPR, fix G7); 64-bit operands need long mode.
;
; FAIL 1: 'TBM or higher required'
; FAIL 2: 'TBM required'
; FAIL 3: 'BMI1 required'
; FAIL 4: 'LWP or higher required'
; FAIL 5: 'bits64 or higher required'
; FAIL 6: 'invalid operand'

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
	expect <8Fh,0E9h,78h,01h,0C9h>, {i386, x64, TBM} blcfill eax, ecx
	expect <8Fh,0E9h,0F8h,01h,09h>, {i386, x64, TBM} blcfill rax, [rcx]
	expect <8Fh,0E9h,78h,01h,0D1h>, {i386, x64, TBM} blsfill eax, ecx
	expect <8Fh,0E9h,78h,01h,0D9h>, {i386, x64, TBM} blcs eax, ecx
	expect <8Fh,0E9h,78h,01h,0E1h>, {i386, x64, TBM} tzmsk eax, ecx
	expect <8Fh,0E9h,78h,01h,0E9h>, {i386, x64, TBM} blcic eax, ecx
	expect <8Fh,0E9h,78h,01h,0F1h>, {i386, x64, TBM} blsic eax, ecx
	expect <8Fh,0E9h,78h,01h,0F9h>, {i386, x64, TBM} t1mskc eax, ecx
	expect <8Fh,0E9h,78h,02h,0C9h>, {i386, x64, TBM} blcmsk eax, ecx
	expect <8Fh,0E9h,0B8h,02h,0F1h>, {i386, x64, TBM} blci r8, rcx
	expect <8Fh,0EAh,78h,10h,0C1h,34h,12h,00h,00h>, {i386, x64, TBM} bextr eax, ecx, 1234h
	expect <8Fh,0EAh,0F8h,10h,01h,34h,12h,00h,00h>, {i386, x64, TBM} bextr rax, [rcx], 1234h
	expect <0C4h,0E2h,68h,0F7h,0C1h>, {i386, x64, BMI1} bextr eax, ecx, edx

	expect <8Fh,0E9h,78h,12h,0C0h>, {i386, x64, LWP} llwpcb eax
	expect <8Fh,0E9h,0F8h,12h,0C0h>, {i386, x64, LWP} llwpcb rax
	expect <8Fh,0C9h,0F8h,12h,0C9h>, {i386, x64, LWP} slwpcb r9
	expect <8Fh,0EAh,78h,12h,0C1h,05h,00h,00h,00h>, {i386, x64, LWP} lwpins eax, ecx, 5
	expect <8Fh,0EAh,0F8h,12h,09h,05h,00h,00h,00h>, {i386, x64, LWP} lwpval rax, dword [rcx], 5

	use32
	expect <8Fh,0E9h,78h,01h,0C9h>, {i386, TBM} blcfill eax, ecx

else if FAIL = 1	; BMI1 does not provide TBM
	use64
	use i386, x64, BMI1
	blcfill eax, ecx

else if FAIL = 2	; the immediate bextr is TBM
	use64
	use i386, x64, BMI1
	bextr eax, ecx, 1234h

else if FAIL = 3	; the register bextr is BMI1
	use64
	use i386, x64, TBM
	bextr eax, ecx, edx

else if FAIL = 4
	use64
	use i386, x64, TBM
	llwpcb eax

else if FAIL = 5	; 64-bit operands need long mode
	use32
	blci rax, rcx

else if FAIL = 6	; llwpcb takes a register
	use64
	llwpcb [rax]

end if
