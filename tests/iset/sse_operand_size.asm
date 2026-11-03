; sse_operand_size.asm -- operand checks that compared a size with a type (G16b)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc
;	and the SDM); a mismatch stops assembly with 'encoding mismatch'. The
;	failure cases at the end are each assembled alone by _run.ps1
;	(-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 sse_operand_size.asm sse_operand_size.bin
;
; What is resolved
;	sse.inc (cvtss2si, cvttss2si, cvtps2pi, cvttps2pi) and sse4.inc
;	(pextrb) wrote @src.size = 'mem' / 'mmreg' where @src.type was
;	meant, so the operand-size check never applied:
;	  cvtss2si eax, mm0     assembled as cvtss2si eax, xmm0
;	  cvtps2pi mm0, mm1     assembled as cvtps2pi mm0, xmm1
;	  pextrb dword [rax], xmm0, 1   took a dword destination (m8 only)
;	Valid bytes for a different operand, so XED could not flag them.
;
; FAIL 1: 'invalid operand size'
; FAIL 2: 'invalid operand size'
; FAIL 3: 'invalid operand size'
; FAIL 4: 'invalid operand size'
; FAIL 5: 'invalid operand size'
; FAIL 6: 'invalid operand size'

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

	expect <0F3h,0Fh,2Dh,0C0h>, cvtss2si eax, xmm0
	expect <0F3h,48h,0Fh,2Ch,00h>, cvttss2si rax, dword [rax]
	expect <0F3h,0Fh,2Dh,00h>, cvtss2si eax, [rax]
	expect <0Fh,2Dh,0C1h>, cvtps2pi mm0, xmm1
	expect <0Fh,2Ch,00h>, cvttps2pi mm0, qword [rax]
	expect <66h,0Fh,3Ah,14h,00h,01h>, pextrb byte [rax], xmm0, 1
	expect <66h,0Fh,3Ah,14h,00h,01h>, pextrb [rax], xmm0, 1
	expect <66h,0Fh,3Ah,14h,0C0h,01h>, pextrb eax, xmm0, 1

else if FAIL = 1	; SSE scalar source: xmm or m32, not mm
	cvtss2si eax, mm0

else if FAIL = 2
	cvttss2si rax, mm1

else if FAIL = 3	; m32 only
	cvtss2si eax, qword [rax]

else if FAIL = 4	; xmm or m64, not mm
	cvtps2pi mm0, mm1

else if FAIL = 5	; m64 only
	cvttps2pi mm0, dword [rax]

else if FAIL = 6	; m8 only
	pextrb dword [rax], xmm0, 1

end if
