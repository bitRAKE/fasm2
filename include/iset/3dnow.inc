; 3dnow.inc -- AMD 3DNow! and its extensions (MMX registers)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	3DNow		8000_0001h:EDX[31]	femms prefetch pf* pi2fd pavgusb pmulhrw ...
;	3DNowExt	8000_0001h:EDX[30]	pf2iw pi2fw pfnacc pfpnacc pswapd
;	3DNow or PRFCHW	8000_0001h:ECX[8]	prefetchw
;
; Valid in 16-, 32- and 64-bit code. Every arithmetic form is
; 0F 0F /r followed by its suffix byte.


iterate <instr,opcode>, pf2id,1Dh, pfacc,0AEh, pfadd,9Eh, pfmax,0A4h, pfmin,94h, \
			pfmul,0B4h, pfrcp,96h, pfsub,9Ah, pi2fd,0Dh, \
			pfsubr,0AAh, pavgusb,0BFh, pfcmpeq,0B0h, pfcmpge,90h, pfcmpgt,0A0h, \
			pfrsqrt,97h, pmulhrw,0B7h, pfrcpit2,0B6h, pfrsqit1,0A7h

	calminstruction instr? dest*,src*
		call	x86.require.3DNow
		call	MMX.parse_operand@dest, dest
		call	MMX.parse_operand@src, src
		check	@src.size and not @dest.size
		jno	size_ok
		err	'operand sizes do not match'
	    size_ok:
		check	@dest.type = 'mmreg' & (@src.type = 'mem' | @src.type = 'mmreg')
		jno	invalid_combination_of_operands
		xcall	x86.store_instruction@src, <0Fh,0Fh>,@dest.rm,(1),(opcode)
		exit
	    invalid_combination_of_operands:
		err	'invalid combination of operands'
	end calminstruction

end iterate

calminstruction femms?
	call	x86.require.3DNow
	emit	1, 0Fh
	emit	1, 0Eh
end calminstruction

; PREFETCH requires 3DNow; PREFETCHW is also present on parts that report
; only PRFCHW (CPUID 8000_0001h:ECX[8]), which covers Intel since Broadwell
iterate <instr,postbyte,features,message>, \
	prefetch,0,x86.3DNow.ext,'3DNow or higher required', \
	prefetchw,1,x86.3DNow.ext or x86.PRFCHW.ext,'3DNow or PRFCHW required'
	calminstruction instr? src*
		check	x86.ext and (features)
		jyes	supported
		err	message
		exit
	    supported:
		call	MMX.parse_operand@src, src
		check	@src.type = 'mem'
		jno	invalid_operand
		check	@src.size and not 1
		jno	size_ok
		err	'invalid operand size'
	    size_ok:
		xcall	x86.store_instruction@src, <0Fh,0Dh>,(postbyte)
		exit
	    invalid_operand:
		err	'invalid operand'
	end calminstruction
end iterate

; 3DNow! extensions (Athlon): same 0F 0F /r suffix encoding
iterate <instr,opcode>, pf2iw,1Ch, pi2fw,0Ch, pfnacc,8Ah, pfpnacc,8Eh, pswapd,0BBh

	calminstruction instr? dest*,src*
		call	x86.require.3DNowExt
		call	MMX.parse_operand@dest, dest
		call	MMX.parse_operand@src, src
		check	@src.size and not @dest.size
		jno	size_ok
		err	'operand sizes do not match'
		exit
	    size_ok:
		check	@dest.type = 'mmreg' & (@src.type = 'mem' | @src.type = 'mmreg')
		jno	invalid_combination_of_operands
		xcall	x86.store_instruction@src, <0Fh,0Fh>,@dest.rm,(1),(opcode)
		exit
	    invalid_combination_of_operands:
		err	'invalid combination of operands'
	end calminstruction

end iterate
