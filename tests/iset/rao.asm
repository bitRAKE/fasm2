; rao.asm -- remote atomic operations AADD/AAND/AOR/AXOR (iset/rao.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 rao.asm rao.bin
;
; What is new
;	RAO-INT (7.1:EAX[3]) was missing. Failure modes this file guards
;	against:
;	- the mandatory prefix (none/66/F2/F3) is what selects the operation;
;	  a 16-bit operand-size 66h would turn aadd into aand, so there is no
;	  16-bit-code form;
;	- memory destination only, 32- or 64-bit, 64-bit only in long mode;
;	- RAO_INT sits at bit 3 of the 7.1:EAX window, where XSS used to be.
;
; FAIL 1: 'RAO_INT or higher required'
; FAIL 2: 'invalid combination of operands'
; FAIL 3: 'invalid operand size'
; FAIL 4: 'operand sizes do not match'
; FAIL 5: 'instruction not available in 16-bit code'
; FAIL 6: 'RAO_INT or higher required'

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
	expect <0Fh,38h,0FCh,02h>, {i386, x64, RAO_INT} aadd [rdx], eax
	expect <48h,0Fh,38h,0FCh,02h>, {i386, x64, RAO_INT} aadd [rdx], rax
	expect <66h,0Fh,38h,0FCh,02h>, {i386, x64, RAO_INT} aand dword [rdx], eax
	expect <0F2h,4Dh,0Fh,38h,0FCh,01h>, {i386, x64, RAO_INT} aor [r9], r8
	expect <0F3h,0Fh,38h,0FCh,02h>, {i386, x64, RAO_INT} axor [rdx], eax
	use32
	expect <0Fh,38h,0FCh,02h>, {i386, RAO_INT} aadd [edx], eax
	expect <66h,0Fh,38h,0FCh,02h>, {i386, RAO_INT} aand [edx], eax

else if FAIL = 1
	use64
	use i386, x64
	aadd [rdx], eax

else if FAIL = 2	; memory destination only
	use64
	aadd eax, ecx

else if FAIL = 3	; 32- or 64-bit
	use64
	axor [rdx], ax

else if FAIL = 4
	use64
	aor dword [rdx], rax

else if FAIL = 5	; no 16-bit-code form
	use16
	aadd [di], eax

else if FAIL = 6	; XSS, once in the same window, does not enable it
	use64
	use i386, x64, XSAVE, XSS
	aadd [rdx], eax

end if
