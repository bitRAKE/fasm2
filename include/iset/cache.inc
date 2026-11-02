; cache.inc -- cache-line write-back, prefetch and invalidation hints
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	CLWB		7.0:EBX[24]		clwb
;	PREFETCHWT1	7.0:ECX[0]		prefetchwt1
;	PREFETCHI	7.1:EDX[14]		prefetchit0 prefetchit1
;	WBNOINVD	8000_0008h:EBX[9]	wbnoinvd
;
; prefetchit0/1 take a RIP-relative address and exist in 64-bit code only;
; with any other address the opcode is a reserved NOP, so it is refused.
; The others are valid in 16-, 32- and 64-bit code.
;
; Shares opcodes with: tpause/umwait (66/F2 0F AE /6, register form) next
; to clwb (66 0F AE /6, memory only); prefetch/prefetchw (0F 0D /0, /1) next
; to prefetchwt1 (/2); prefetchnta..t2 (0F 18 /0../3) next to prefetchit*.

; CLWB m8 -- write back a cache line, keeping it cached
calminstruction clwb? dest*
	call	x86.require.CLWB
	call	x86.parse_operand@dest, dest
	check	@dest.type = 'mem'
	jyes	memory
	err	'invalid operand'
	exit
    memory:
	check	@dest.size and not 1
	jno	encode
	err	'invalid operand size'
	exit
    encode:
	compute @dest.opcode_prefix, 66h
	xcall	x86.store_instruction@dest, <0Fh,0AEh>,(6)
end calminstruction

; PREFETCHWT1 m8 -- prefetch with intent to write, into the L2
calminstruction prefetchwt1? src*
	call	x86.require.PREFETCHWT1
	call	x86.parse_operand@src, src
	check	@src.type = 'mem'
	jyes	memory
	err	'invalid operand'
	exit
    memory:
	check	@src.size and not 1
	jno	encode
	err	'invalid operand size'
	exit
    encode:
	xcall	x86.store_instruction@src, <0Fh,0Dh>,(2)
end calminstruction

; PREFETCHIT0 / PREFETCHIT1 m8 -- prefetch code, RIP-relative only
iterate <instr,postbyte>, prefetchit0,7, prefetchit1,6

	calminstruction instr? src*
		call	x86.require.PREFETCHI
		call	x86.require.bits64
		call	x86.parse_operand@src, src
		check	@src.type = 'mem'
		jyes	memory
		err	'invalid operand'
		exit
	    memory:
		check	@src.mod = 0 & @src.rm = 5
		jyes	relative
		err	'RIP-relative address expected'
		exit
	    relative:
		check	@src.size and not 1
		jno	encode
		err	'invalid operand size'
		exit
	    encode:
		xcall	x86.store_instruction@src, <0Fh,18h>,(postbyte)
	end calminstruction

end iterate

; WBNOINVD -- write back all caches without invalidating them
calminstruction wbnoinvd?
	call	x86.require.WBNOINVD
	emit	3, 0x090FF3
end calminstruction
