; svm.inc -- AMD-V instruction encodings (APM vol.3)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting	CPUID bit		instructions
;	SVM	8000_0001h:ECX[2]	vmrun vmload vmsave vmmcall clgi invlpga
;	SKINIT	8000_0001h:ECX[12]	skinit
;	SVM or SKINIT			stgi
;
; Valid in 16-, 32- and 64-bit code.

; The rAX operand is implicit and optional; when given, its width selects
; the address size (67h), never the operand size.

iterate <instr,opcode>, vmrun,0xD8, vmload,0xDA, vmsave,0xDB

	calminstruction instr? addr
		call	x86.require.SVM
		match	, addr
		jyes	encode
		call	x86.parse_operand@dest, addr
		check	@dest.type = 'reg' & @dest.rm = 0
		jno	invalid
		call	x86.store_address_prefix, @dest.size
	encode:
		emit	3, 0x010F or (opcode shl 16)
		exit
	invalid:
		err	'only rAX register supported'
	end calminstruction

end iterate

iterate <instr,opcode>,\
	vmmcall,	0xD9010F,\
	clgi,		0xDD010F

	calminstruction instr?
		call	x86.require.SVM
		emit	3, opcode
	end calminstruction

end iterate

; STGI is present when either SVM or SKINIT is reported
calminstruction stgi?
	check	x86.ext and (x86.SVM.ext or x86.SKINIT.ext)
	jyes	supported
	err	'SVM or SKINIT required'
	exit
supported:
	emit	3, 0xDC010F
end calminstruction

calminstruction skinit? base ; EAX = SLB base
	call	x86.require.SKINIT
	match	, base
	jyes	encode
	call	x86.parse_operand@dest, base
	check	@dest.type = 'reg' & @dest.rm = 0 & @dest.size = 4
	jno	invalid
encode:
	emit 3, 0xDE010F
	exit
invalid:
	err 'register EAX expected'
end calminstruction

calminstruction invlpga? addr,asid
	call	x86.require.SVM
	match	, asid
	jyes	address
	call	x86.parse_operand@src, asid
	check	@src.type = 'reg' & @src.rm = 1 & @src.size = 4
	jno	invalid
address:
	match	, addr
	jyes	encode
	call	x86.parse_operand@dest, addr
	check	@dest.type = 'reg' & @dest.rm = 0
	jno	invalid
	call	x86.store_address_prefix, @dest.size
encode:
	emit	3, 0xDF010F
	exit
invalid:
	err 'invlpga operands must be rAX and ECX'
end calminstruction
