; cache.asm -- CLWB, PREFETCHWT1, PREFETCHIT0/1, WBNOINVD (iset/cache.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 cache.asm cache.bin
;
; What is new
;	CLWB (7.0:EBX[24]), PREFETCHWT1 (7.0:ECX[0]), PREFETCHIT0/1
;	(PREFETCHI, 7.1:EDX[14]) and WBNOINVD (8000_0008h:EBX[9]) were
;	missing. Failure modes this file guards against:
;	- clwb is memory-only: its register form is tpause (66 0F AE /6);
;	- prefetchit0/1 with anything but a RIP-relative address is a reserved
;	  NOP, and they exist in 64-bit code only;
;	- each feature has its own bit.
;
; FAIL 1: 'CLWB or higher required'
; FAIL 2: 'invalid operand'
; FAIL 3: 'RIP-relative address expected'
; FAIL 4: 'bits64 or higher required'
; FAIL 5: 'PREFETCHWT1 or higher required'
; FAIL 6: 'WBNOINVD or higher required'
; FAIL 7: 'invalid operand size'

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
	expect <66h,0Fh,0AEh,30h>, {i386, x64, CLWB} clwb [rax]
	expect <66h,41h,0Fh,0AEh,74h,89h,10h>, {i386, x64, CLWB} clwb byte [r9+rcx*4+10h]
	expect <0Fh,0Dh,10h>, {i386, x64, PREFETCHWT1} prefetchwt1 [rax]
	expect <0Fh,18h,3Dh,10h,00h,00h,00h>, {i386, x64, PREFETCHI} prefetchit0 [rip+10h]
	expect <0Fh,18h,35h,10h,00h,00h,00h>, {i386, x64, PREFETCHI} prefetchit1 [rip+10h]
	expect <0F3h,0Fh,09h>, {i386, x64, WBNOINVD} wbnoinvd
	use32
	expect <66h,0Fh,0AEh,30h>, {i386, CLWB} clwb [eax]
	expect <0F3h,0Fh,09h>, {i386, WBNOINVD} wbnoinvd

	; neighbours keep their bytes
	use64
	expect <66h,0Fh,0AEh,0F1h>, {everything} tpause ecx
	expect <0Fh,0Dh,08h>, {everything} prefetchw [rax]
	expect <0Fh,18h,00h>, {everything} prefetchnta [rax]

else if FAIL = 1	; CLFLUSHOPT does not provide CLWB
	use64
	use i386, x64, CLFLUSHOPT
	clwb [rax]

else if FAIL = 2	; clwb takes memory only
	use64
	clwb rax

else if FAIL = 3	; RIP-relative only
	use64
	prefetchit0 [rax]

else if FAIL = 4	; 64-bit code only
	use32
	prefetchit1 [eax]

else if FAIL = 5	; PRFCHW (prefetchw) does not provide PREFETCHWT1
	use64
	use i386, x64, PRFCHW
	prefetchwt1 [rax]

else if FAIL = 6
	use64
	use i386, x64, CLWB
	wbnoinvd

else if FAIL = 7	; byte-sized hint
	use64
	clwb qword [rax]

end if
