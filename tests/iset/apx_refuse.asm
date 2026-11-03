; apx_refuse.asm -- APX extended registers are refused (Phase 0, include/iset/apx-plan.md)
;
; Environment
;	Assembly-time only: nothing here is executed. fasmg assembles each
;	instruction inside VIRTUAL and compares the bytes (reference: llvm-mc);
;	a mismatch stops assembly with 'encoding mismatch'. The failure cases
;	at the end are each assembled alone by _run.ps1 (-i"FAIL=n") and must
;	stop with the quoted message. By hand:  fasm2 apx_refuse.asm apx_refuse.bin
;
; What is resolved
;	APX adds r16-r31 (with r16d/w/b/l). Their numbers need five bits, and
;	REX, VEX and the ModRM/SIB fields hold four. Defined as elements without
;	a refusal, every encoder masks the number and silently writes another
;	register:
;		add rax, r16	->  48 01 C0	(add rax, rax)
;		push r16	->  60		(pusha: 50h + 16)
;		bswap r16	->  48 0F C8	(bswap rax)
;		vpaddd zmm2, zmm1, [r16]  ->  base rax
;	The names now exist, and the three operand parsers that read GPR
;	numbers refuse 16-31: x86.parse_operand (registers and addresses, used
;	by every GPR instruction and through AVX_512.parse_operand by the
;	vector ones), AVX_512.parse_vsib_operand (gather/scatter base) and
;	MPX.parse_sib_operand (bndldx/bndstx, AMX's [base, index] form).
;	Instructions that emit 50h+rm, B8h+rm, 90h+rm or 0Fh,C8h+rm directly
;	get their register from x86.parse_operand, so the same check covers
;	them. The refusal holds in every mode and under "use everything"; the
;	APX phases replace it per encoder (REX2 first).
;
;	r8-r15 must keep their bytes: the cases below are the neighbours of
;	each refused form.
;
; FAIL 1: 'extended register not supported: r16'
; FAIL 2: 'extended register not supported: r16'
; FAIL 3: 'extended register not supported: r16'
; FAIL 4: 'extended register not supported: [r16]'
; FAIL 5: 'extended register not supported: [rax+r20*4]'
; FAIL 6: 'extended register not supported: r31b'
; FAIL 7: 'extended register not supported: r16w'
; FAIL 8: 'extended register not supported: r17l'
; FAIL 9: 'extended register not supported: r16d'
; FAIL 10: 'extended register not supported: [r16w]'
; FAIL 11: 'extended register not supported: r16'
; FAIL 12: 'extended register not supported: [r16]'
; FAIL 13: 'extended register not supported: r16'
; FAIL 14: 'extended register not supported: r31'
; FAIL 15: 'extended register not supported: r16'
; FAIL 16: 'extended register not supported: [r16+zmm1]'
; FAIL 17: 'extended register not supported: [r16,rax]'
; FAIL 18: 'extended register not supported: [rax+r16]'
; FAIL 19: 'extended register not supported: [r16,rax]'
; FAIL 20: 'extended register not supported: r16d'

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

	; the r8-r15 neighbours of every refused form keep their bytes
	expect <4Ch,01h,0F8h>, add rax, r15
	expect <47h,03h,7Ch,0AEh,08h>, add r15d, [r14+r13*4+8]
	expect <41h,57h>, push r15
	expect <41h,5Fh>, pop r15
	expect <49h,0Fh,0CFh>, bswap r15
	expect <41h,0B7h,01h>, mov r15b, 1
	expect <66h,45h,8Bh,3Fh>, mov r15w, [r15]
	expect <62h,0D1h,75h,48h,0FEh,17h>, vpaddd zmm2, zmm1, [r15]
	expect <62h,0D2h,7Dh,49h,90h,04h,0Fh>, vpgatherdd zmm0{k1}, [r15+zmm1]
	expect <0C4h,0C2h,7Bh,4Bh,04h,07h>, tileloadd tmm0, [r15+rax]

	; the names are registers of the right class and number
	irp <reg,class,number>, r16b,x86.r8,16, r16l,x86.r8,16, r31b,x86.r8,31, \
				r16w,x86.r16,16, r31w,x86.r16,31, \
				r16d,x86.r32,16, r31d,x86.r32,31, \
				r16,x86.r64,16, r31,x86.r64,31
		assert 1 metadataof reg relativeto class & 1 metadataof reg - class = number
	end irp

else if FAIL = 1	; was 48 01 C0: add rax, rax
	add rax, r16

else if FAIL = 2	; was 60: pusha
	push r16

else if FAIL = 3	; was 48 0F C8: bswap rax
	bswap r16

else if FAIL = 4	; was base rax
	vpaddd zmm2, zmm1, [r16]

else if FAIL = 5	; index
	mov eax, [rax+r20*4]

else if FAIL = 6	; 8-bit
	mov r31b, 1

else if FAIL = 7	; 16-bit
	mov r16w, ax

else if FAIL = 8	; the l spelling, as r8l-r15l
	mov r17l, al

else if FAIL = 9	; 32-bit
	lea r16d, [rip]

else if FAIL = 10	; a 16-bit address register
	mov ax, [r16w]

else if FAIL = 11	; B8h+rm
	mov r16, 1

else if FAIL = 12	; jump operands go through x86.parse_jump_operand
	call qword [r16]

else if FAIL = 13	; 90h+rm
	xchg rax, r16

else if FAIL = 14	; control register moves
	mov cr0, r31

else if FAIL = 15	; pop: 58h+rm
	pop r16

else if FAIL = 16	; VSIB base (AVX_512.parse_vsib_operand)
	vpgatherdd zmm0{k1}, [r16+zmm1]

else if FAIL = 17	; MPX [base, index] (MPX.parse_sib_operand)
	bndldx bnd0, [r16,rax]

else if FAIL = 18	; AMX through AVX_512.parse_operand
	tileloadd tmm0, [rax+r16]

else if FAIL = 19	; AMX [base, index] through MPX.parse_sib_operand
	tileloadd tmm0, [r16,rax]

else if FAIL = 20	; also outside long mode
	use32
	mov eax, r16d

end if
