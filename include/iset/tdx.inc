; tdx.inc -- Intel Trust Domain Extensions
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting	CPUID bit		instructions
;	TDX	none (synthetic)	tdcall seamcall seamret seamops
;
; TDX has no CPUID feature bit: it is discovered through the SEAM range
; MSRs and the TDX module. TDX is a synthetic setting (x86.ext bit 480).
; 64-bit code only. tdcall is used by a TD guest, seamcall by the VMM,
; seamret and seamops by the TDX module.
;
; 66 0F 01 CC..CF; without 66h these bytes are other or reserved opcodes.

iterate <instr,opcode>, tdcall,0xCC010F66, seamret,0xCD010F66, seamops,0xCE010F66, seamcall,0xCF010F66

	calminstruction instr?
		call	x86.require.TDX
		call	x86.require.bits64
		emit	4, opcode
	end calminstruction

end iterate
