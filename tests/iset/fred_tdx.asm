; fred_tdx.asm -- ERETS/ERETU, LKGS and TDX instructions (iset/fred.inc, iset/tdx.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc;
;	modes from XED); a mismatch stops assembly with 'encoding mismatch'.
;	The failure cases at the end are each assembled alone by _run.ps1
;	(-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 fred_tdx.asm fred_tdx.bin
;
; What is new
;	FRED (7.1:EAX[17]: erets, eretu), LKGS (7.1:EAX[18]) and TDX (no CPUID
;	bit; synthetic setting: tdcall, seamcall, seamret, seamops) were
;	missing. All are 64-bit only. Failure modes this file guards against:
;	- erets/eretu differ from clac (0F 01 CA) only by prefix;
;	- lkgs takes a 16-bit register or memory operand; 0F 00 /6 without F2
;	  is the IA-64 jmpe, which stays unimplemented;
;	- TDX has to be enabled explicitly (or by "use everything").
;
; FAIL 1: 'FRED or higher required'
; FAIL 2: 'LKGS or higher required'
; FAIL 3: 'bits64 or higher required'
; FAIL 4: 'invalid operand size'
; FAIL 5: 'TDX or higher required'
; FAIL 6: 'bits64 or higher required'
; FAIL 7: 'illegal instruction'

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

	expect <0F2h,0Fh,01h,0CAh>, {i386, x64, FRED} erets
	expect <0F3h,0Fh,01h,0CAh>, {i386, x64, FRED} eretu
	expect <0F2h,0Fh,00h,0F0h>, {i386, x64, LKGS} lkgs ax
	expect <0F2h,0Fh,00h,30h>, {i386, x64, LKGS} lkgs [rax]
	expect <0F2h,0Fh,00h,30h>, {i386, x64, LKGS} lkgs word [rax]
	expect <66h,0Fh,01h,0CCh>, {i386, x64, TDX} tdcall
	expect <66h,0Fh,01h,0CDh>, {i386, x64, TDX} seamret
	expect <66h,0Fh,01h,0CEh>, {i386, x64, TDX} seamops
	expect <66h,0Fh,01h,0CFh>, {i386, x64, TDX} seamcall
	expect <66h,0Fh,01h,0CCh>, {everything} tdcall

	; neighbours keep their bytes
	expect <0Fh,01h,0CAh>, {everything} clac
	expect <0Fh,00h,0D0h>, {everything} lldt ax

else if FAIL = 1	; LKGS does not provide FRED
	use i386, x64, LKGS
	erets

else if FAIL = 2	; FRED does not provide LKGS
	use i386, x64, FRED
	lkgs ax

else if FAIL = 3	; 64-bit only
	use32
	erets

else if FAIL = 4	; 16-bit selector
	lkgs eax

else if FAIL = 5	; TDX must be enabled
	use i386, x64, VMX
	seamcall

else if FAIL = 6
	use32
	tdcall

else if FAIL = 7	; jmpe (IA-64) is out of scope
	jmpe ax

end if
