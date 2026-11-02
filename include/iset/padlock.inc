; padlock.inc -- VIA/Zhaoxin PadLock security engine
;
; CPUID requirements ("use <setting>" to enable, or "use everything"):
;
;	setting		CPUID bit		instructions
;	PADLOCK_RNG	C000_0001h:EDX[2]	xstore (xstorerng)
;	PADLOCK_ACE	C000_0001h:EDX[6]	xcryptecb xcryptcbc xcryptctr xcryptcfb xcryptofb
;	PADLOCK_PHE	C000_0001h:EDX[10]	xsha1 xsha256
;	PADLOCK_PMM	C000_0001h:EDX[12]	montmul
;
; These are the "present" bits; software also checks the matching
; "enabled" bit (one higher) at run time. Grouping follows XED.
; Valid in 16-, 32- and 64-bit code, except montmul, which XED lists with
; 16- and 32-bit addressing only, so it is refused in 64-bit code.
;
; All operands are implicit. xcrypt*, xsha* and montmul always carry the
; REP prefix (F3), which is part of the instruction; xstore takes an
; optional "rep" written by the user. 0F A6/A7 with a register ModRM were
; the 386 A-step xbts/ibts (x86-2.inc, gated on exactly 80386).

iterate <instr,opcode>, xstore,0xC0A70F, xstorerng,0xC0A70F

	calminstruction instr?
		call	x86.require.PADLOCK_RNG
		emit	3, opcode
	end calminstruction

end iterate

iterate <instr,opcode,feature>, \
	xcryptecb,0xC8A70FF3,PADLOCK_ACE, xcryptcbc,0xD0A70FF3,PADLOCK_ACE, \
	xcryptctr,0xD8A70FF3,PADLOCK_ACE, xcryptcfb,0xE0A70FF3,PADLOCK_ACE, \
	xcryptofb,0xE8A70FF3,PADLOCK_ACE, \
	xsha1,    0xC8A60FF3,PADLOCK_PHE, xsha256,  0xD0A60FF3,PADLOCK_PHE

	calminstruction instr?
		call	x86.require.feature
		emit	4, opcode
	end calminstruction

end iterate

calminstruction montmul?
	call	x86.require.PADLOCK_PMM
	check	x86.mode = 64
	jno	encode
	err	'instruction not available in 64-bit code'
	exit
    encode:
	emit	4, 0xC0A60FF3
end calminstruction
