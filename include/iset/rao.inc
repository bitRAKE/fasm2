; rao.inc -- remote atomic operations (RAO-INT)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	RAO_INT		7.1:EAX[3]		aadd aand aor axor
;
; Valid in 32- and 64-bit code; the 64-bit operand size only in 64-bit code.
; Not in 16-bit code: a 32-bit operand there would need 66h, which selects
; a different instruction (aand) instead.
;
; aadd/aand/aor/axor are NP/66/F2/F3 0F 38 FC /r, memory destination only.
; The APX EVEX forms (map 4) are not implemented.

iterate <instr,prefix>, aadd,0, aand,66h, aor,0F2h, axor,0F3h

	calminstruction instr? dest*,src*
		call	x86.require.RAO_INT
		check	x86.mode = 16
		jno	mode_ok
		err	'instruction not available in 16-bit code'
		exit
	    mode_ok:
		call	x86.parse_operand@dest, dest
		call	x86.parse_operand@src, src
		check	@dest.type = 'mem' & @src.type = 'reg'
		jyes	operands_ok
		err	'invalid combination of operands'
		exit
	    operands_ok:
		check	@src.size = 4 | @src.size = 8
		jyes	register_ok
		err	'invalid operand size'
		exit
	    register_ok:
		check	@dest.size and not @src.size
		jno	sizes_ok
		err	'operand sizes do not match'
		exit
	    sizes_ok:
		compute @dest.opcode_prefix, prefix
		call	x86.select_operand_prefix@dest, @src.size
		xcall	x86.store_instruction@dest, <0Fh,38h,0FCh>,@src.rm
	end calminstruction

end iterate
