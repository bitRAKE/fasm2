; padlock.asm -- VIA/Zhaoxin PadLock instructions (iset/padlock.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc;
;	groups and modes from XED); a mismatch stops assembly with 'encoding
;	mismatch'. The failure cases at the end are each assembled alone by
;	_run.ps1 (-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 padlock.asm padlock.bin
;
; What is new
;	PadLock (CPUID C000_0001h:EDX, "present" bits 2, 6, 10, 12) was
;	missing. It is included and gated like every other set: "use
;	everything" enables it, and a target without the bits refuses it.
;	Failure modes this file guards against:
;	- xcrypt*, xsha* and montmul need their F3 prefix in the bytes, xstore
;	  only when "rep" is written;
;	- each unit (RNG, ACE, PHE, PMM) has its own bit;
;	- montmul has no 64-bit-addressing form.
;
; FAIL 1: 'PADLOCK_RNG or higher required'
; FAIL 2: 'PADLOCK_ACE or higher required'
; FAIL 3: 'PADLOCK_PHE or higher required'
; FAIL 4: 'PADLOCK_PMM or higher required'
; FAIL 5: 'instruction not available in 64-bit code'

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

	use32
	expect <0Fh,0A7h,0C0h>, {i386, PADLOCK_RNG} xstore
	expect <0Fh,0A7h,0C0h>, {i386, PADLOCK_RNG} xstorerng
	expect <0F3h,0Fh,0A7h,0C0h>, {i386, PADLOCK_RNG} rep xstore
	expect <0F3h,0Fh,0A7h,0C8h>, {i386, PADLOCK_ACE} xcryptecb
	expect <0F3h,0Fh,0A7h,0D0h>, {i386, PADLOCK_ACE} xcryptcbc
	expect <0F3h,0Fh,0A7h,0D8h>, {i386, PADLOCK_ACE} xcryptctr
	expect <0F3h,0Fh,0A7h,0E0h>, {i386, PADLOCK_ACE} xcryptcfb
	expect <0F3h,0Fh,0A7h,0E8h>, {i386, PADLOCK_ACE} xcryptofb
	expect <0F3h,0Fh,0A6h,0C8h>, {i386, PADLOCK_PHE} xsha1
	expect <0F3h,0Fh,0A6h,0D0h>, {i386, PADLOCK_PHE} xsha256
	expect <0F3h,0Fh,0A6h,0C0h>, {i386, PADLOCK_PMM} montmul
	use64
	expect <0F3h,0Fh,0A7h,0C8h>, {everything} xcryptecb

else if FAIL = 1	; each unit has its own bit
	use32
	use i386, PADLOCK_ACE
	xstore

else if FAIL = 2
	use32
	use i386, PADLOCK_RNG
	xcryptcbc

else if FAIL = 3
	use32
	use i386, PADLOCK_ACE
	xsha256

else if FAIL = 4
	use32
	use i386, PADLOCK_PHE
	montmul

else if FAIL = 5	; no 64-bit addressing form
	use64
	montmul

end if
