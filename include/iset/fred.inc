; fred.inc -- flexible return and event delivery, and LKGS
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting	CPUID bit		instructions
;	FRED	7.1:EAX[17]		erets eretu
;	LKGS	7.1:EAX[18]		lkgs
;
; 64-bit code only.
;
; erets/eretu (F2/F3 0F 01 CA) differ from clac (0F 01 CA) only by prefix.
; lkgs (F2 0F 00 /6) is lldt's neighbour (0F 00 /2); without F2, /6 is the
; IA-64 jmpe, which is not implemented.

iterate <instr,opcode>, erets,0xCA010FF2, eretu,0xCA010FF3

	calminstruction instr?
		call	x86.require.FRED
		call	x86.require.bits64
		emit	4, opcode
	end calminstruction

end iterate

; LKGS r/m16 -- load the kernel GS base from a selector
calminstruction lkgs? src*
	call	x86.require.LKGS
	call	x86.require.bits64
	call	x86.parse_operand@src, src
	check	(@src.type = 'reg' & @src.size = 2) | (@src.type = 'mem' & @src.size and not 2 = 0)
	jyes	encode
	check	@src.type = 'reg' | @src.type = 'mem'
	jno	invalid
	err	'invalid operand size'
	exit
    invalid:
	err	'invalid operand'
	exit
    encode:
	compute @src.opcode_prefix, 0F2h
	xcall	x86.store_instruction@src, <0Fh,00h>,(6)
end calminstruction
