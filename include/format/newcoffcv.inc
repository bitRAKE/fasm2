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
;   cvlocal n [, n:type]   rsp-relative locals/params of the open proc
;                          (offset derived from the symbol; default type
;                          T_UQUAD 0x77 - see newcoffcv.md for indices)
;   cvlabel name           name the current position (S_LABEL32)
;   cvconst name [, type]  name an integer equate (S_CONSTANT)
;   cvdata name [, type]   name data at the current position (S_LDATA32)
;   cvldata/cvgdata        explicit local/global data symbol variants
;
; newcoff_debug_prologue / newcoff_debug_close wrap the static_rsp proc
; frame so PROC/ENDP mark themselves - including frame size and USES
; registers for the unwind codes:
;   prologue@proc equ newcoff_debug_prologue
;   close@proc    equ newcoff_debug_close
;
; NEWCOFF.DEBUG levels 1..5 retain explicit cvline behavior. At levels >5,
; the unnamed-macro interceptor at the bottom tracks every source file, but
; defers each marker until the line has actually emitted bytes. POSTPONE
; coalesces equal section/offset records (last wins). With
; newcoff_debug_procs installed, PROC parameters and LOCALS declarations
; become S_REGREL32 entries without explicit cvlocal markers.
; ============================================================================

include 'macro/sha256.inc'

; CodeView symbol record types and constants
S_END	     = 0x0006
S_FRAMEPROC  = 0x1012
S_REGREL32   = 0x1111
S_OBJNAME    = 0x1101
S_LABEL32    = 0x1105
S_CONSTANT   = 0x1107
S_LDATA32    = 0x110C
S_GDATA32    = 0x110D
S_GPROC32    = 0x1110
S_COMPILE3   = 0x113C

LF_LONG      = 0x8003
LF_ULONG     = 0x8004

CV_SIGNATURE_C13	= 4
DEBUG_S_SYMBOLS 	= 0xF1
DEBUG_S_LINES		= 0xF2
DEBUG_S_STRINGTABLE	= 0xF3
DEBUG_S_FILECHKSMS	= 0xF4

CV_CFL_MASM	= 0x03
CV_CFL_80386	= 0x03
CV_CFL_X64	= 0xD0

CV_AMD64_RSP	= 335	; CodeView register numbers for S_REGREL32
CV_REG_ESP	= 21

; Curated CodeView simple type aliases. Indices below 0x1000 are predefined
; simple types; pointer spellings combine a mode byte with a kind byte.
CV_T_INT4	= 0x74
CV_T_UINT4	= 0x75
CV_T_QUAD	= 0x76
CV_T_UQUAD	= 0x77	; default cvlocal type: unsigned 64-bit
CV_T_64PVOID	= 0x603	; NearPointer64 | Void

struct LINE_RECORD, packed
	; 'file' and 'section' are taken (directive / macro), hence the names
	srcfile 	dd ?	; ordinal into cv_files (FileID = ordinal * 8)
	line		dd ?
	sect		dd ?	; 0-based section index
	ofs		dd ?	; section-relative code offset
	endofs		dd ?	; automatic line's exclusive end; -1 = explicit cvline
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

struct LOCAL_RECORD, packed
	name_ofs	dd ?
	name_len	dd ?
	prc		dd ?	; owning PROC_RECORD ordinal ('proc' is the macro)
	ofs		dd ?	; rsp-relative offset
	typind		dd ?	; CodeView type index
ends

struct LABEL_RECORD, packed
	name_ofs	dd ?
	name_len	dd ?
	sect		dd ?
	ofs		dd ?
ends

struct CONST_RECORD, packed
	name_ofs	dd ?
	name_len	dd ?
	value		dd ?
	typind		dd ?
	signed		dd ?
ends

struct DATA_RECORD, packed
	name_ofs	dd ?
	name_len	dd ?
	sect		dd ?
	ofs		dd ?
	typind		dd ?
	rectyp		dd ?	; S_LDATA32 or S_GDATA32
ends

namespace NEWCOFF

	; CodeView staging: markers append here; the F3 string table starts
	; with the empty string by C13 convention
	virtual at 0
		line_records::
	end virtual

	virtual at 0
		line_emit_records::
	end virtual

	virtual at 0
		proc_records::
	end virtual

	virtual at 0
		label_records::
	end virtual

	virtual at 0
		local_records::
	end virtual

	virtual at 0
		const_records::
	end virtual

	virtual at 0
		data_records::
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
	CV_IN_PROC_LOCALS = 0
	CV_LINE_PENDING = 0

	; ------------------------------------------------------------------
	; POSTPONE synthesis - invoked by newcoffms after the machine is
	; decided and before the record counts are taken
	; ------------------------------------------------------------------
	macro codeview_synthesize
		virtual line_records
			RAW_NUMBER_OF_LINES = ($ - $$) / sizeof LINE_RECORD
		end virtual
		; Produce the canonical line stream. Automatic records must cover
		; bytes; explicit cvline records remain an intentional escape hatch.
		; Equal adjacent section/offset records are necessarily separated by
		; no emitted bytes, so retain the last one. This also collapses an
		; explicit marker followed by the automatic record for the same bytes.
		repeat RAW_NUMBER_OF_LINES
			CV_RAW_I = % - 1
			load L_FILE : 4 from line_records : CV_RAW_I * sizeof LINE_RECORD + LINE_RECORD.srcfile
			load L_LINE : 4 from line_records : CV_RAW_I * sizeof LINE_RECORD + LINE_RECORD.line
			load L_SEC : 4 from line_records : CV_RAW_I * sizeof LINE_RECORD + LINE_RECORD.sect
			load L_OFF : 4 from line_records : CV_RAW_I * sizeof LINE_RECORD + LINE_RECORD.ofs
			load L_END : 4 from line_records : CV_RAW_I * sizeof LINE_RECORD + LINE_RECORD.endofs
			CV_LINE_KEEP = 0
			if L_END = -1 | L_END > L_OFF
				CV_LINE_KEEP = 1
			end if
			if CV_RAW_I + 1 < RAW_NUMBER_OF_LINES
				load L_NEXT_SEC : 4 from line_records : (CV_RAW_I + 1) * sizeof LINE_RECORD + LINE_RECORD.sect
				load L_NEXT_OFF : 4 from line_records : (CV_RAW_I + 1) * sizeof LINE_RECORD + LINE_RECORD.ofs
				if L_NEXT_SEC = L_SEC & L_NEXT_OFF = L_OFF
					CV_LINE_KEEP = 0
				end if
			end if
			if CV_LINE_KEEP
				virtual line_emit_records
					LINE_RECORD NEWCOFF.L_FILE, NEWCOFF.L_LINE, NEWCOFF.L_SEC, NEWCOFF.L_OFF, NEWCOFF.L_END
				end virtual
			end if
		end repeat
		virtual line_emit_records
			NUMBER_OF_LINES = ($ - $$) / sizeof LINE_RECORD
		end virtual
		virtual proc_records
			NUMBER_OF_PROCS = ($ - $$) / sizeof PROC_RECORD
		end virtual
		virtual label_records
			NUMBER_OF_LABELS = ($ - $$) / sizeof LABEL_RECORD
		end virtual
		virtual local_records
			NUMBER_OF_CVLOCALS = ($ - $$) / sizeof LOCAL_RECORD
		end virtual
		virtual const_records
			NUMBER_OF_CONSTS = ($ - $$) / sizeof CONST_RECORD
		end virtual
		virtual data_records
			NUMBER_OF_DATA = ($ - $$) / sizeof DATA_RECORD
		end virtual
		if NUMBER_OF_LINES > 0 | NUMBER_OF_PROCS > 0 | NUMBER_OF_LABELS > 0 | NUMBER_OF_CONSTS > 0 | NUMBER_OF_DATA > 0
			virtual section_records
				USER_SECTIONS = ($ - $$) / sizeof SECTION_RECORD
			end virtual
			virtual cv_files
				CV_FILE_COUNT = ($ - $$) / 8
			end virtual
			virtual cv_strings
				CV_STR_SIZE = $
			end virtual

			; one shared, always-kept '.debug$S': producer identification
			; (F1), constants, and the per-object file tables (F3 strings,
			; F4 checksums)
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
			CV_F1SIZE = CV_OTOTAL + CV_OPAD + 40
			repeat NUMBER_OF_CONSTS
				load C_NLEN : 4 from const_records : (%-1) * sizeof CONST_RECORD + CONST_RECORD.name_len
				load C_VAL : 4 from const_records : (%-1) * sizeof CONST_RECORD + CONST_RECORD.value
				load C_NEG : 4 from const_records : (%-1) * sizeof CONST_RECORD + CONST_RECORD.signed
				if C_NEG = 0 & C_VAL < 0x8000
					CV_F1SIZE = CV_F1SIZE + ((11 + C_NLEN + 3) and (not 3))
				else
					CV_F1SIZE = CV_F1SIZE + ((15 + C_NLEN + 3) and (not 3))
				end if
			end repeat
			dd DEBUG_S_SYMBOLS, CV_F1SIZE
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
			; S_CONSTANT: module-scope integer equates
			repeat NUMBER_OF_CONSTS
				C_I = % - 1
				load C_NOFS : 4 from const_records : C_I * sizeof CONST_RECORD + CONST_RECORD.name_ofs
				load C_NLEN : 4 from const_records : C_I * sizeof CONST_RECORD + CONST_RECORD.name_len
				load C_VAL : 4 from const_records : C_I * sizeof CONST_RECORD + CONST_RECORD.value
				load C_TY : 4 from const_records : C_I * sizeof CONST_RECORD + CONST_RECORD.typind
				load C_NEG : 4 from const_records : C_I * sizeof CONST_RECORD + CONST_RECORD.signed
				if C_NEG = 0 & C_VAL < 0x8000
					CV_PAD = (0 - (11 + C_NLEN)) and 3
					dw 9 + C_NLEN + CV_PAD, S_CONSTANT
					dd C_TY and 0xFFFFFFFF
					dw C_VAL and 0xFFFF
				else
					CV_PAD = (0 - (15 + C_NLEN)) and 3
					dw 13 + C_NLEN + CV_PAD, S_CONSTANT
					dd C_TY and 0xFFFFFFFF
					if C_NEG
						dw LF_LONG
					else
						dw LF_ULONG
					end if
					dd C_VAL and 0xFFFFFFFF
				end if
				load byte_sequence : C_NLEN from cv_names : C_NOFS
				db byte_sequence
				db 0
				db CV_PAD dup 0
			end repeat
			; F3: file name strings
			dd DEBUG_S_STRINGTABLE, CV_STR_SIZE
			load byte_sequence : CV_STR_SIZE from cv_strings : 0
			db byte_sequence
			db (4 - CV_STR_SIZE) and 3 dup 0
			; F4: file checksums - SHA-256 (kind 3) of each source's
			; bytes, hashed in place (40-byte entries; the FileID used
			; by F2 is the entry's byte offset, ordinal * 40).
			; NEWCOFF.NOCHECKSUM=1 skips the hashing (kind none, zeros)
			dd DEBUG_S_FILECHKSMS, CV_FILE_COUNT * 40
			repeat CV_FILE_COUNT
				load CV_FOFS : 4 from cv_files : (%-1) * 8
				load CV_FLEN : 4 from cv_files : (%-1) * 8 + 4
				cv_emit_checksum CV_FOFS, CV_FLEN
			end repeat
			; re-anchor the current label: SHA256.calc's macro-local
			; pad became the parent, and anonymous struct instances
			; born under a macro-local lose directive resolution
			; (the namespace-of-a-local rule)
			cv_reanchor:
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
				load CS_SIZE : 4 from section_records : SEC_I * sizeof SECTION_RECORD + SECTION_RECORD.size
				CV_L = 0
				CV_R = 0
				CV_P = 0
				CV_LB = 0
				CV_D = 0
				if CS_FLAGS and IMAGE_SCN_CNT_CODE
					; Explicit markers at ofs >= size have no addressable
					; byte. Automatic records are already byte-backed, but
					; keep the bounds check in every phase for cvline.
					CV_LASTF = -1
					repeat NUMBER_OF_LINES
						load L_SEC : 4 from line_emit_records : (%-1) * sizeof LINE_RECORD + LINE_RECORD.sect
						load L_OFF : 4 from line_emit_records : (%-1) * sizeof LINE_RECORD + LINE_RECORD.ofs
						if L_SEC = SEC_I & L_OFF < CS_SIZE
							CV_L = CV_L + 1
							load L_FILE : 4 from line_emit_records : (%-1) * sizeof LINE_RECORD + LINE_RECORD.srcfile
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
				repeat NUMBER_OF_DATA
					load D_SECT : 4 from data_records : (%-1) * sizeof DATA_RECORD + DATA_RECORD.sect
					if D_SECT = SEC_I
						CV_D = CV_D + 1
					end if
				end repeat
				if CV_L > 0 | CV_P > 0 | CV_LB > 0 | CV_D > 0
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
							load L_SEC : 4 from line_emit_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.sect
							load L_OFF : 4 from line_emit_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.ofs
							if L_SEC = SEC_I & L_OFF < CS_SIZE
								; a run: consecutive markers of this section
								; sharing one file (other sections interleave)
								load CV_RUNF : 4 from line_emit_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.srcfile
								CV_N = 0
								CV_SCAN = CV_IDX
								while CV_SCAN < NUMBER_OF_LINES
									load L_SEC : 4 from line_emit_records : CV_SCAN * sizeof LINE_RECORD + LINE_RECORD.sect
									load L_OFF : 4 from line_emit_records : CV_SCAN * sizeof LINE_RECORD + LINE_RECORD.ofs
									if L_SEC = SEC_I & L_OFF < CS_SIZE
										load L_FILE : 4 from line_emit_records : CV_SCAN * sizeof LINE_RECORD + LINE_RECORD.srcfile
										if L_FILE <> CV_RUNF
											break
										end if
										CV_N = CV_N + 1
									end if
									CV_SCAN = CV_SCAN + 1
								end while
								dd CV_RUNF * 40, CV_N, 12 + CV_N * 8
								while CV_N > 0
									load L_SEC : 4 from line_emit_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.sect
									load L_OFF : 4 from line_emit_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.ofs
									if L_SEC = SEC_I & L_OFF < CS_SIZE
										load L_LINE : 4 from line_emit_records : CV_IDX * sizeof LINE_RECORD + LINE_RECORD.line
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
					if CV_P > 0 | CV_LB > 0 | CV_D > 0
						; F1: S_GPROC32 + S_FRAMEPROC + S_END per procedure
						; (scope pointers 0; the linker rewrites them),
						; S_LABEL32 per label, S_GDATA32/S_LDATA32 per data symbol
						F1SIZE = 0
						repeat NUMBER_OF_PROCS
							FP_I = % - 1
							load P_SECT : 4 from proc_records : FP_I * sizeof PROC_RECORD + PROC_RECORD.sect
							if P_SECT = SEC_I
								load P_NLEN : 4 from proc_records : FP_I * sizeof PROC_RECORD + PROC_RECORD.name_len
								F1SIZE = F1SIZE + ((40 + P_NLEN + 3) and (not 3)) + 32 + 4
								repeat NUMBER_OF_CVLOCALS
									load LR_PROC : 4 from local_records : (%-1) * sizeof LOCAL_RECORD + LOCAL_RECORD.prc
									if LR_PROC = FP_I
										load LR_NLEN : 4 from local_records : (%-1) * sizeof LOCAL_RECORD + LOCAL_RECORD.name_len
										F1SIZE = F1SIZE + ((15 + LR_NLEN + 3) and (not 3))
									end if
								end repeat
							end if
						end repeat
						repeat NUMBER_OF_LABELS
							load L_SECT : 4 from label_records : (%-1) * sizeof LABEL_RECORD + LABEL_RECORD.sect
							if L_SECT = SEC_I
								load L_NLEN : 4 from label_records : (%-1) * sizeof LABEL_RECORD + LABEL_RECORD.name_len
								F1SIZE = F1SIZE + ((12 + L_NLEN + 3) and (not 3))
							end if
						end repeat
						repeat NUMBER_OF_DATA
							load D_SECT : 4 from data_records : (%-1) * sizeof DATA_RECORD + DATA_RECORD.sect
							if D_SECT = SEC_I
								load D_NLEN : 4 from data_records : (%-1) * sizeof DATA_RECORD + DATA_RECORD.name_len
								F1SIZE = F1SIZE + ((15 + D_NLEN + 3) and (not 3))
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
								; S_REGREL32: named rsp-relative locals
								repeat NUMBER_OF_CVLOCALS
									LR_I = % - 1
									load LR_PROC : 4 from local_records : LR_I * sizeof LOCAL_RECORD + LOCAL_RECORD.prc
									if LR_PROC = P_I
										load LR_NOFS : 4 from local_records : LR_I * sizeof LOCAL_RECORD + LOCAL_RECORD.name_ofs
										load LR_NLEN : 4 from local_records : LR_I * sizeof LOCAL_RECORD + LOCAL_RECORD.name_len
										load LR_OFS : 4 from local_records : LR_I * sizeof LOCAL_RECORD + LOCAL_RECORD.ofs
										load LR_TY : 4 from local_records : LR_I * sizeof LOCAL_RECORD + LOCAL_RECORD.typind
										CV_PAD = (0 - (15 + LR_NLEN)) and 3
										dw 13 + LR_NLEN + CV_PAD, S_REGREL32
										dd LR_OFS and 0xFFFFFFFF
										dd LR_TY and 0xFFFFFFFF
										if MACHINE = IMAGE_FILE_MACHINE_AMD64
											dw CV_AMD64_RSP
										else
											dw CV_REG_ESP
										end if
										load byte_sequence : LR_NLEN from cv_names : LR_NOFS
										db byte_sequence
										db 0
										db CV_PAD dup 0
									end if
								end repeat
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
						repeat NUMBER_OF_DATA
							D_I = % - 1
							load D_SECT : 4 from data_records : D_I * sizeof DATA_RECORD + DATA_RECORD.sect
							if D_SECT = SEC_I
								load D_NOFS : 4 from data_records : D_I * sizeof DATA_RECORD + DATA_RECORD.name_ofs
								load D_NLEN : 4 from data_records : D_I * sizeof DATA_RECORD + DATA_RECORD.name_len
								load D_OFS : 4 from data_records : D_I * sizeof DATA_RECORD + DATA_RECORD.ofs
								load D_TY : 4 from data_records : D_I * sizeof DATA_RECORD + DATA_RECORD.typind
								load D_KIND : 4 from data_records : D_I * sizeof DATA_RECORD + DATA_RECORD.rectyp
								CV_PAD = (0 - (15 + D_NLEN)) and 3
								dw 13 + D_NLEN + CV_PAD, D_KIND and 0xFFFF
								dd D_TY and 0xFFFFFFFF
								DBG_FLD = $% - DBG_OFF
								virtual reloc_records
									RELOC_RECORD NEWCOFF.DBG_FLD, NEWCOFF.SEC_I, 0, NEWCOFF.RELOC_SECREL32
									RELOC_RECORD NEWCOFF.DBG_FLD + 4, NEWCOFF.SEC_I, 0, NEWCOFF.RELOC_SECTION
								end virtual
								dd D_OFS
								dw 0
								load byte_sequence : D_NLEN from cv_names : D_NOFS
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

	; one 40-byte F4 entry: the string offset, then the SHA-256 of the
	; file's bytes - the generator statement feeds the hash directly,
	; nothing is copied ('file' accepts the string value loaded from the
	; name table); SHA256.result is the 32-byte digest string
	macro cv_emit_checksum? strofs*, strlen*
		load CV_HNAME : strlen from cv_strings : strofs
		dd strofs
		if definite NEWCOFF.NOCHECKSUM & NEWCOFF.NOCHECKSUM
			db 0, 0 		; no checksum recorded
			db 32 dup 0
		else
			SHA256.calc file NEWCOFF.CV_HNAME
			db 32, 3		; CbChecksum, ChecksumKind: SHA-256
			db SHA256.result
		end if
		dw 0			; pad the entry to 40 bytes
	end macro

end namespace

; Append one source-line marker. Automatic tracking supplies the byte span;
; endofs=-1 preserves cvline as an explicit position marker.
macro cvline_record? lnum*, fname*, sect*, ofs*, endofs*
	namespace NEWCOFF
		CV_NAME = fname
		virtual cv_files
			CV_FILE_COUNT = ($ - $$) / 8
		end virtual
		CV_ORD = -1
		repeat CV_FILE_COUNT
			load CV_FLEN : 4 from cv_files : (%-1) * 8 + 4
			if CV_FLEN = lengthof CV_NAME
				load CV_FOFS : 4 from cv_files : (%-1) * 8
				load CV_CAND : CV_FLEN from cv_strings : CV_FOFS
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
				dd CV_FOFS, lengthof CV_NAME
			end virtual
		end if
		virtual line_records
			LINE_RECORD NEWCOFF.CV_ORD, lnum, sect, ofs, endofs
		end virtual
	end namespace
end macro

; record an explicit source-line marker at the current position; with no
; arguments the invocation site's line and file are captured (fasmg reports
; the invocation for __LINE__/__FILE__ even from inside a macro)
macro cvline? lnum:__LINE__, fname:__FILE__
	namespace NEWCOFF
		CV_OFFSET = $ - SECTION_BASE
	end namespace
	cvline_record lnum, fname, NEWCOFF.SECTION_INDEX, NEWCOFF.CV_OFFSET, -1
end macro

; Verbose automatic line tracking is deferred by one source line. At the
; next intercepted line (or section close), commit only if the pending line
; grew the same section. This records included sources without filling the
; PDB with directives, labels, declarations, or other zero-byte lines.
macro newcoff_debug_line_flush?
	namespace NEWCOFF
		if CV_LINE_PENDING & $ relativeto SECTION_BASE
			CV_LINE_END = $ - SECTION_BASE
			if CV_LINE_END > CV_LINE_START
				cvline_record CV_LINE_NUMBER, CV_LINE_FILE, CV_LINE_SECTION, CV_LINE_START, CV_LINE_END
			end if
			CV_LINE_PENDING = 0
		end if
	end namespace
end macro

macro newcoff_debug_line_begin? lnum*, fname*
	newcoff_debug_line_flush
	namespace NEWCOFF
		if $ relativeto SECTION_BASE
			CV_LINE_NUMBER = lnum
			CV_LINE_FILE = fname
			CV_LINE_SECTION = SECTION_INDEX
			CV_LINE_START = $ - SECTION_BASE
			CV_LINE_PENDING = 1
		end if
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

; rsp-relative locals/params of the open procedure: the offset is derived
; from the symbol itself (it must be rsp-relative, as static_rsp locals
; and params are), the display name is the symbol's own name. Items may
; carry a CodeView type index (default T_UQUAD):
; cvlocal len, flags:CV_T_UINT4, dst:CV_T_64PVOID
macro cvlocal? args&
	namespace NEWCOFF
		if ~ CV_PROC_OPEN
			err 'cvlocal: no open procedure (cvproc first)'
		end if
	end namespace
	iterate item, args
		match nm:ty, item
			cvlocal_one nm, `nm, ty, 1
		else
			cvlocal_one item, `item, CV_T_UQUAD, 1
		end match
	end iterate
end macro

; strict=0 skips silently when the symbol is absent or not rsp-relative
; (used by the automatic proc-parameter harvest, which sees declaration
; tokens that are not always parameters)
macro cvlocal_one? ref*, namestr*, ty*, strict*
	namespace NEWCOFF
		CV_LOK = 0
		if defined ref
			if ref relativeto rsp
				CV_LOK = 1
			end if
		end if
		if CV_LOK
			CV_LNAME = namestr
			virtual cv_names
				CV_LNOFS = $
				db CV_LNAME
			end virtual
			CV_LNLEN = lengthof CV_LNAME
			CV_LOFS = ref - rsp
			virtual proc_records
				CV_LPROC = ($ - $$) / sizeof PROC_RECORD
			end virtual
			virtual local_records
				LOCAL_RECORD NEWCOFF.CV_LNOFS, NEWCOFF.CV_LNLEN, NEWCOFF.CV_LPROC,\
					NEWCOFF.CV_LOFS, ty
			end virtual
		else if strict
			err 'cvlocal: symbol is not rsp-relative'
		end if
	end namespace
end macro

; harvest parameter names from a PROC declaration's comma list. The
; declaration tokens were read BEFORE the proc namespace opened, so they
; carry the caller's global context - each name is resolved through the
; proc symbol instead (prnt.nm), which is defined and searchable
macro cv_proc_params? prnt*, params&
	match any, params
		iterate p, params
			match nm:sz, p
				cvlocal_one prnt.nm, `nm, CV_T_UQUAD, 0
			else
				cvlocal_one prnt.p, `p, CV_T_UQUAD, 0
			end match
		end iterate
	end match
end macro

; NEWCOFF-specific proc64 wrapper: automatic PROC parameter and LOCALS harvesting
include 'format/newcoffproc64.inc'

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

; integer equate marker (S_CONSTANT). The constant value is taken from the
; symbol passed in, and the display name is the symbol's own name.
macro cvconst? name*, ty:CV_T_UINT4
	namespace NEWCOFF
		if name eqtype ''
			CV_CNAME = name
		else
			CV_CNAME = `name
		end if
		CV_CVAL = name
		CV_CNEG = 0
		if CV_CVAL < 0
			CV_CNEG = 1
		end if
		virtual cv_names
			CV_CNOFS = $
			db CV_CNAME
		end virtual
		CV_CNLEN = lengthof CV_CNAME
		virtual const_records
			CONST_RECORD NEWCOFF.CV_CNOFS, NEWCOFF.CV_CNLEN, NEWCOFF.CV_CVAL and 0xFFFFFFFF, ty, NEWCOFF.CV_CNEG
		end virtual
	end namespace
end macro

; data markers name the current position with S_LDATA32/S_GDATA32 and a
; primitive CodeView type index (default T_UQUAD). Public COFF symbols still
; go through the linker normally; these markers add typed/static debugger names.
macro cvdata_one? rectyp*, name*, ty*
	namespace NEWCOFF
		if name eqtype ''
			CV_DNAME = name
		else
			CV_DNAME = `name
		end if
		virtual cv_names
			CV_DNOFS = $
			db CV_DNAME
		end virtual
		CV_DNLEN = lengthof CV_DNAME
		CV_DOFS = $ - SECTION_BASE
		virtual data_records
			DATA_RECORD NEWCOFF.CV_DNOFS, NEWCOFF.CV_DNLEN, NEWCOFF.SECTION_INDEX, NEWCOFF.CV_DOFS, ty, rectyp
		end virtual
	end namespace
end macro

macro cvldata? name*, ty:CV_T_UQUAD
	cvdata_one S_LDATA32, name, ty
end macro

macro cvgdata? name*, ty:CV_T_UQUAD
	cvdata_one S_GDATA32, name, ty
end macro

macro cvdata? name*, ty:CV_T_UQUAD
	cvldata name, ty
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

if definite NEWCOFF.DEBUG & NEWCOFF.DEBUG > 5
	macro ? &line&
		if NEWCOFF.CV_IN_PROC_LOCALS = 0
			newcoff_debug_line_begin __LINE__, __FILE__
		else
			newcoff_debug_line_flush
		end if
		line
	end macro
end if
