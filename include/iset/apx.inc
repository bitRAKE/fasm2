; apx.inc -- Intel APX: new instructions (Advanced Performance Extensions)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	APX_F		7.1:EDX[21]		push2 pop2 push2p pop2p pushp popp jmpabs
;
; 64-bit code only. r16-r31 themselves (REX2 on legacy instructions) are in
; x86-2.inc; the plan for the rest of APX is include/iset/apx-plan.md.
; Encodings: Intel APX architecture specification 355828, 3.1.3.
;
; push2 v, b	EVEX.map4.ND=1 FF /6, Mod=3	push v, then push b
; pop2 v, b	EVEX.map4.ND=1 8F /0, Mod=3	pop v, then pop b
;	v in EVEX.vvvv (V4 in P2), b in ModRM.rm; 64-bit registers only,
;	neither may be rsp, and pop2's two must differ (#UD otherwise). The
;	stack must be 16-byte aligned (#GP otherwise; not checked here).
; push2p, pop2p	the same with EVEX.W = 1: the PPX (push-pop acceleration)
;	hint, for a push/pop pair that balance each other
; pushp r64	REX2.W=1 50+r	push with the PPX hint (REX2 even for rax-r15)
; popp r64	REX2.W=1 58+r
;	both take a list like push and pop: pushp rax rbx r16
; jmpabs imm64	REX2(M0=0,W=0) A1 imm64	jump to an absolute 64-bit address

iterate <instr,opcode,postbyte,ppx>, push2,0FFh,6,0, push2p,0FFh,6,1, pop2,8Fh,0,0, pop2p,8Fh,0,1

	calminstruction instr? first*,second*
		local	v, b
		call	x86.require.bits64
		call	x86.require.APX_F
		call	x86.parse_operand@dest, first
		call	x86.parse_operand@src, second
		check	@dest.type = 'reg' & @dest.size = 8 & @src.type = 'reg' & @src.size = 8
		jyes	registers
		err	'invalid combination of operands'
		exit
	    registers:
		compute v, @dest.rm and 11111b
		compute b, @src.rm and 11111b
		check	v = 4 | b = 4
		jno	no_rsp
		err	'rsp not allowed'
		exit
	    no_rsp:
		check	opcode = 8Fh & v = b
		jno	encode
		err	'pop2 registers must differ'
		exit
	    encode:		; 62 [R3 X3 B3 R4 B4 1 0 0] [W V3-V0 1 0 0] [0 0 0 ND=1 V4 0 0 0]
		emit	1, 62h
		emit	1, (0F4h xor ((b and 1000b) shl 2)) or ((b and 10000b) shr 1)
		emit	1, (ppx shl 7) or (((not v) and 1111b) shl 3) or 100b
		emit	1, 10h or ((((not v) and 10000b) shr 4) shl 3)
		emit	1, opcode
		emit	1, 0C0h + (postbyte shl 3) + (b and 111b)
	end calminstruction

end iterate

iterate <handler,opcode>, pushp_instruction,50h, popp_instruction,58h

	calminstruction x86.handler size*,operand*
		call	x86.require.bits64
		call	x86.require.APX_F
		call	x86.parse_operand@src, operand
		check	@src.type = 'reg' & @src.size = 8
		jyes	register
		err	'invalid operand'
		exit
	    register:
		asm	x86.rexsize =: 2	; the hint is REX2.W, so REX2 even for rax-r15
		xcall	x86.store_operand_prefix, (8),@src.rm
		asm	restore x86.rexsize
		emit	1, opcode + (@src.rm and 111b)
	end calminstruction

end iterate

; one register, or a list of them as for push and pop (x86.compact)
iterate <instr,handler>, pushp,pushp_instruction, popp,popp_instruction

	calminstruction instr? operand

		local	head, tail

		match	head tail, operand
		jno	plain
		transform head, x86.compact
		jno	plain
		match	{head}, head
		jno	plain
	    loop:
		xcall	x86.handler, (8),head
		match	head tail, tail
		jno	final
		transform head, x86.compact
		jno	error
		match	{head}, head
		jyes	loop
	    error:
		err	'only register operands allowed in compact syntax'
		exit
	    final:
		transform tail, x86.compact
		jno	error
		match	{operand}, tail
		jno	error
	    plain:
		xcall	x86.handler, (8),operand

	end calminstruction

end iterate

calminstruction jmpabs? target*
	call	x86.require.bits64
	call	x86.require.APX_F
	call	x86.parse_operand@dest, target
	check	@dest.type = 'imm' & (@dest.size = 0 | @dest.size = 8)
	jyes	immediate
	err	'invalid operand'
	exit
    immediate:
	emit	1, 0D5h		; REX2 with M0 = 0, W = 0; A1 is otherwise reserved under REX2
	emit	1, 0
	emit	1, 0A1h
	call	qword, @dest.imm
end calminstruction
