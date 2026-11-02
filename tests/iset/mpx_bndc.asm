; mpx_bndc.asm -- MPX BNDCL/BNDCU/BNDCN (fix G11; iset/mpx.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes; LLVM no longer
;	supports MPX, so the reference is a ZydisInfo decode of the bytes
;	(XED decodes these opcodes as NOP: MPX is retired). A mismatch stops
;	assembly with 'encoding mismatch'. The failure cases at the end are
;	each assembled alone by _run.ps1 (-i"FAIL=n") and must stop with the
;	quoted message. By hand:  fasm2 mpx_bndc.asm mpx.bin
;
; Failure mode resolved
;	G11 bndcl/bndcu/bndcn parsed their second operand with
;	    "x86.parse_operand @src,src" (a space before the context), which
;	    is not a defined instruction: all three failed with 'illegal
;	    instruction' in every form.
;
; FAIL 1: 'invalid operand size'
; FAIL 2: 'MPX required'

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
	expect <0F3h,0Fh,1Ah,0C0h>, {everything} bndcl bnd0, rax
	expect <0F2h,0Fh,1Ah,08h>, {everything} bndcu bnd1, [rax]
	use32
	expect <0F2h,0Fh,1Bh,0D0h>, {everything} bndcn bnd2, eax

else if FAIL = 1	; the register matches the mode's address size
	use64
	use everything
	bndcl bnd0, eax

else if FAIL = 2
	use64
	use i386, x64
	bndcl bnd0, rax

end if
