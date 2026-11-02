; cmpccxadd.inc -- compare and add if condition is met (CMPccXADD)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	CMPCCXADD	7.1:EAX[7]		cmp<cc>xadd for every condition code
;
; 64-bit code only. VEX-encoded general-purpose instruction: no AVX needed.
;
; cmp<cc>xadd m32/m64, r1, r2 -- VEX.128.66.0F38.W0/W1 E0+cc /r:
; ModRM.rm = memory, ModRM.reg = r1, VEX.vvvv = r2. W follows the size.
; Condition names and aliases follow Jcc (o, no, b/c/nae, nb/nc/ae, z/e,
; nz/ne, be/na, nbe/a, s, ns, p/pe, np/po, l/nge, nl/ge, le/ng, nle/g).
; The APX EVEX forms are not implemented.

iterate <cc,code>, o,0, no,1, b,2, c,2, nae,2, nb,3, nc,3, ae,3, z,4, e,4, nz,5, ne,5, \
		   be,6, na,6, nbe,7, a,7, s,8, ns,9, p,0Ah, pe,0Ah, np,0Bh, po,0Bh, \
		   l,0Ch, nge,0Ch, nl,0Dh, ge,0Dh, le,0Eh, ng,0Eh, nle,0Fh, g,0Fh

	calminstruction cmp#cc#xadd? dest*,src*,src2*
		local	vex_mpw
		call	x86.require.CMPCCXADD
		call	x86.require.bits64
		call	x86.parse_operand@dest, dest
		call	x86.parse_operand@src, src
		call	x86.parse_operand@src2, src2
		check	@dest.type = 'mem' & @src.type = 'reg' & @src2.type = 'reg'
		jyes	operands_ok
		err	'invalid combination of operands'
		exit
	    operands_ok:
		check	(@src.size = 4 | @src.size = 8) & @src2.size = @src.size
		jyes	registers_ok
		err	'invalid operand size'
		exit
	    registers_ok:
		check	@dest.size and not @src.size
		jno	sizes_ok
		err	'operand sizes do not match'
		exit
	    sizes_ok:
		compute vex_mpw, VEX_66_0F38_W0
		check	@src.size = 8
		jno	encode
		compute vex_mpw, VEX_66_0F38_W1
	    encode:
		xcall	AVX.store_gpr_instruction@dest, (16),vex_mpw,(0E0h+code),@src.rm,@src2.rm
	end calminstruction

end iterate
