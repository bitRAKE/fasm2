; newcoffcv.inc - CodeView debug support for the NEWCOFF backend
; ============================================================================
; Included by newcoffms.inc; scope, format background and the progression
; plan live in docs/newcoffcv.md. Everything here follows the backend's records
; discipline: markers append staging records during the pass, and
; codeview_synthesize (called from the newcoffms POSTPONE, after the machine
; is known and before the counts are taken) turns them into C13 '.debug$S'
; sections plus - on AMD64 - '.pdata'/'.xdata' unwind sections. Debug
; sections for COMDAT code are COMDAT ASSOCIATIVE to it, so /OPT:REF
; discards code and its debug data together.
;
; Markers (all optional, all record-only until POSTPONE):
;   cvline [line], [file]  source-line marker (defaults: invocation site)
;   cvproc name            open a procedure at the current position
;   cvframe size [, regs]  after the prologue: rsp allocation + pushed regs
;   cvendp                 close the procedure (records its length)
;   cvlabel name           name the current position (S_LABEL32)
;
; newcoff_debug_prologue / newcoff_debug_close wrap the static_rsp proc
; frame so PROC/ENDP mark themselves - including frame size and USES
; registers for the unwind codes:
;   prologue@proc equ newcoff_debug_prologue
;   close@proc    equ newcoff_debug_close
;
; When NEWCOFF.DEBUG is defined non-zero, the unnamed-macro interceptor at
; the bottom tags every line of the main source file automatically.
; ============================================================================

; CodeView symbol record types and constants
S_END	     = 0x0006
S_FRAMEPROC  = 0x1012
S_OBJNAME    = 0x1101
S_LABEL32    = 0x1105
S_GPROC32    = 0x1110
S_COMPILE3   = 0x113C

CV_SIGNATURE_C13	= 4
DEBUG_S_SYMBOLS 	= 0xF1
DEBUG_S_LINES		= 0xF2
DEBUG_S_STRINGTABLE	= 0xF3
DEBUG_S_FILECHKSMS	= 0xF4

CV_CFL_MASM	= 0x03
CV_CFL_80386	= 0x03
CV_CFL_X64	= 0xD0

struct LINE_RECORD, packed
	; 'file' and 'section' are taken (directive / macro), hence the names
	srcfile 	dd ?	; ordinal into cv_files (FileID = ordinal * 8)
	line		dd ?
	sect		dd ?	; 0-based section index
	ofs		dd ?	; section-relative code offset
ends

struct PROC_RECORD, packed
	name_ofs	dd ?	; into the cv_names pool
	name_len	dd ?
	sect		dd ?
	start		dd ?	; section-relative offset of the entry
	code_len	dd ?	; filled by cvendp
	rsp_alloc	dd ?	; bytes subtracted from rsp (0 = leaf); 'frame' is a macro
	prologue	dd ?	; prologue byte length (frame established after)
	reg_count	dd ?	; nonvolatile pushes, in push order
	reg_nibbles	dd ?	; their register numbers, 4 bits each
	xd		dd ?	; unwind entry offset (filled by POSTPONE)
ends

struct LABEL_RECORD, packed
	name_ofs	dd ?
	name_len	dd ?
	sect		dd ?
	ofs		dd ?
ends

namespace NEWCOFF

	; CodeView staging: markers append here; the F3 string table starts
	; with the empty string by C13 convention
	virtual at 0
		line_records::
	end virtual

	virtual at 0
		proc_records::
	end virtual

	virtual at 0
		label_records::
	end virtual

	virtual at 0
		cv_strings:: db 0
	end virtual

	virtual at 0
		cv_files::
	end virtual

	virtual at 0
		cv_names::
	end virtual

	CV_PROC_OPEN = 0

	; ------------------------------------------------------------------
	; POSTPONE synthesis - invoked by newcoffms after the machine is
	; decided and before the record counts are taken
	; ------------------------------------------------------------------
	macro codeview_synthesize
		virtual line_records
			NUMBER_OF_LINES = ($ - $$) / sizeof LINE_RECORD
		end virtual
		virtual proc_records
			NUMBER_OF_PROCS = ($ - $$) / sizeof PROC_RECORD
		end virtual
		virtual label_records
			NUMBER_OF_LABELS = ($ - $$) / sizeof LABEL_RECORD
		end virtual
		if NUMBER_OF_LINES > 0 | NUMBER_OF_PROCS > 0 | NUMBER_OF_LABELS > 0
			virtual section_records
				USER_SECTIONS = ($ - $$) / sizeof SECTION_RECORD
			end virtual
			virtual cv_files
				CV_FILE_COUNT = ($ - $$) / 4
			end virtual
			virtual cv_strings
				CV_STR_SIZE = $
			end virtual

			; one shared, always-kept '.debug$S': producer identification
			; (F1) and the per-object file tables (F3 strings, F4 checksums)
			virtual reloc_records
				DBG_RELOC1 = ($ - $$) / sizeof RELOC_RECORD
			end virtual
			DBG_OFF = $%
			dd CV_SIGNATURE_C13
			; F1: S_OBJNAME + S_COMPILE3
			CV_ONAME = __SOURCE__
			CV_TNAME = 'fasm2 newcoff'
			CV_OTOTAL = 9 + lengthof CV_ONAME
			CV_OPAD = (0 - CV_OTOTAL) and 3
			dd DEBUG_S_SYMBOLS, CV_OTOTAL + CV_OPAD + 40
			dw CV_OTOTAL + CV_OPAD - 2, S_OBJNAME
			dd 0			; signature
			db CV_ONAME, 0
			db CV_OPAD dup 0
			dw 38, S_COMPILE3
			dd CV_CFL_MASM		; language, no extra flags
			if MACHINE = IMAGE_FILE_MACHINE_AMD64
				dw CV_CFL_X64
			else
				dw CV_CFL_80386
			end if
			dw 2, 0, 0, 0		; front end version
			dw 2, 0, 0, 0		; back end version
			db CV_TNAME, 0		; 13+1 chars: record ends 4-aligned
			; F3: file name strings
			dd DEBUG_S_STRINGTABLE, CV_STR_SIZE
			load byte_sequence : CV_STR_SIZE from cv_strings : 0
			db byte_sequence
			db (4 - CV_STR_SIZE) and 3 dup 0
			; F4: checksum table (kind none, for now - see docs/newcoffcv.md)
			dd DEBUG_S_FILECHKSMS, CV_FILE_COUNT * 8
			repeat CV_FILE_COUNT
				load CV_FOFS : 4 from cv_files : (%-1) * 4
				dd CV_FOFS
				db 0, 0
				dw 0
			end repeat
			DBG_SIZE = $% - DBG_OFF
			DBG_FLAGS = IMAGE_SCN_CNT_INITIALIZED_DATA or IMAGE_SCN_MEM_READ or IMAGE_SCN_MEM_DISCARDABLE or IMAGE_SCN_ALIGN_1BYTES
			virtual section_records
				SECTION_RECORD '.debug$S', 0, NEWCOFF.DBG_FLAGS, NEWCOFF.DBG_OFF,\
					NEWCOFF.DBG_SIZE, NEWCOFF.DBG_RELOC1, 0,\
					IMAGE_COMDAT_SELECT_NODUPLICATES, 0, 0, 0, 0
			end virtual

			; one '.debug$S' per CODE section with debug material: lines
			; (F2) and procedure/label symbols (F1), bound to the code by
			; SECREL32+SECTION relocations - ASSOCIATIVE when the code is
			; a COMDAT, so lines and symbols live and die with it
			repeat USER_SECTIONS
				SEC_I = % - 1
				load CS_FLAGS : 4 from section_records : SEC_I * sizeof SECTION_RECORD + SECTION_RECORD.flags
				CV_L = 0
				CV_R = 0
				CV_P = 0
				CV_LB = 0
				if CS_FLAGS and IMAGE_SCN_CNT_CODE
					CV_LASTF = -1
					repeat NUMBER_OF_LINES
						load L_SEC : 4 from line_records : (%-1) * sizeof LINE_RECORD + LINE_RECORD.sect
						if L_SEC = SEC_I
							CV_L = CV_L + 1
							load L_FILE : 4 from line_records : (%-1) * sizeof LINE_RECORD + LINE_RECORD.srcfile
							if L_FILE <> CV_LASTF
								CV_R = CV_R + 1
								CV_LASTF = L_FILE
							end if
						end if
					end repeat
					repeat NUMBER_OF_PROCS
						load P_SECT : 4 from proc_records : (%-1) * sizeof PROC_RECORD + PROC_RECORD.sect
						if P_SECT = SEC_I
							CV_P = CV_P + 1
						end if
					end repeat
					repeat NUMBER_OF_LABELS
						load L_SECT : 4 from label_records : (%-1) * sizeof LABEL_RECORD + LABEL_RECORD.sect
						if L_SECT = SEC_I
							CV_LB = CV_LB + 1
						end if
					end repeat
				end if
				if CV_L > 0 | CV_P > 0 | CV_LB > 0
					load CS_SIZE : 4 from section_records : SEC_I * sizeof SECTION_RECORD + SECTION_RECORD.size
					virtual reloc_records
						DBG_RELOC1 = ($ - $$) / sizeof RELOC_RECORD
					end virtual
					DBG_OFF = $%
					dd CV_SIGNATURE_C13
					if CV_L > 0
						dd DEBUG_S_LINES, 12 + CV_R * 12 + CV_L * 8
						; SECREL32 + SECTION bind the chunk to the code
						DBG_FLD = $% - DBG_OFF
						virtual reloc_records
							RELOC_RECORD NEWCOFF.DBG_FLD, NEWCOFF.SEC_I, 0, NEWCOFF.RELOC_SECREL32
							RELOC_RECORD NEWCOFF.DBG_FLD + 4, NEWCOFF.SEC_I, 0, NEWCOFF.RELOC_SECTION
						end virtual
						dd 0		; RelocOffset (section start)
						dw 0		; RelocSegment
						dw 0		; flags: no column info
						dd CS_SIZE	; code length covered
						CV_IDX = 0
						while CV_IDX < NUMBER_OF_LINES
							load L_SEC : 4 from line_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.sect
							if L_SEC = SEC_I
								; a run: consecutive markers of this section
								; sharing one file (other sections interleave)
								load CV_RUNF : 4 from line_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.srcfile
								CV_N = 0
								CV_SCAN = CV_IDX
								while CV_SCAN < NUMBER_OF_LINES
									load L_SEC : 4 from line_records : CV_SCAN * sizeof LINE_RECORD + LINE_RECORD.sect
									if L_SEC = SEC_I
										load L_FILE : 4 from line_records : CV_SCAN * sizeof LINE_RECORD + LINE_RECORD.srcfile
										if L_FILE <> CV_RUNF
											break
										end if
										CV_N = CV_N + 1
									end if
									CV_SCAN = CV_SCAN + 1
								end while
								dd CV_RUNF * 8, CV_N, 12 + CV_N * 8
								while CV_N > 0
									load L_SEC : 4 from line_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.sect
									if L_SEC = SEC_I
										load L_OFF : 4 from line_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.ofs
										load L_LINE : 4 from line_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.line
										dd L_OFF
										dd (L_LINE and 0xFFFFFF) or 0x80000000
										CV_N = CV_N - 1
									end if
									CV_IDX = CV_IDX + 1
								end while
							else
								CV_IDX = CV_IDX + 1
							end if
						end while
					end if
					if CV_P > 0 | CV_LB > 0
						; F1: S_GPROC32 + S_FRAMEPROC + S_END per procedure
						; (scope pointers 0; the linker rewrites them),
						; S_LABEL32 per label
						F1SIZE = 0
						repeat NUMBER_OF_PROCS
							load P_SECT : 4 from proc_records : (%-1) * sizeof PROC_RECORD + PROC_RECORD.sect
							if P_SECT = SEC_I
								load P_NLEN : 4 from proc_records : (%-1) * sizeof PROC_RECORD + PROC_RECORD.name_len
								F1SIZE = F1SIZE + ((40 + P_NLEN + 3) and (not 3)) + 32 + 4
							end if
						end repeat
						repeat NUMBER_OF_LABELS
							load L_SECT : 4 from label_records : (%-1) * sizeof LABEL_RECORD + LABEL_RECORD.sect
							if L_SECT = SEC_I
								load L_NLEN : 4 from label_records : (%-1) * sizeof LABEL_RECORD + LABEL_RECORD.name_len
								F1SIZE = F1SIZE + ((12 + L_NLEN + 3) and (not 3))
							end if
						end repeat
						dd DEBUG_S_SYMBOLS, F1SIZE
						repeat NUMBER_OF_PROCS
							P_I = % - 1
							load P_SECT : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.sect
							if P_SECT = SEC_I
								load P_NOFS : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.name_ofs
								load P_NLEN : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.name_len
								load P_START : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.start
								load P_LEN : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.code_len
								load P_FRAME : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.rsp_alloc
								load P_PRO : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.prologue
								CV_PAD = (0 - (40 + P_NLEN)) and 3
								dw 38 + P_NLEN + CV_PAD, S_GPROC32
								dd 0, 0, 0	; pParent, pEnd, pNext
								dd P_LEN
								dd P_PRO	; DbgStart: after the prologue
								dd P_LEN	; DbgEnd
								dd 0		; T_NOTYPE
								DBG_FLD = $% - DBG_OFF
								virtual reloc_records
									RELOC_RECORD NEWCOFF.DBG_FLD, NEWCOFF.SEC_I, 0, NEWCOFF.RELOC_SECREL32
									RELOC_RECORD NEWCOFF.DBG_FLD + 4, NEWCOFF.SEC_I, 0, NEWCOFF.RELOC_SECTION
								end virtual
								dd P_START
								dw 0
								db 0		; flags
								load byte_sequence : P_NLEN from cv_names : P_NOFS
								db byte_sequence
								db 0
								db CV_PAD dup 0
								dw 30, S_FRAMEPROC
								dd P_FRAME and 0xFFFFFFFF
								dd 0, 0, 0, 0	; pads, saved regs, handler
								dw 0
								dd 0		; flags
								dw 0		; record pad
								dw 2, S_END
							end if
						end repeat
						repeat NUMBER_OF_LABELS
							L_I = % - 1
							load L_SECT : 4 from label_records : L_I * sizeof LABEL_RECORD + LABEL_RECORD.sect
							if L_SECT = SEC_I
								load L_NOFS : 4 from label_records : L_I * sizeof LABEL_RECORD + LABEL_RECORD.name_ofs
								load L_NLEN : 4 from label_records : L_I * sizeof LABEL_RECORD + LABEL_RECORD.name_len
								load L_LOFS : 4 from label_records : L_I * sizeof LABEL_RECORD + LABEL_RECORD.ofs
								CV_PAD = (0 - (12 + L_NLEN)) and 3
								dw 10 + L_NLEN + CV_PAD, S_LABEL32
								DBG_FLD = $% - DBG_OFF
								virtual reloc_records
									RELOC_RECORD NEWCOFF.DBG_FLD, NEWCOFF.SEC_I, 0, NEWCOFF.RELOC_SECREL32
									RELOC_RECORD NEWCOFF.DBG_FLD + 4, NEWCOFF.SEC_I, 0, NEWCOFF.RELOC_SECTION
								end virtual
								dd L_LOFS
								dw 0
								db 0		; flags
								load byte_sequence : L_NLEN from cv_names : L_NOFS
								db byte_sequence
								db 0
								db CV_PAD dup 0
							end if
						end repeat
					end if
					DBG_SIZE = $% - DBG_OFF
					virtual reloc_records
						DBG_RELOCN = ($ - $$) / sizeof RELOC_RECORD
					end virtual
					DBG_FLAGS = IMAGE_SCN_CNT_INITIALIZED_DATA or IMAGE_SCN_MEM_READ or IMAGE_SCN_MEM_DISCARDABLE or IMAGE_SCN_ALIGN_1BYTES
					DBG_SEL = IMAGE_COMDAT_SELECT_NODUPLICATES
					DBG_ASSOC = 0
					if CS_FLAGS and IMAGE_SCN_LNK_COMDAT
						DBG_FLAGS = DBG_FLAGS or IMAGE_SCN_LNK_COMDAT
						DBG_SEL = IMAGE_COMDAT_SELECT_ASSOCIATIVE
						DBG_ASSOC = SEC_I + 1
					end if
					virtual section_records
						SECTION_RECORD '.debug$S', 0, NEWCOFF.DBG_FLAGS, NEWCOFF.DBG_OFF,\
							NEWCOFF.DBG_SIZE, NEWCOFF.DBG_RELOC1,\
							NEWCOFF.DBG_RELOCN - NEWCOFF.DBG_RELOC1,\
							NEWCOFF.DBG_SEL, NEWCOFF.DBG_ASSOC, 0, 0, 0
					end virtual
				end if

				; ---- unwind information (AMD64): '.xdata' UNWIND_INFO +
				; '.pdata' RUNTIME_FUNCTION per procedure of this section,
				; from the frame facts cvframe recorded ------------------
				if CV_P > 0 & MACHINE = IMAGE_FILE_MACHINE_AMD64
					virtual reloc_records
						XD_RELOC1 = ($ - $$) / sizeof RELOC_RECORD
					end virtual
					XD_OFF = $%
					repeat NUMBER_OF_PROCS
						P_I = % - 1
						load P_SECT : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.sect
						if P_SECT = SEC_I
							store ($% - XD_OFF) and 0xFFFFFFFF : 4 at proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.xd
							load P_FRAME : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.rsp_alloc
							load P_PRO : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.prologue
							load P_REGN : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.reg_count
							load P_REGS : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.reg_nibbles
							XD_SLOTS = P_REGN
							if P_FRAME > 0
								if P_FRAME and 7 | P_FRAME >= 0x80000
									err 'cvframe: allocation must be a multiple of 8 below 512K'
								end if
								if P_FRAME <= 128
									XD_SLOTS = XD_SLOTS + 1
								else
									XD_SLOTS = XD_SLOTS + 2
								end if
							end if
							db 1			; version 1, no flags
							db P_PRO and 0xFF	; prologue size
							db XD_SLOTS and 0xFF	; unwind code slots
							db 0			; no frame register
							; codes in reverse prologue order: the
							; allocation first, then pushes, last first
							if P_FRAME > 0
								if P_FRAME <= 128
									db P_PRO and 0xFF, 0x02 or ((((P_FRAME - 8) shr 3) and 15) shl 4)
								else
									db P_PRO and 0xFF, 0x01
									dw (P_FRAME shr 3) and 0xFFFF
								end if
							end if
							repeat P_REGN
								XD_K = P_REGN - %
								; end offset of push XD_K: r8..r15 need a REX
								XD_CUM = 0
								repeat XD_K + 1
									if ((P_REGS shr (4 * (% - 1))) and 15) >= 8
										XD_CUM = XD_CUM + 2
									else
										XD_CUM = XD_CUM + 1
									end if
								end repeat
								db XD_CUM and 0xFF, (((P_REGS shr (4 * XD_K)) and 15) shl 4)	; UWOP_PUSH_NONVOL
							end repeat
							if XD_SLOTS and 1
								dw 0		; pad slots to an even count
							end if
							db (0 - ($% - XD_OFF)) and 3 dup 0
						end if
					end repeat
					XD_SIZE = $% - XD_OFF
					XD_FLAGS = IMAGE_SCN_CNT_INITIALIZED_DATA or IMAGE_SCN_MEM_READ or IMAGE_SCN_ALIGN_4BYTES
					DBG_SEL = IMAGE_COMDAT_SELECT_NODUPLICATES
					DBG_ASSOC = 0
					if CS_FLAGS and IMAGE_SCN_LNK_COMDAT
						XD_FLAGS = XD_FLAGS or IMAGE_SCN_LNK_COMDAT
						DBG_SEL = IMAGE_COMDAT_SELECT_ASSOCIATIVE
						DBG_ASSOC = SEC_I + 1
					end if
					virtual section_records
						XSEC_I = ($ - $$) / sizeof SECTION_RECORD
					end virtual
					virtual section_records
						SECTION_RECORD '.xdata', 0, NEWCOFF.XD_FLAGS, NEWCOFF.XD_OFF,\
							NEWCOFF.XD_SIZE, NEWCOFF.XD_RELOC1, 0,\
							NEWCOFF.DBG_SEL, NEWCOFF.DBG_ASSOC, 0, 0, 0
					end virtual
					virtual reloc_records
						PD_RELOC1 = ($ - $$) / sizeof RELOC_RECORD
					end virtual
					PD_OFF = $%
					repeat NUMBER_OF_PROCS
						P_I = % - 1
						load P_SECT : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.sect
						if P_SECT = SEC_I
							load P_START : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.start
							load P_LEN : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.code_len
							load P_XD : 4 from proc_records : P_I * sizeof PROC_RECORD + PROC_RECORD.xd
							DBG_FLD = $% - PD_OFF
							virtual reloc_records
								RELOC_RECORD NEWCOFF.DBG_FLD, NEWCOFF.SEC_I, 0, NEWCOFF.RELOC_RVA32
								RELOC_RECORD NEWCOFF.DBG_FLD + 4, NEWCOFF.SEC_I, 0, NEWCOFF.RELOC_RVA32
								RELOC_RECORD NEWCOFF.DBG_FLD + 8, NEWCOFF.XSEC_I, 0, NEWCOFF.RELOC_RVA32
							end virtual
							dd P_START, P_START + P_LEN, P_XD
						end if
					end repeat
					PD_SIZE = $% - PD_OFF
					virtual reloc_records
						PD_RELOCN = ($ - $$) / sizeof RELOC_RECORD
					end virtual
					XD_FLAGS = IMAGE_SCN_CNT_INITIALIZED_DATA or IMAGE_SCN_MEM_READ or IMAGE_SCN_ALIGN_4BYTES
					if CS_FLAGS and IMAGE_SCN_LNK_COMDAT
						XD_FLAGS = XD_FLAGS or IMAGE_SCN_LNK_COMDAT
					end if
					virtual section_records
						SECTION_RECORD '.pdata', 0, NEWCOFF.XD_FLAGS, NEWCOFF.PD_OFF,\
							NEWCOFF.PD_SIZE, NEWCOFF.PD_RELOC1,\
							NEWCOFF.PD_RELOCN - NEWCOFF.PD_RELOC1,\
							NEWCOFF.DBG_SEL, NEWCOFF.DBG_ASSOC, 0, 0, 0
					end virtual
				end if
			end repeat
		end if
	end macro

end namespace

; record a source-line marker at the current position; with no arguments
; the invocation site's line and file are captured (fasmg reports the
; invocation for __LINE__/__FILE__ even from inside a macro)
macro cvline? lnum:__LINE__, fname:__FILE__
	namespace NEWCOFF
		CV_NAME = fname
		virtual cv_files
			CV_FILE_COUNT = ($ - $$) / 4
		end virtual
		virtual cv_strings
			CV_STR_END = $
		end virtual
		CV_ORD = -1
		repeat CV_FILE_COUNT
			load CV_FOFS : 4 from cv_files : (%-1) * 4
			if CV_FOFS + lengthof CV_NAME + 1 <= CV_STR_END
				load CV_CAND : lengthof CV_NAME + 1 from cv_strings : CV_FOFS
				if CV_CAND = CV_NAME
					CV_ORD = % - 1
					break
				end if
			end if
		end repeat
		if CV_ORD < 0
			CV_ORD = CV_FILE_COUNT
			virtual cv_strings
				CV_FOFS = $
				db CV_NAME, 0
			end virtual
			virtual cv_files
				dd CV_FOFS
			end virtual
		end if
		CV_OFFSET = $ - SECTION_BASE
		virtual line_records
			LINE_RECORD NEWCOFF.CV_ORD, lnum, NEWCOFF.SECTION_INDEX, NEWCOFF.CV_OFFSET
		end virtual
	end namespace
end macro

; open a procedure at the current position (before any prologue pushes)
macro cvproc? name*
	namespace NEWCOFF
		if CV_PROC_OPEN
			err 'cvproc: previous procedure still open (missing cvendp)'
		end if
		if name eqtype ''
			CV_PNAME = name
		else
			CV_PNAME = `name
		end if
		virtual cv_names
			CV_PROC_NOFS = $
			db CV_PNAME
		end virtual
		CV_PROC_NLEN = lengthof CV_PNAME
		CV_PROC_SECT = SECTION_INDEX
		CV_PROC_START = $ - SECTION_BASE
		CV_PROC_FRAME = 0
		CV_PROC_PRO = 0
		CV_PROC_REGN = 0
		CV_PROC_REGS = 0
		CV_PROC_OPEN = 1
	end namespace
end macro

; place directly after the prologue: records the rsp allocation, the
; prologue length, and the nonvolatile registers pushed (in push order)
macro cvframe? size*, reglist&
	namespace NEWCOFF
		CV_PROC_FRAME = size
		CV_PROC_PRO = ($ - SECTION_BASE) - CV_PROC_START
		CV_PROC_REGN = 0
		CV_PROC_REGS = 0
		match any, reglist
			iterate reg, reglist
				match =rax?, reg
					CV_REG = 0
				else match =rcx?, reg
					CV_REG = 1
				else match =rdx?, reg
					CV_REG = 2
				else match =rbx?, reg
					CV_REG = 3
				else match =rsp?, reg
					CV_REG = 4
				else match =rbp?, reg
					CV_REG = 5
				else match =rsi?, reg
					CV_REG = 6
				else match =rdi?, reg
					CV_REG = 7
				else match =r8?, reg
					CV_REG = 8
				else match =r9?, reg
					CV_REG = 9
				else match =r10?, reg
					CV_REG = 10
				else match =r11?, reg
					CV_REG = 11
				else match =r12?, reg
					CV_REG = 12
				else match =r13?, reg
					CV_REG = 13
				else match =r14?, reg
					CV_REG = 14
				else match =r15?, reg
					CV_REG = 15
				else
					err 'cvframe: unrecognized register'
					CV_REG = 0
				end match
				CV_PROC_REGS = CV_PROC_REGS or (CV_REG shl (4 * CV_PROC_REGN))
				CV_PROC_REGN = CV_PROC_REGN + 1
			end iterate
		end match
	end namespace
end macro

; close the procedure: its length is now known, append the record
macro cvendp?
	namespace NEWCOFF
		if ~ CV_PROC_OPEN
			err 'cvendp: no open procedure (missing cvproc)'
		end if
		CV_PROC_LEN = ($ - SECTION_BASE) - CV_PROC_START
		virtual proc_records
			PROC_RECORD NEWCOFF.CV_PROC_NOFS, NEWCOFF.CV_PROC_NLEN, NEWCOFF.CV_PROC_SECT,\
				NEWCOFF.CV_PROC_START, NEWCOFF.CV_PROC_LEN,\
				NEWCOFF.CV_PROC_FRAME and 0xFFFFFFFF, NEWCOFF.CV_PROC_PRO,\
				NEWCOFF.CV_PROC_REGN, NEWCOFF.CV_PROC_REGS, 0
		end virtual
		CV_PROC_OPEN = 0
	end namespace
end macro

; name the current position (S_LABEL32) - jump targets, dispatch stubs
macro cvlabel? name*
	namespace NEWCOFF
		if name eqtype ''
			CV_PNAME = name
		else
			CV_PNAME = `name
		end if
		virtual cv_names
			CV_LNOFS = $
			db CV_PNAME
		end virtual
		CV_LNLEN = lengthof CV_PNAME
		CV_LOFS = $ - SECTION_BASE
		virtual label_records
			LABEL_RECORD NEWCOFF.CV_LNOFS, NEWCOFF.CV_LNLEN, NEWCOFF.SECTION_INDEX, NEWCOFF.CV_LOFS
		end virtual
	end namespace
end macro

; drop-in static_rsp frame wrappers: PROC/ENDP mark themselves, with the
; frame size and USES registers captured for the unwind codes
;   prologue@proc equ newcoff_debug_prologue
;   close@proc    equ newcoff_debug_close
macro newcoff_debug_prologue procname,flag,parmbytes,localbytes,reglist
	cvproc procname
	static_rsp_prologue procname,flag,parmbytes,localbytes,reglist
	match any, reglist
		cvframe framebytes@proc, reglist
	else
		cvframe framebytes@proc
	end match
end macro

macro newcoff_debug_close procname,flag,parmbytes,localbytes,reglist
	cvendp
	static_rsp_close procname,flag,parmbytes,localbytes,reglist
end macro

if definite NEWCOFF.DEBUG & NEWCOFF.DEBUG
	macro ? &line& ; tag every line!
		if __FILE__ = __SOURCE__
			cvline
		end if
		line
	end macro
end if
