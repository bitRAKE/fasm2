; encoding_request.asm -- a one-line {vex2}/{vex3}/{evex}/{rex2} is a request
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc
;	and the SDM); a mismatch stops assembly with 'encoding mismatch'. The
;	failure cases at the end are each assembled alone by _run.ps1
;	(-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 encoding_request.asm encoding_request.bin
;
; What is resolved
;	An encoding selected for one line was a preference: {vex3} add,
;	{evex} vaesimc and {vex2} with a REX.B register silently assembled
;	another encoding. Like a size on a jump (jz byte target), it is now a
;	request, and an instruction that cannot have it is an error. The
;	check reads the first byte after legacy prefixes (C5, C4, 62, D5).
;	`use vex2/vex3/evex/rex2` set for a block stay preferences.
;
; FAIL 1: '{vex2} cannot encode this instruction'
; FAIL 2: '{vex2} cannot encode this instruction'
; FAIL 3: '{vex3} cannot encode this instruction'
; FAIL 4: '{vex3} cannot encode this instruction'
; FAIL 5: '{evex} cannot encode this instruction'
; FAIL 6: '{evex} cannot encode this instruction'
; FAIL 7: '{rex2} cannot encode this instruction'

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
use everything

if ~ defined FAIL

	; honoured requests, also after legacy prefixes
	expect <0C5h,0F1h,0FEh,0C2h>, {vex2} vpaddd xmm0, xmm1, xmm2
	expect <0C4h,0E1h,71h,0FEh,0C2h>, {vex3} vpaddd xmm0, xmm1, xmm2
	expect <64h,0C4h,0E1h,71h,0FEh,00h>, {vex3} vpaddd xmm0, xmm1, [fs:rax]
	expect <62h,0F1h,75h,08h,0FEh,0C2h>, {evex} vpaddd xmm0, xmm1, xmm2
	expect <0C4h,0C1h,71h,0FEh,00h>, {vex3} vpaddd xmm0, xmm1, [r8]
	expect <66h,0D5h,00h,01h,0D8h>, {rex2} add ax, bx
	expect <0F0h,0D5h,00h,01h,18h>, {rex2} lock add [rax], ebx
	; automatic selection and other settings need nothing
	expect <0C5h,0F1h,0FEh,0C2h>, {vexauto} vpaddd xmm0, xmm1, xmm2
	expect <01h,0D8h>, {rexauto} add eax, ebx

	; `use` keeps a preference: dropped where it cannot apply
	use vex3
	expect <0C4h,0E1h,71h,0FEh,0C2h>, vpaddd xmm0, xmm1, xmm2
	expect <01h,0D8h>, add eax, ebx
	use evex
	expect <0C4h,0E2h,79h,0DBh,0C1h>, vaesimc xmm0, xmm1
	use vexauto

	; 32-bit code has VEX and EVEX too
	use32
	expect <0C4h,0E1h,71h,0FEh,0C2h>, {vex3} vpaddd xmm0, xmm1, xmm2
	expect <62h,0F1h,75h,08h,0FEh,0C2h>, {evex} vpaddd xmm0, xmm1, xmm2
	use64

else if FAIL = 1	; REX.B needs VEX3
	{vex2} vpaddd xmm0, xmm1, [r8]

else if FAIL = 2	; map 2 needs VEX3
	{vex2} vpshufb xmm0, xmm1, xmm2

else if FAIL = 3	; no VEX form
	{vex3} add eax, ebx

else if FAIL = 4
	{vex3} paddd xmm0, xmm1

else if FAIL = 5	; vaesimc has no EVEX form
	{evex} vaesimc xmm0, xmm1

else if FAIL = 6
	{evex} paddd xmm0, xmm1

else if FAIL = 7	; no REX2 for VEX instructions
	{rex2} vaesimc xmm0, xmm1

end if
