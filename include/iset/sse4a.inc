; sse4a.inc -- AMD SSE4a bit-field and streaming-store instructions
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting	CPUID bit		instructions
;	SSE4A	8000_0001h:ECX[6]	extrq insertq movntsd movntss
;
; Valid in 16-, 32- and 64-bit code. xmm0-15 only.
;
; extrq xmm1, xmm2			66 0F 79 /r
; extrq xmm1, length, index		66 0F 78 /0 ib ib
; insertq xmm1, xmm2			F2 0F 79 /r
; insertq xmm1, xmm2, length, index	F2 0F 78 /r ib ib
; movntsd m64, xmm / movntss m32, xmm	F2 / F3 0F 2B /r
; Without a prefix, 0F 78/79 are vmread/vmwrite; 0F 2B is movntps.

; EXTRQ xmm1, xmm2 / EXTRQ xmm1, imm8 (length), imm8 (index)
calminstruction extrq? dest*,src*,index
	local	fields
	call	x86.require.SSE4A
	call	SSE.parse_operand@dest, dest
	check	@dest.type = 'mmreg' & @dest.size = 16
	jno	invalid
	match	, index
	jno	immediate
	call	SSE.parse_operand@src, src
	check	@src.type = 'mmreg' & @src.size = 16
	jno	invalid
	compute @src.opcode_prefix, 66h
	xcall	x86.store_instruction@src, <0Fh,79h>,@dest.rm
	exit
    immediate:
	call	x86.parse_operand@src, src
	call	x86.parse_operand@aux, index
	check	@src.type = 'imm' & @aux.type = 'imm'
	jno	invalid
	check	@src.size and not 1 | @aux.size and not 1
	jyes	out_of_range
	check	@src.imm >= 0 & @src.imm < 100h & @aux.imm >= 0 & @aux.imm < 100h
	jno	out_of_range
	compute fields, @src.imm + @aux.imm shl 8
	compute @dest.opcode_prefix, 66h
	xcall	x86.store_instruction@dest, <0Fh,78h>,(0),(2),fields
	exit
    out_of_range:
	err	'invalid operand size'
	exit
    invalid:
	err	'invalid combination of operands'
end calminstruction

; INSERTQ xmm1, xmm2 / INSERTQ xmm1, xmm2, imm8 (length), imm8 (index)
calminstruction insertq? dest*,src*,length,index
	local	fields
	call	x86.require.SSE4A
	call	SSE.parse_operand@dest, dest
	call	SSE.parse_operand@src, src
	check	@dest.type = 'mmreg' & @dest.size = 16 & @src.type = 'mmreg' & @src.size = 16
	jno	invalid
	compute @src.opcode_prefix, 0F2h
	match	, length
	jno	immediate
	match	, index
	jno	invalid
	xcall	x86.store_instruction@src, <0Fh,79h>,@dest.rm
	exit
    immediate:
	match	, index
	jyes	invalid
	call	x86.parse_operand@src2, length
	call	x86.parse_operand@aux, index
	check	@src2.type = 'imm' & @aux.type = 'imm'
	jno	invalid
	check	@src2.size and not 1 | @aux.size and not 1
	jyes	out_of_range
	check	@src2.imm >= 0 & @src2.imm < 100h & @aux.imm >= 0 & @aux.imm < 100h
	jno	out_of_range
	compute fields, @src2.imm + @aux.imm shl 8
	xcall	x86.store_instruction@src, <0Fh,78h>,@dest.rm,(2),fields
	exit
    out_of_range:
	err	'invalid operand size'
	exit
    invalid:
	err	'invalid combination of operands'
end calminstruction

; MOVNTSD m64, xmm / MOVNTSS m32, xmm -- non-temporal scalar store
iterate <instr,prefix,msize>, movntsd,0F2h,8, movntss,0F3h,4

	calminstruction instr? dest*,src*
		call	x86.require.SSE4A
		call	SSE.parse_operand@dest, dest
		call	SSE.parse_operand@src, src
		check	@dest.type = 'mem' & @src.type = 'mmreg' & @src.size = 16
		jyes	operands_ok
		err	'invalid combination of operands'
		exit
	    operands_ok:
		check	@dest.size and not msize
		jno	encode
		err	'invalid operand size'
		exit
	    encode:
		compute @dest.opcode_prefix, prefix
		xcall	x86.store_instruction@dest, <0Fh,2Bh>,@src.rm
	end calminstruction

end iterate
