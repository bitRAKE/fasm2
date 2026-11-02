; system.inc -- Intel single-instruction system extensions
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting			CPUID bit		instructions
;	INTEL_PT and PTWRITE	7.0:EBX[25], 14h.0:EBX[4] ptwrite
;	PCONFIG			7.0:EDX[18]		pconfig
;	ENCLV			12h.0:EAX[5]		enclv
;
; ptwrite is valid in 32- and 64-bit code: its 16-bit-mode form would need
; a 66h prefix, which the instruction does not allow. pconfig and enclv are
; valid in 16-, 32- and 64-bit code.
;
; ptwrite (F3 0F AE /4, register or memory) sits beside the FSGSBASE group
; (F3 0F AE /0../3, register only) and clwb/tpause (/6). enclv (0F 01 C0)
; sits beside vmcall (0F 01 C1). vmfunc is in vmx.inc, with its VMX gate.

; PTWRITE r/m32 / r/m64 -- write data to the processor trace
calminstruction ptwrite? src*
	call	x86.require.INTEL_PT
	call	x86.require.PTWRITE
	check	x86.mode = 16
	jno	mode_ok
	err	'instruction not available in 16-bit code'
	exit
    mode_ok:
	call	x86.parse_operand@src, src
	check	@src.type = 'reg' | @src.type = 'mem'
	jyes	operand_ok
	err	'invalid operand'
	exit
    operand_ok:
	check	@src.size = 4 | @src.size = 8
	jyes	size_ok
	check	@src.size
	jno	unsized
	err	'invalid operand size'
	exit
    unsized:
	err	'operand size not specified'
	exit
    size_ok:
	compute @src.opcode_prefix, 0F3h
	call	x86.select_operand_prefix@src, @src.size
	xcall	x86.store_instruction@src, <0Fh,0AEh>,(4)
end calminstruction

; PCONFIG -- platform configuration (MKTME keys); EAX selects the leaf
; ENCLV -- SGX oversubscription leaves; EAX selects the leaf
iterate <instr,opcode,feature>, pconfig,0xC5010F,PCONFIG, enclv,0xC0010F,ENCLV

	calminstruction instr?
		call	x86.require.feature
		emit	3, opcode
	end calminstruction

end iterate
