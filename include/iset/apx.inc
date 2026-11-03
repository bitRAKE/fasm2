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
;
; EVEX map 4 forms of legacy instructions (APX_F, plus the instruction's own
; feature, e.g. POPCNT or LZCNT):
;	NDD	add r, r/m, r/m|imm	a new data destination: one operand more
;		than the legacy form, written first; zero-uppered at 8/16 bits.
;		add or and sub xor adc sbb inc dec neg not shl sal shr sar rol
;		ror rcl rcr shld shrd, imul r, r, r/m
;	{nf}	add {nf} rax, rbx	no flags update, after the mnemonic only:
;		add or and sub xor inc dec neg shl sal shr sar rol ror shld
;		shrd imul mul div idiv popcnt lzcnt tzcnt
;	{evex}	{evex} add rax, rbx	EVEX encoding of the legacy form
; Each of these mnemonics is a layer over its legacy definition: forms with
; none of the above go straight to the earlier definition. The others set
; x86.apx.* and call it again; x86.store_instruction writes the EVEX prefix
; (map 4; 0F AF/A4/A5/AC/AD and F3 0F B8/BC/BD get their map 4 opcodes).
; The accumulator short forms (04h, 05h...) have no EVEX form; 80h-83h are
; used instead.

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

; APX.promote name, count, nf_ok, ndd_ok, sized, operands
;	count: operands of the legacy form (0: imul, whose 3-operand form is
;	NDD only when the last operand is not an immediate); sized: the
;	operation size may come from the second source (add rax, [rbx], rcx)
calminstruction APX.promote name*,count*,nf_ok*,ndd_ok*,sized*,operands&
	local	nf, rest, ndd, first, second, commas, line, size
	call	x86.require.bits64
	call	x86.require.APX_F
	compute nf, 0
	match	{=nf?} rest, operands
	jno	nf_ready
	check	nf_ok
	jno	nf_unsupported
	compute nf, 1
	arrange operands, rest
    nf_ready:
	match	{first} rest, operands
	jyes	unknown_option
	compute x86.apx.nd, 0
	compute x86.apx.v, 0
	compute commas, 0
	arrange rest, operands
    count_commas:
	match	first=,rest, rest
	jno	counted
	compute commas, commas + 1
	jump	count_commas
    counted:
	check	count
	jyes	ndd_by_count
	check	commas = 2		; imul ndd, r, r/m (but imul r, r/m, imm)
	jno	promote
	match	first=,second=,rest, operands
	call	x86.parse_operand@aux, rest
	check	@aux.type = 'imm'
	jyes	promote
	jump	ndd
    ndd_by_count:
	check	commas = count
	jno	promote
    ndd:
	check	ndd_ok
	jno	ndd_unsupported
	match	ndd=,rest, operands
	call	x86.parse_operand@aux, ndd
	check	@aux.type = 'reg'
	jno	invalid_ndd
	check	@aux.rm and x86.REX_FORBIDDEN
	jyes	disallowed
	compute x86.apx.v, @aux.rm and 11111b
	compute size, @aux.size
	match	first=,second, rest
	jyes	sources
	arrange first, rest
	arrange second,
    sources:
	call	x86.parse_operand@aux, first
	check	@aux.type = 'mem' & @aux.size = 0
	jyes	size_from_ndd
	check	@aux.size | sized = 0
	jyes	source_size
	match	second=,first, second
	call	x86.parse_operand@aux, second
	check	@aux.type = 'imm'
	jyes	ndd_ready
    source_size:
	check	@aux.size & @aux.size <> size
	jno	ndd_ready
	err	'operand sizes do not match'
	exit
    size_from_ndd:		; inc rax, [rbx]: the memory source takes the NDD's size
	check	size = 1
	jyes	byte_source
	check	size = 2
	jyes	word_source
	check	size = 4
	jyes	dword_source
	arrange first, =qword first
	jump	sized_source
    byte_source:
	arrange first, =byte first
	jump	sized_source
    word_source:
	arrange first, =word first
	jump	sized_source
    dword_source:
	arrange first, =dword first
    sized_source:
	match	, second
	jyes	single_source
	arrange rest, first=,second
	jump	ndd_ready
    single_source:
	arrange rest, first
    ndd_ready:
	compute x86.apx.nd, 1
	arrange operands, rest
    promote:
	compute x86.apx.nf, nf
	compute x86.apx.done, 0
	compute x86.apx.evex, 1
	arrange line, name operands
	assemble line
	compute x86.apx.evex, 0
	check	x86.apx.done
	jyes	done
	err	'no EVEX form for this instruction'
	exit
    nf_unsupported:
	err	'{nf} not supported by this instruction'
	exit
    unknown_option:
	err	'unknown instruction option'
	exit
    ndd_unsupported:
	err	'new data destination not supported by this instruction'
	exit
    invalid_ndd:
	err	'invalid new data destination'
	exit
    disallowed:
	err	'disallowed combination of registers'
    done:
end calminstruction

; the layers: legacy forms pass straight to the earlier definition
iterate <instr,count,nf_ok,ndd_ok,sized>, \
	add,2,1,1,1, or,2,1,1,1, and,2,1,1,1, sub,2,1,1,1, xor,2,1,1,1, \
	adc,2,0,1,1, sbb,2,0,1,1, \
	inc,1,1,1,0, dec,1,1,1,0, neg,1,1,1,0, not,1,0,1,0, \
	mul,1,1,0,0, div,1,1,0,0, idiv,1,1,0,0, imul,0,1,1,1, \
	shl,2,1,1,0, sal,2,1,1,0, shr,2,1,1,0, sar,2,1,1,0, rol,2,1,1,0, ror,2,1,1,0, \
	rcl,2,0,1,0, rcr,2,0,1,0, \
	shld,3,1,1,1, shrd,3,1,1,1, \
	popcnt,2,1,0,0, lzcnt,2,1,0,0, tzcnt,2,1,0,0

	calminstruction instr? operands&
		local	line, a, b
		check	x86.apx.evex		; called by APX.promote: the legacy form
		jyes	legacy
		match	{a} b, operands		; {nf} after the mnemonic
		jyes	promote
		check	x86.vexsize = 4		; {evex}
		jyes	promote
		check	ndd_ok
		jno	legacy
		match	a=,b, operands		; NDD: one operand more than the legacy form
		jno	legacy
		check	count = 1
		jyes	promote
		match	a=,b, b
		jno	legacy
		check	count = 2
		jyes	promote
		check	count = 3
		jno	imul_three
		match	a=,b, b
		jyes	promote
		jump	legacy
	    imul_three:			; imul r, r/m, imm is legacy; imul ndd, r, r/m is not
		call	x86.parse_operand@aux, b
		check	@aux.type = 'imm'
		jno	promote
	    legacy:
		arrange line, =instr operands
		assemble line
		exit
	    promote:
		xcall	APX.promote, <instr>,(count),(nf_ok),(ndd_ok),(sized),operands
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
