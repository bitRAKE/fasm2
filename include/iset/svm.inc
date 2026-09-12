; svm.inc -- AMD-V instruction encodings (APM vol.3)

iterate <instr,opcode>, vmrun,0xD8, vmload,0xDA, vmsave,0xDB

	calminstruction instr? dest*
		call	x86.require.SVM
		call	x86.parse_operand@dest, dest
		check	@dest.type = 'reg' & @dest.rm = 0 \
			& (@dest.size = 4 | @dest.size = 8)
		jno	invalid
		call	x86.select_operand_prefix@dest, @dest.size
		xcall	x86.store_instruction@dest, (0Fh),(01h),(opcode)
		exit
	invalid:
		err	'only rAX register supported'
	end calminstruction

end iterate

iterate <instr,opcode>,\
	vmmcall,	0xD9010F,\
	stgi,		0xDC010F,\
	clgi,		0xDD010F

	calminstruction instr?
		call	x86.require.SVM
		emit	3, opcode
	end calminstruction

end iterate

calminstruction skinit? base:eax ; EAX = SLB base
	call	x86.require.SVM
	call	x86.parse_operand@dest, base
	check	@dest.type = 'reg' & @dest.rm = 0 & @dest.size = 4
	jno	invalid
	emit 3, 0xDE010F
	exit
invalid:
	err 'register EAX expected'
end calminstruction

calminstruction invlpga? addr:rax,asid:ecx
	call	x86.require.SVM
	call	x86.parse_operand@dest, addr
	call	x86.parse_operand@src, asid
	check	@dest.type = 'reg' & @dest.rm = 0 \
		& (@dest.size = 4 | @dest.size = 8)
	jno	invalid
	check	@src.type = 'reg' & @src.rm = 1 & @src.size = 4
	jno	invalid
	call	x86.select_operand_prefix@dest, @dest.size
	xcall	x86.store_instruction@dest, <0Fh,01h,0xDF>
	exit
invalid:
	err 'invlpga operands must be rAX and ECX'
end calminstruction
