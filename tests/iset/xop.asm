; xop.asm -- AMD XOP vector instructions and the 8Fh encoder path (iset/xop.inc)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc,
;	operand roles from zydis-db); a mismatch stops assembly with 'encoding
;	mismatch'. The failure cases at the end are each assembled alone by
;	_run.ps1 (-i"FAIL=n") and must stop with the quoted message.
;	By hand:  fasm2 xop.asm xop.bin
;
; What is new
;	XOP (8000_0001h:ECX[11]) was missing, and the VEX encoder could only
;	emit C4h/C5h. Maps 8-0Ah now get the 8Fh lead byte. Failure modes this
;	file guards against:
;	- a C4h lead byte for an XOP map, or 8Fh for a VEX map (vpermil2ps is
;	  VEX 0F3A even though it belongs to XOP);
;	- the is4 register in imm8[7:4] and VEX.W choosing which source is in
;	  ModRM.rm: memory in the third or fourth operand position;
;	- the vprot/vpsha/vpshl W swap for a memory count, and the separate
;	  immediate opcode of vprot;
;	- the 64 vpcom<pred><type> aliases;
;	- 128-bit-only forms refusing ymm, and xmm16+ (EVEX-only) refused.
;
; FAIL 1: 'XOP or higher required'
; FAIL 2: 'invalid combination of operands'
; FAIL 3: 'invalid combination of operands'
; FAIL 4: 'invalid combination of operands'
; FAIL 5: 'invalid combination of operands'
; FAIL 6: 'invalid operand'
; FAIL 7: 'AVX or higher required'

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

	expect <8Fh,0E8h,70h,0A2h,0C2h,30h>, {i386, x64, AVX, XOP} vpcmov xmm0, xmm1, xmm2, xmm3
	expect <8Fh,0E8h,70h,0A2h,00h,30h>, {i386, x64, AVX, XOP} vpcmov xmm0, xmm1, [rax], xmm3
	expect <8Fh,0E8h,0F0h,0A2h,00h,20h>, {i386, x64, AVX, XOP} vpcmov xmm0, xmm1, xmm2, [rax]
	expect <8Fh,0E8h,74h,0A2h,0C2h,30h>, {i386, x64, AVX, XOP} vpcmov ymm0, ymm1, ymm2, ymm3
	expect <8Fh,48h,30h,0A2h,0C2h,0B0h>, {i386, x64, AVX, XOP} vpcmov xmm8, xmm9, xmm10, xmm11
	expect <8Fh,0E8h,70h,0A3h,0C2h,30h>, {i386, x64, AVX, XOP} vpperm xmm0, xmm1, xmm2, xmm3
	expect <8Fh,0E8h,0F0h,0A3h,00h,20h>, {i386, x64, AVX, XOP} vpperm xmm0, xmm1, xmm2, [rax]
	expect <8Fh,0E8h,70h,9Eh,0C2h,30h>, {i386, x64, AVX, XOP} vpmacsdd xmm0, xmm1, xmm2, xmm3
	expect <8Fh,0E8h,70h,9Eh,00h,30h>, {i386, x64, AVX, XOP} vpmacsdd xmm0, xmm1, [rax], xmm3
	expect <8Fh,0E8h,70h,0B6h,0C2h,30h>, {i386, x64, AVX, XOP} vpmadcswd xmm0, xmm1, xmm2, xmm3
	expect <8Fh,0E8h,70h,0CCh,0C2h,02h>, {i386, x64, AVX, XOP} vpcomb xmm0, xmm1, xmm2, 2
	expect <8Fh,0E8h,70h,0EFh,00h,05h>, {i386, x64, AVX, XOP} vpcomuq xmm0, xmm1, [rax], 5
	expect <8Fh,0E8h,70h,0CCh,0C2h,00h>, {i386, x64, AVX, XOP} vpcomltb xmm0, xmm1, xmm2
	expect <8Fh,0E8h,70h,0CDh,0C2h,05h>, {i386, x64, AVX, XOP} vpcomneqw xmm0, xmm1, xmm2
	expect <8Fh,0E8h,70h,0EFh,0C2h,07h>, {i386, x64, AVX, XOP} vpcomtrueuq xmm0, xmm1, xmm2
	expect <8Fh,0E9h,68h,90h,0C1h>, {i386, x64, AVX, XOP} vprotb xmm0, xmm1, xmm2
	expect <8Fh,0E9h,0F0h,90h,00h>, {i386, x64, AVX, XOP} vprotb xmm0, xmm1, [rax]
	expect <8Fh,0E9h,68h,90h,00h>, {i386, x64, AVX, XOP} vprotb xmm0, [rax], xmm2
	expect <8Fh,0E8h,78h,0C2h,0C1h,05h>, {i386, x64, AVX, XOP} vprotd xmm0, xmm1, 5
	expect <8Fh,0E8h,78h,0C3h,00h,05h>, {i386, x64, AVX, XOP} vprotq xmm0, [rax], 5
	expect <8Fh,0E9h,68h,94h,0C1h>, {i386, x64, AVX, XOP} vpshlb xmm0, xmm1, xmm2
	expect <8Fh,0E9h,0F0h,9Bh,00h>, {i386, x64, AVX, XOP} vpshaq xmm0, xmm1, [rax]
	expect <8Fh,0E9h,78h,80h,0C1h>, {i386, x64, AVX, XOP} vfrczps xmm0, xmm1
	expect <8Fh,0E9h,7Ch,80h,0C1h>, {i386, x64, AVX, XOP} vfrczps ymm0, ymm1
	expect <8Fh,0E9h,7Ch,81h,00h>, {i386, x64, AVX, XOP} vfrczpd ymm0, [rax]
	expect <8Fh,0E9h,78h,82h,0C1h>, {i386, x64, AVX, XOP} vfrczss xmm0, xmm1
	expect <8Fh,0E9h,78h,83h,00h>, {i386, x64, AVX, XOP} vfrczsd xmm0, qword [rax]
	expect <8Fh,0E9h,78h,0C1h,0C1h>, {i386, x64, AVX, XOP} vphaddbw xmm0, xmm1
	expect <8Fh,0E9h,78h,0DBh,00h>, {i386, x64, AVX, XOP} vphaddudq xmm0, [rax]
	expect <8Fh,0E9h,78h,0E3h,0C1h>, {i386, x64, AVX, XOP} vphsubdq xmm0, xmm1
	expect <0C4h,0E3h,71h,48h,0C2h,31h>, {i386, x64, AVX, XOP} vpermil2ps xmm0, xmm1, xmm2, xmm3, 1
	expect <0C4h,0E3h,0F1h,48h,00h,22h>, {i386, x64, AVX, XOP} vpermil2ps xmm0, xmm1, xmm2, [rax], 2
	expect <0C4h,0E3h,75h,49h,00h,33h>, {i386, x64, AVX, XOP} vpermil2pd ymm0, ymm1, [rax], ymm3, 3

	; VEX map 0F3A instructions keep C4h
	expect <0C4h,0E3h,71h,4Ah,0C2h,30h>, {everything} vblendvps xmm0, xmm1, xmm2, xmm3

else if FAIL = 1	; FMA4 does not provide XOP
	use i386, x64, AVX, FMA4
	vpcmov xmm0, xmm1, xmm2, xmm3

else if FAIL = 2	; vpperm is 128-bit only
	use everything
	vpperm ymm0, ymm1, ymm2, ymm3

else if FAIL = 3	; vpmacs* take the accumulator in a register (is4)
	use everything
	vpmacsdd xmm0, xmm1, xmm2, [rax]

else if FAIL = 4	; only one memory operand
	use everything
	vpcmov xmm0, xmm1, [rax], [rbx]

else if FAIL = 5	; no immediate count for vpshl
	use everything
	vpshlb xmm0, xmm1, 3

else if FAIL = 6	; VEX layout: no xmm16+
	use everything
	vfrczps xmm0, xmm17

else if FAIL = 7	; XOP vector instructions need AVX
	use i386, x64, XOP
	vpcmov xmm0, xmm1, xmm2, xmm3

end if
