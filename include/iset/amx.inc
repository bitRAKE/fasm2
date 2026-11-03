; amx.inc -- Advanced Matrix Extensions (tile registers, VEX)
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	AMX_TILE	7.0:EDX[24]		ldtilecfg sttilecfg tileloadd tileloaddt1
;						tilestored tilezero tilerelease
;	AMX_INT8	7.0:EDX[25]		tdpbssd tdpbsud tdpbusd tdpbuud
;	AMX_BF16	7.0:EDX[22]		tdpbf16ps
;	AMX_FP16	7.1:EAX[21]		tdpfp16ps
;	AMX_COMPLEX	7.1:EDX[8]		tcmmimfp16ps tcmmrlfp16ps
;
; AMX executes in 64-bit mode only. tdpfp16ps and the complex products
; check that, and AMX_TILE as CPUID requires; the older instructions do
; not check either yet. Like every VEX instruction, AMX also needs AVX.


define AMX AMX

element AMX.reg

repeat 8, i:0
	element tmm#i? : AMX.reg + i
end repeat

iterate context, @dest,@src,@src2,@aux

	namespace context

		calminstruction AMX.parse_operand#context operand

			call	x86.parse_operand#context, operand

			check	type = 'imm' & size = 0
			jno	ready

			check	imm eq 1 elementof imm & 1 metadataof imm relativeto AMX.reg
			jyes	amx_register

		    ready:
			exit

		    amx_register:
			compute rm, 1 metadataof imm - AMX.reg
			compute type, 'tmmreg'
			compute mod, 11b
			exit

		end calminstruction

		calminstruction AMX.parse_sib_operand#context operand&

			local	b,i
			match	[b=,i], operand
			jyes	mpx_syntax

			take	b, x86.usesib
			compute x86.usesib, 1
			call	AVX_512.parse_operand#context, operand
			take	x86.usesib, b

			check	type <> 'mem' | rm <> 4 | mode = 16
			jyes	invalid_operand
			exit

		    invalid_operand:
			err	'invalid operand'
			exit

		    mpx_syntax:
			local	m
			initsym m, MPX.parse_sib_operand context,
			arrange i, m operand
			assemble i

			check	0 scaleof context.index_part eq 0
			jyes	ok
			err	'invalid index'
		    ok:

		end calminstruction

	end namespace

end iterate

iterate <instr,vex_mpw>, ldtilecfg,VEX_0F38_W0, sttilecfg,VEX_66_0F38_W0

	macro instr? src*
		require AMX_TILE
		AMX.parse_operand@src src
		if @src.type = 'mem'
			if @src.size and not 64
				err 'invalid operand size'
			end if
			AVX.store_apx_instruction@src 16,vex_mpw,49h,0
		else
			err 'invalid operand'
		end if
	end macro

end iterate

iterate <instr,vex_mpw>, tileloadd,VEX_F2_0F38_W0, tileloaddt1,VEX_66_0F38_W0

	macro instr? dest*,src*&
		require AMX_TILE
		AMX.parse_operand@dest dest
		AMX.parse_sib_operand@src src
		if @dest.type = 'tmmreg' & @src.type = 'mem'
			AVX.store_apx_instruction@src 16,vex_mpw,4Bh,@dest.rm
		else
			err 'invalid operand'
		end if
	end macro

end iterate

macro tilestored? dest*,src*&
	require AMX_TILE
	match dtail]=,stail, src
		AMX.parse_sib_operand@dest dest,dtail]
		AMX.parse_operand@src stail
	else
		AMX.parse_sib_operand@dest dest
		AMX.parse_operand@src src
	end match
	if @dest.type = 'mem' & @src.type = 'tmmreg'
		AVX.store_apx_instruction@dest 16,VEX_F3_0F38_W0,4Bh,@src.rm
	else
		err 'invalid operand'
	end if
end macro

macro tilezero? dest*
	require AMX_TILE
	AMX.parse_operand@dest dest
	if @dest.type = 'tmmreg'
		x86.parse_operand@src al
		AVX.store_instruction@src 16,VEX_F2_0F38_W0,49h,@dest.rm
	else
		err 'invalid operand'
	end if
end macro

macro tilerelease?
	require AMX_TILE
	x86.parse_operand@src al
	AVX.store_instruction@src 16,VEX_0F38_W0,49h,0
end macro

iterate <instr,vex_mpw>, tdpbssd,VEX_F2_0F38_W0, tdpbsud,VEX_F3_0F38_W0, tdpbusd,VEX_66_0F38_W0, tdpbuud,VEX_0F38_W0

	macro instr? dest*,src*,src2*
		require AMX_INT8
		AMX.parse_operand@dest dest
		AMX.parse_operand@src src
		AMX.parse_operand@src2 src2
		if @dest.type = 'tmmreg' & @src.type = 'tmmreg' & @src2.type = 'tmmreg'
			AVX.store_instruction@src 16,vex_mpw,5Eh,@dest.rm,@src2.rm
		else
			err 'invalid operand'
		end if
	end macro

end iterate

macro tdpbf16ps? dest*,src*,src2*
	require AMX_BF16
	AMX.parse_operand@dest dest
	AMX.parse_operand@src src
	AMX.parse_operand@src2 src2
	if @dest.type = 'tmmreg' & @src.type = 'tmmreg' & @src2.type = 'tmmreg'
		AVX.store_instruction@src 16,VEX_F3_0F38_W0,5Ch,@dest.rm,@src2.rm
	else
		err 'invalid operand'
	end if
end macro

; AMX-FP16 / AMX-COMPLEX: tmm1, tmm2, tmm3, all three distinct
iterate <instr,vex_mpw,opcode,feature>, 	tdpfp16ps,   VEX_F2_0F38_W0,5Ch,AMX_FP16, 	tcmmimfp16ps,VEX_66_0F38_W0,6Ch,AMX_COMPLEX, 	tcmmrlfp16ps,VEX_0F38_W0,   6Ch,AMX_COMPLEX

	calminstruction instr? dest*,src*,src2*
		call	x86.require.AMX_TILE
		call	x86.require.feature
		call	x86.require.bits64
		call	AMX.parse_operand@dest, dest
		call	AMX.parse_operand@src, src
		call	AMX.parse_operand@src2, src2
		check	@dest.type = 'tmmreg' & @src.type = 'tmmreg' & @src2.type = 'tmmreg'
		jyes	tiles
		err	'invalid operand'
		exit
	    tiles:
		check	@dest.rm = @src.rm | @dest.rm = @src2.rm | @src.rm = @src2.rm
		jno	encode
		err	'tile registers must be distinct'
		exit
	    encode:
		xcall	AVX.store_instruction@src, (16),(vex_mpw),(opcode),@dest.rm,@src2.rm
	end calminstruction

end iterate
