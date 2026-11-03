; apx.inc -- Intel APX: new instructions (Advanced Performance Extensions)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	APX_F		7.1:EDX[21]		push2 pop2 push2p pop2p pushp popp jmpabs
;						ccmp<scc> ctest<scc> cfcmov<cc>
;						(and the EVEX forms below, with the
;						instruction's own setting too)
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
;	{zu}	setb {zu} al		zero the upper bits: set<cc>, imul r, r/m, imm
;	{evex}	{evex} add rax, rbx	EVEX encoding of the legacy form (also
;		set<cc>; cmp/test as ccmpt/ctestt {dfv=})
;	cmov<cc> ndd, r, r/m (NDD only: ND=0 is cfcmov's load form)
; Each of these mnemonics is a layer over its legacy definition: forms with
; none of the above go straight to the earlier definition. The others set
; x86.apx.* and call it again; x86.store_instruction writes the EVEX prefix
; (map 4; 0F AF/A4/A5/AC/AD and F3 0F B8/BC/BD get their map 4 opcodes).
; The accumulator short forms (04h, 05h...) have no EVEX form; 80h-83h are
; used instead.

define APX APX		; makes APX.* reachable from inside CALM bodies, like x86, AVX, MPX

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

; APX.promote name, count, flags, operands
;	count: operands of the legacy form (0: imul, whose 3-operand form is
;	NDD only when the last operand is not an immediate)
;	flags: APX.NF, APX.NDD, APX.ZU allow those options; APX.SIZED: the
;	operation size may come from the second source (add rax, [rbx], rcx);
;	APX.NDD_ONLY: no EVEX form without NDD (cmovcc: ND=0 is CFCMOV);
;	APX.FORCE_NF: NF set by the caller (cfcmovcc)
APX.NF := 1
APX.NDD := 2
APX.ZU := 4
APX.SIZED := 8
APX.NDD_ONLY := 16
APX.FORCE_NF := 32

calminstruction APX.promote name*,count*,flags*,operands&
	local	nf, zu, immform, option, rest, ndd, first, second, commas, line, size
	call	x86.require.bits64
	call	x86.require.APX_F
	compute nf, 0
	compute zu, 0
	compute immform, 0
	check	flags and APX.FORCE_NF
	jno	options
	compute nf, 1
    options:			; {nf} {zu} after the mnemonic, in any order
	match	{option} rest, operands
	jno	options_done
	arrange operands, rest
	match	=nf?, option
	jyes	option_nf
	match	=zu?, option
	jyes	option_zu
	err	'unknown instruction option'
	exit
    option_nf:
	check	flags and APX.NF
	jno	nf_unsupported
	compute nf, 1
	jump	options
    option_zu:
	check	flags and APX.ZU
	jno	zu_unsupported
	compute zu, 1
	jump	options
    options_done:
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
	jno	ndd
	compute immform, 1
	jump	promote
    ndd_by_count:
	check	commas = count
	jno	promote
    ndd:
	check	flags and APX.NDD
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
	check	@aux.size | flags and APX.SIZED = 0
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
	check	zu
	jno	zu_ready
	check	x86.apx.nd | (flags and APX.NDD & immform = 0)
	jyes	zu_unsupported		; imul: only r, r/m, imm (69h/6Bh)
	compute x86.apx.nd, 1		; ZU is the ND bit
    zu_ready:
	check	flags and APX.NDD_ONLY & x86.apx.nd = 0
	jyes	no_evex_form
	compute x86.apx.nf, nf
	compute x86.apx.done, 0
	compute x86.apx.evex, 1
	arrange line, name operands
	assemble line
	compute x86.apx.evex, 0
	check	x86.apx.done
	jyes	done
    no_evex_form:
	err	'no EVEX form for this instruction'
	exit
    nf_unsupported:
	err	'{nf} not supported by this instruction'
	exit
    zu_unsupported:
	err	'{zu} not supported by this instruction'
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
iterate <instr,count,flags>, \
	add,2,APX.NF+APX.NDD+APX.SIZED, or,2,APX.NF+APX.NDD+APX.SIZED, and,2,APX.NF+APX.NDD+APX.SIZED, \
	sub,2,APX.NF+APX.NDD+APX.SIZED, xor,2,APX.NF+APX.NDD+APX.SIZED, \
	adc,2,APX.NDD+APX.SIZED, sbb,2,APX.NDD+APX.SIZED, \
	inc,1,APX.NF+APX.NDD, dec,1,APX.NF+APX.NDD, neg,1,APX.NF+APX.NDD, not,1,APX.NDD, \
	mul,1,APX.NF, div,1,APX.NF, idiv,1,APX.NF, imul,0,APX.NF+APX.NDD+APX.ZU+APX.SIZED, \
	shl,2,APX.NF+APX.NDD, sal,2,APX.NF+APX.NDD, shr,2,APX.NF+APX.NDD, sar,2,APX.NF+APX.NDD, \
	rol,2,APX.NF+APX.NDD, ror,2,APX.NF+APX.NDD, rcl,2,APX.NDD, rcr,2,APX.NDD, \
	shld,3,APX.NF+APX.NDD+APX.SIZED, shrd,3,APX.NF+APX.NDD+APX.SIZED, \
	popcnt,2,APX.NF, lzcnt,2,APX.NF, tzcnt,2,APX.NF, \
	seto,1,APX.ZU, setno,1,APX.ZU, setc,1,APX.ZU, setb,1,APX.ZU, setnae,1,APX.ZU, \
	setnc,1,APX.ZU, setnb,1,APX.ZU, setae,1,APX.ZU, setz,1,APX.ZU, sete,1,APX.ZU, \
	setnz,1,APX.ZU, setne,1,APX.ZU, setna,1,APX.ZU, setbe,1,APX.ZU, seta,1,APX.ZU, \
	setnbe,1,APX.ZU, sets,1,APX.ZU, setns,1,APX.ZU, setp,1,APX.ZU, setpe,1,APX.ZU, \
	setnp,1,APX.ZU, setpo,1,APX.ZU, setl,1,APX.ZU, setnge,1,APX.ZU, setnl,1,APX.ZU, \
	setge,1,APX.ZU, setng,1,APX.ZU, setle,1,APX.ZU, setg,1,APX.ZU, setnle,1,APX.ZU, \
	cmovo,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovno,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovc,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovb,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovnae,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovnc,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovnb,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovae,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovz,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmove,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovnz,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovne,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovna,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovbe,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmova,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovnbe,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovs,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovns,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovp,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovpe,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovnp,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovpo,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovl,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovnge,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovnl,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovge,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovng,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovle,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, \
	cmovg,2,APX.NDD+APX.SIZED+APX.NDD_ONLY, cmovnle,2,APX.NDD+APX.SIZED+APX.NDD_ONLY

	calminstruction instr? operands&
		local	line, a, b
		check	x86.apx.evex		; called by APX.promote: the legacy form
		jyes	legacy
		match	{a} b, operands		; {nf} or {zu} after the mnemonic
		jyes	promote
		check	x86.vexsize = 4		; {evex}
		jyes	promote
		check	flags and APX.NDD
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
		xcall	APX.promote, <instr>,(count),(flags),operands
	end calminstruction

end iterate

; cfcmov<cc>: conditionally faulting cmov (EVEX map 4, row 4, NF = 1 except
; for the load form). The forms map onto the legacy cmov<cc> r, r/m:
;	cfcmov<cc> r, m			ND=0 NF=0	cmov<cc> r, m
;	cfcmov<cc> r/m, r		ND=0 NF=1	cmov<cc> r, r/m, operands swapped
;	cfcmov<cc> ndd, r, r/m		ND=1 NF=1	cmov<cc> r, r/m with NDD
calminstruction APX.cfcmov name*,operands&
	local	a, b, rest, flags
	match	a=,b=,rest, operands
	jyes	new_destination
	match	a=,b, operands
	jno	invalid
	call	x86.parse_operand@aux, a
	check	@aux.type = 'reg'
	jno	store_form
	call	x86.parse_operand@aux, b
	check	@aux.type = 'mem'
	jno	store_form
	compute flags, APX.SIZED
	xcall	APX.promote, name,(2),flags,operands
	exit
    store_form:
	arrange rest, b=,a
	compute flags, APX.SIZED + APX.FORCE_NF
	xcall	APX.promote, name,(2),flags,rest
	exit
    new_destination:
	compute flags, APX.NDD + APX.SIZED + APX.FORCE_NF
	xcall	APX.promote, name,(2),flags,operands
	exit
    invalid:
	err	'invalid combination of operands'
end calminstruction

iterate cc, o,no,c,b,nae,nc,nb,ae,z,e,nz,ne,na,be,a,nbe,s,ns,p,pe,np,po,l,nge,nl,ge,ng,le,g,nle

	calminstruction cfcmov#cc? operands&
		xcall	APX.cfcmov, <cmov#cc>,operands
	end calminstruction

end iterate

; ccmp<scc>, ctest<scc>: cmp/test when the source condition holds, else set
; OF, SF, ZF, CF to the default flags value. EVEX map 4 with cmp's and
; test's opcodes; P1.vvvv = [OF SF ZF CF], P2[3:0] = SCC. The SCC names are
; the condition codes, with t (true, 1010b) and f (false, 1011b) in place
; of p and np. The default flags value comes first:
;	ccmpb {dfv=of,cf} rax, rbx	a set of flags, the comma after it optional
;	ccmpb 9, rax, rbx		or a number 0-15 (OF=8, SF=4, ZF=2, CF=1)
calminstruction APX.conditional name*,scc*,operands&
	local	dfv, flags, flag, rest, line
	call	x86.require.bits64
	call	x86.require.APX_F
	compute dfv, 0
	check	scc and 10h		; {evex} cmp/test: no default flags value operand
	jno	dfv_operand
	compute scc, scc and 0Fh
	arrange rest, operands
	jump	operands_ready
    dfv_operand:
	match	{=dfv?==} rest, operands
	jyes	dfv_ready
	match	{=dfv?==flags} rest, operands
	jyes	next_flag
	match	flags=,rest, operands
	jno	dfv_missing
	call	x86.parse_operand@aux, flags
	check	@aux.type = 'imm' & @aux.imm relativeto 0
	jno	dfv_missing
	check	@aux.imm >= 0 & @aux.imm < 16
	jno	dfv_invalid
	compute dfv, @aux.imm
	jump	operands_ready
    next_flag:
	match	flag=,flags, flags
	jyes	one_flag
	arrange flag, flags
	arrange flags,
    one_flag:
	match	=of?, flag
	jyes	flag_of
	match	=sf?, flag
	jyes	flag_sf
	match	=zf?, flag
	jyes	flag_zf
	match	=cf?, flag
	jyes	flag_cf
	jump	dfv_invalid
    flag_of:
	compute dfv, dfv or 8
	jump	flag_done
    flag_sf:
	compute dfv, dfv or 4
	jump	flag_done
    flag_zf:
	compute dfv, dfv or 2
	jump	flag_done
    flag_cf:
	compute dfv, dfv or 1
    flag_done:
	match	, flags
	jno	next_flag
    dfv_ready:
	match	=,flags, rest		; the comma after {dfv=...} is optional
	jno	operands_ready
	arrange rest, flags
    operands_ready:
	compute x86.apx.dfv, dfv
	compute x86.apx.scc, scc
	compute x86.apx.nd, 0
	compute x86.apx.nf, 0
	compute x86.apx.v, 0
	compute x86.apx.done, 0
	compute x86.apx.ccmp, 1
	compute x86.apx.evex, 1
	arrange line, name rest
	assemble line
	compute x86.apx.evex, 0
	compute x86.apx.ccmp, 0
	check	x86.apx.done
	jyes	done
	err	'no EVEX form for this instruction'
	exit
    dfv_missing:
	err	'default flags value expected'
	exit
    dfv_invalid:
	err	'invalid default flags value'
    done:
end calminstruction

iterate <cc,code>, o,0, no,1, b,2, c,2, nae,2, nb,3, nc,3, ae,3, z,4, e,4, nz,5, ne,5, \
		   be,6, na,6, nbe,7, a,7, s,8, ns,9, t,0Ah, f,0Bh, \
		   l,0Ch, nge,0Ch, nl,0Dh, ge,0Dh, le,0Eh, ng,0Eh, nle,0Fh, g,0Fh

	calminstruction ccmp#cc? operands&
		xcall	APX.conditional, <cmp>,(code),operands
	end calminstruction

	calminstruction ctest#cc? operands&
		xcall	APX.conditional, <test>,(code),operands
	end calminstruction

end iterate

; {evex} cmp/test: EVEX map 4 form = ccmpt/ctestt {dfv=} (as llvm-mc encodes it)
iterate instr, cmp, test

	calminstruction instr? operands&
		local	line
		check	x86.apx.evex | x86.vexsize <> 4
		jyes	legacy
		xcall	APX.conditional, <instr>,(1Ah),operands
		exit
	    legacy:
		arrange line, =instr operands
		assemble line
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
