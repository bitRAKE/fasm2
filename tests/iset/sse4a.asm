; sse4a.asm -- AMD SSE4a: EXTRQ, INSERTQ, MOVNTSD, MOVNTSS (iset/sse4a.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 sse4a.asm sse4a.bin
;
; What is new
;	SSE4a (8000_0001h:ECX[6]) was missing. Failure modes this file guards
;	against:
;	- 66/F2 0F 78/79 without the prefix are vmread/vmwrite: the prefix must
;	  always be emitted, and vmread must keep its bytes;
;	- the immediate forms take two byte immediates, length then index,
;	  emitted in that order;
;	- movntsd/movntss store to memory only, with the scalar's size.
;
; FAIL 1: 'SSE4A or higher required'
; FAIL 2: 'invalid operand size'
; FAIL 3: 'invalid combination of operands'
; FAIL 4: 'invalid combination of operands'
; FAIL 5: 'invalid operand size'

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
	expect <66h,0Fh,78h,0C1h,03h,02h>, {i386, x64, SSE2, SSE4A} extrq xmm1, 3, 2
	expect <66h,0Fh,79h,0CAh>, {i386, x64, SSE2, SSE4A} extrq xmm1, xmm2
	expect <0F2h,0Fh,78h,0CAh,03h,02h>, {i386, x64, SSE2, SSE4A} insertq xmm1, xmm2, 3, 2
	expect <0F2h,0Fh,79h,0CAh>, {i386, x64, SSE2, SSE4A} insertq xmm1, xmm2
	expect <0F2h,0Fh,2Bh,08h>, {i386, x64, SSE2, SSE4A} movntsd [rax], xmm1
	expect <0F3h,0Fh,2Bh,08h>, {i386, x64, SSE2, SSE4A} movntss dword [rax], xmm1
	expect <66h,41h,0Fh,78h,0C1h,03h,02h>, {i386, x64, SSE2, SSE4A} extrq xmm9, 3, 2
	expect <0F2h,45h,0Fh,79h,0CAh>, {i386, x64, SSE2, SSE4A} insertq xmm9, xmm10
	use32
	expect <66h,0Fh,79h,0CAh>, {i386, SSE2, SSE4A} extrq xmm1, xmm2

	; neighbours keep their bytes
	use64
	expect <0Fh,78h,0C8h>, {everything} vmread rax, rcx
	expect <0Fh,2Bh,08h>, {everything} movntps [rax], xmm1

else if FAIL = 1
	use64
	use i386, x64, SSE4.2
	extrq xmm1, xmm2

else if FAIL = 2	; byte immediates
	use64
	extrq xmm1, 300, 2

else if FAIL = 3	; extrq has no memory form
	use64
	extrq xmm1, [rax]

else if FAIL = 4	; movntsd stores to memory
	use64
	movntsd xmm2, xmm1

else if FAIL = 5	; movntss stores a dword
	use64
	movntss qword [rax], xmm1

end if
