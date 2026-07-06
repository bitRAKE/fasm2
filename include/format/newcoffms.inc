; newcoffms.inc - ground-up MS COFF backend (development skeleton)
; ============================================================================
; A re-engineering of include/format/coffms.inc around three rules that the
; 2026 COMDAT retrofits taught us (see tests/newcoff/readme.md and
; docs/coff_comdat_postmortem.md):
;
;   1. RECORDS, NOT STORES. During the pass, every entity (section, public,
;      extern, relocation) appends ONE STRUCT INSTANCE to an extendable
;      virtual block (macro/struct.inc named initializers):
;
;          virtual section_records
;              SECTION_RECORD flags: SECTION_FLAGS, size: SECTION_SIZE, ...
;          end virtual
;
;      Records are append-once and immutable; counts and ordinals are not
;      kept in variables but derived from block lengths,
;      ($ - $$) / sizeof RECORD, wherever they are needed. Changing a
;      record's shape touches the struct definition and its one
;      initializer - no offset arithmetic anywhere. POSTPONE reads the
;      records and emits the whole object once.
;
;   2. CANONICAL SYMBOL ORDER BY CONSTRUCTION. The symbol table is laid out
;      as: section symbols (each with a section-definition aux record),
;      then publics, then synthesized COMDAT statics, then externs. The
;      linker's "section symbol must come first" rule (MSVC LNK1143) is
;      satisfied structurally - no declaration-order constraints, no errors
;      policing them. Symbol indices become arithmetic:
;          section i  -> 2*i
;          public p   -> 2*NSEC + p
;          extern e   -> 2*NSEC + NPUB + e
;
;   3. SYMBOLIC RELOCATION TARGETS. A relocation records WHAT it targets
;      (section number or extern ordinal) and WHAT IT MEANS (a semantic
;      kind: abs32/rva32/rel32/abs64/rva64), never a symbol index or an
;      IMAGE_REL_* number. POSTPONE resolves both in one place - including
;      the fold-safe redirection of a section target to its offset-0
;      external symbol, derived there from a scan of the public records.
;
; One CALM engine (NEWCOFF.record) does all relocation recording; dword?/
; qword? classify the addressing form and delegate. Compare with the four
; copies of the add_relocation tail in the legacy file.
;
; MODERN-ONLY, BY DESIGN. This backend always writes the big object format:
; ANON_OBJECT_HEADER_BIGOBJ + IMAGE_SYMBOL_EX (20-byte symbols, 32-bit
; section numbers - no 32767-section ceiling, no wide-association gap).
; Consumers must understand bigobj (MSVC link, lld-link, LLVM tools,
; binutils >= 2.25); older third-party linkers are served by the legacy
; coffms.inc, which remains untouched. The machine is not a format variant:
; POSTPONE reads x86.mode (use32 -> I386, use64 -> AMD64) and maps the
; semantic relocation kinds to IMAGE_REL_* numbers for that machine.
; ============================================================================

include 'macro/struct.inc'

struct IMAGE_SECTION_HEADER, packed
	Name			db 8 dup ?
	VirtualSize		dd ?
	VirtualAddress		dd ?
	SizeOfRawData		dd ?
	PointerToRawData	dd ?
	PointerToRelocations	dd ?
	PointerToLinenumbers	dd ?
	NumberOfRelocations	dw ?
	NumberOfLinenumbers	dw ?
	Characteristics 	dd ?
ends

struct IMAGE_RELOCATION, packed
	VirtualAddress		dd ?
	SymbolTableIndex	dd ?
	Type			dw ?
ends

struct IMAGE_SYMBOL_EX, packed
	ShortName		db 8 dup ?
	Value			dd ?
	SectionNumber		dd ?
	Type			dw ?
	StorageClass		db ?
	NumberOfAuxSymbols	db ?
ends

; ---------------------------------------------------------------------------
; staging records - the single source of truth POSTPONE emits from. Every
; instantiation passes every field, so no record byte is ever uninitialized.
; ---------------------------------------------------------------------------

struct SECTION_RECORD, packed
	shortname	dq ?	; zero-padded name, 0 if long
	name_pos	dd ?	; string table offset of long name (0 = short)
	flags		dd ?	; Characteristics
	raw_offset	dd ?	; PointerToRawData (0 if fully uninitialized)
	size		dd ?	; SizeOfRawData and aux Length
	reloc_1st	dd ?	; first relocation record
	reloc_count	dd ?
	selection	dd ?	; COMDAT selection
	association	dd ?	; COMDAT association (1-based section number)
	xm_init 	dd ?	; EXACT_MATCH initialized length
	xm_tail 	dd ?	; EXACT_MATCH zero-padded tail length
	xm_crc		dd ?	; EXACT_MATCH CRC-32 (filled in by POSTPONE)
ends

struct PUBLIC_RECORD, packed
	shortname	dq ?
	name_pos	dd ?
	value		dd ?
	secnum		dd ?	; 1-based; 0 undefined, -1 absolute
	class		dd ?
	aux		dd ?	; aux record count (1 for weak externals)
	tag		dd ?	; weak external: extern ordinal of the alias tag
ends

struct EXTERN_RECORD, packed
	shortname	dq ?
	name_pos	dd ?
ends

struct RELOC_RECORD, packed
	offset		dd ?	; offset in section
	target		dd ?	; section index (target_kind 0) or extern ordinal (1)
	target_kind	dd ?
	kind		dd ?	; semantic relocation kind, mapped per machine in POSTPONE
ends

IMAGE_FILE_MACHINE_I386 	 = 0x14C
IMAGE_FILE_MACHINE_AMD64	 = 0x8664

IMAGE_SCN_CNT_CODE		 = 0x00000020
IMAGE_SCN_CNT_INITIALIZED_DATA	 = 0x00000040
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_SCN_LNK_INFO		 = 0x00000200
IMAGE_SCN_LNK_REMOVE		 = 0x00000800
IMAGE_SCN_LNK_COMDAT		 = 0x00001000
IMAGE_SCN_ALIGN_1BYTES		 = 0x00100000
IMAGE_SCN_ALIGN_2BYTES		 = 0x00200000
IMAGE_SCN_ALIGN_4BYTES		 = 0x00300000
IMAGE_SCN_ALIGN_8BYTES		 = 0x00400000
IMAGE_SCN_ALIGN_16BYTES 	 = 0x00500000
IMAGE_SCN_ALIGN_32BYTES 	 = 0x00600000
IMAGE_SCN_ALIGN_64BYTES 	 = 0x00700000
IMAGE_SCN_ALIGN_128BYTES	 = 0x00800000
IMAGE_SCN_ALIGN_256BYTES	 = 0x00900000
IMAGE_SCN_ALIGN_512BYTES	 = 0x00A00000
IMAGE_SCN_ALIGN_1024BYTES	 = 0x00B00000
IMAGE_SCN_ALIGN_2048BYTES	 = 0x00C00000
IMAGE_SCN_ALIGN_4096BYTES	 = 0x00D00000
IMAGE_SCN_ALIGN_8192BYTES	 = 0x00E00000
IMAGE_SCN_LNK_NRELOC_OVFL	 = 0x01000000
IMAGE_SCN_MEM_DISCARDABLE	 = 0x02000000
IMAGE_SCN_MEM_NOT_PAGED 	 = 0x08000000
IMAGE_SCN_MEM_SHARED		 = 0x10000000
IMAGE_SCN_MEM_EXECUTE		 = 0x20000000
IMAGE_SCN_MEM_READ		 = 0x40000000
IMAGE_SCN_MEM_WRITE		 = 0x80000000

IMAGE_REL_AMD64_ADDR64	 = 0x0001
IMAGE_REL_AMD64_ADDR32	 = 0x0002
IMAGE_REL_AMD64_ADDR32NB = 0x0003
IMAGE_REL_AMD64_REL32	 = 0x0004
IMAGE_REL_AMD64_SECTION  = 0x000A
IMAGE_REL_AMD64_SECREL	 = 0x000B

IMAGE_REL_I386_DIR32	 = 0x0006
IMAGE_REL_I386_DIR32NB	 = 0x0007
IMAGE_REL_I386_SECTION	 = 0x000A
IMAGE_REL_I386_SECREL	 = 0x000B
IMAGE_REL_I386_REL32	 = 0x0014

IMAGE_SYM_UNDEFINED = 0
IMAGE_SYM_ABSOLUTE  = -1

IMAGE_SYM_CLASS_EXTERNAL      = 2
IMAGE_SYM_CLASS_STATIC	      = 3
IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105

IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY = 1
IMAGE_WEAK_EXTERN_SEARCH_LIBRARY   = 2
IMAGE_WEAK_EXTERN_SEARCH_ALIAS	   = 3

IMAGE_COMDAT_SELECT_NODUPLICATES = 1
IMAGE_COMDAT_SELECT_ANY 	 = 2
IMAGE_COMDAT_SELECT_SAME_SIZE	 = 3
IMAGE_COMDAT_SELECT_EXACT_MATCH  = 4
IMAGE_COMDAT_SELECT_ASSOCIATIVE  = 5
IMAGE_COMDAT_SELECT_LARGEST	 = 6
IMAGE_COMDAT_SELECT_NEWEST	 = 7	; deprecated: reproducible builds zero the timestamps it compares

NEWCOFF::

namespace NEWCOFF

	; semantic relocation kinds
	RELOC_ABS32 := 0	; absolute 32-bit address
	RELOC_RVA32 := 1	; 32-bit image-relative (RVA)
	RELOC_REL32 := 2	; 32-bit relative to the end of the field
	RELOC_ABS64 := 3	; absolute 64-bit address
	RELOC_RVA64 := 4	; 64-bit field holding an RVA
	RELOC_SECREL32 := 5	; 32-bit offset within the target's section
	RELOC_SECTION := 6	; 16-bit section number of the target

	; ANON_OBJECT_HEADER_BIGOBJ - always; classic COFF is legacy's job.
	; The machine is decided in POSTPONE from x86.mode.
	Header:

	Sig1			dw 0		; IMAGE_FILE_MACHINE_UNKNOWN
	Sig2			dw 0xFFFF
	Version 		dw 2
	Machine 		dw MACHINE
	TimeDateStamp		dd __TIME__
	ClassID:		dd 0xD1BAA1C7	; {D1BAA1C7-BAEE-4ba9-AF20-FAF66AA4DCB8}
				dw 0xBAEE, 0x4BA9
				db 0xAF, 0x20, 0xFA, 0xF6, 0x6A, 0xA4, 0xDC, 0xB8
	SizeOfData		dd 0
	Flags			dd 0
	MetaDataSize		dd 0
	MetaDataOffset		dd 0
	NumberOfSections	dd NUMBER_OF_SECTIONS
	PointerToSymbolTable	dd SYMBOL_TABLE_OFFSET
	NumberOfSymbols 	dd NUMBER_OF_SYMBOLS

	Sections:		db NUMBER_OF_SECTIONS * sizeof IMAGE_SECTION_HEADER dup 0

	; extendable staging blocks: appended by reopening (virtual name),
	; sized by what was appended - no forward-referenced reservations
	virtual at 0
		section_records::
	end virtual

	virtual at 0
		public_records::
	end virtual

	virtual at 0
		extern_records::
	end virtual

	virtual at 0
		reloc_records::
	end virtual

	virtual at 0
		string_table:: dd STRING_TABLE_SIZE
	end virtual

	; reflected CRC-32 nibble table (polynomial 0xEDB88320). The COMDAT
	; aux CheckSum used by IMAGE_COMDAT_SELECT_EXACT_MATCH is this CRC of
	; the section's raw data, seeded with 0 and with NO final inversion.
	virtual at 0
		crc32_table::
		dd 0x00000000,0x1DB71064,0x3B6E20C8,0x26D930AC
		dd 0x76DC4190,0x6B6B51F4,0x4DB26158,0x5005713C
		dd 0xEDB88320,0xF00F9344,0xD6D6A3E8,0xCB61B38C
		dd 0x9B64C2B0,0x86D3D2D4,0xA00AE278,0xBDBDF21C
	end virtual

	element relocatable?

	; the one relocation engine: emits the addend, then appends a record
	; with a SYMBOLIC target decoded from the element metadata. Section
	; elements carry scale (1+index), extern elements carry scale -1 with
	; the ordinal as the constant term. Positional initializers keep the
	; asm lines free of field-name tokens that CALM interpolation could
	; otherwise rewrite.
	calminstruction record? size*, value*, type*
		local	roffs, scale, rtgt, rknd
		compute roffs, $%
		emit	size, 0 scaleof value
		check	$% > roffs
		jno	done
		compute scale, 1 scaleof (1 metadataof value)
		check	scale > 0
		jyes	section_target
		compute rtgt, 0 scaleof (1 metadataof value)
		compute rknd, 1
		jump	have_target
	    section_target:
		compute rtgt, scale - 1
		compute rknd, 0
	    have_target:
		compute roffs, roffs - NEWCOFF.SECTION_OFFSET
		asm	virtual NEWCOFF.reloc_records
		asm	RELOC_RECORD roffs, rtgt, rknd, type
		asm	end virtual
	    done:
	end calminstruction

	; register a symbol or section name: short names travel in the record,
	; long ones are appended to the string table here and travel as offsets
	macro register_name src
		if lengthof src > 8
			virtual string_table
				NAME_POS = $
				db src, 0
			end virtual
			NAME_SHORT = 0
		else
			NAME_POS = 0
			NAME_SHORT = src
		end if
	end macro

	macro section_start
		virtual section_records
			SECTION_INDEX = ($ - $$) / sizeof SECTION_RECORD
		end virtual
		local sym
		element sym : relocatable * (1 + SECTION_INDEX)
		SECTION_BASE = sym
		org sym
		align.assume sym, SECTION_ALIGN
		SECTION_RECORDED = 0
		if DEFINED_SECTION | defined DEFAULT_SECTION
			SECTION_RECORDED = 1
			if (SECTION_FLAGS and IMAGE_SCN_LNK_COMDAT) & SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_EXACT_MATCH
				; anchor the section's ORG space so POSTPONE can read
				; the raw bytes back in place for the checksum
				local xm_area
				xm_area::
				repeat 1, si:SECTION_INDEX
					define exactmatch_list xm_area, sym, si
				end repeat
			end if
		end if
	end macro

	define exactmatch_list

	SECTION_RELOC_FIRST = 0
	SECTION_OFFSET = $%
	SECTION_ALIGN = 4
	SECTION_NAME = '.flat'
	SECTION_FLAGS = IMAGE_SCN_CNT_CODE + IMAGE_SCN_CNT_INITIALIZED_DATA + IMAGE_SCN_MEM_EXECUTE + IMAGE_SCN_MEM_READ + IMAGE_SCN_MEM_WRITE
	SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_NODUPLICATES
	SECTION_COMDAT_ASSOCIATION = 0
	DEFINED_SECTION = 0

	section_start

end namespace

macro section?
	namespace NEWCOFF

		SECTION_SIZE = $% - SECTION_OFFSET

		XM_INIT = 0
		XM_TAIL = 0
		if SECTION_RECORDED & (SECTION_FLAGS and IMAGE_SCN_LNK_COMDAT) & SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_EXACT_MATCH & $%% > SECTION_OFFSET
			XM_INIT = $%% - SECTION_OFFSET
			XM_TAIL = $% - $%%
		end if

		if DEFINED_SECTION | SECTION_SIZE > 0

			if ~ DEFINED_SECTION
				DEFAULT_SECTION := 1
			end if

			if SECTION_SIZE > 0 & $%% = SECTION_OFFSET
				SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_CNT_UNINITIALIZED_DATA
				SECTION_OFFSET = 0
				section $
			else
				UNINITIALIZED_LENGTH = $% - $%%
				section $
				db UNINITIALIZED_LENGTH dup 0
			end if

			register_name SECTION_NAME
			virtual reloc_records
				RELOC_TOTAL = ($ - $$) / sizeof RELOC_RECORD
			end virtual
			; values in structure order (names are for partial records);
			; qualified, because the struct engine re-arranges initializer
			; tokens, which then resolve inside the instance namespace
			virtual section_records
				SECTION_RECORD \
					NEWCOFF.NAME_SHORT,\
					NEWCOFF.NAME_POS,\
					NEWCOFF.SECTION_FLAGS,\
					NEWCOFF.SECTION_OFFSET,\
					NEWCOFF.SECTION_SIZE,\
					NEWCOFF.SECTION_RELOC_FIRST,\
					NEWCOFF.RELOC_TOTAL - NEWCOFF.SECTION_RELOC_FIRST,\
					NEWCOFF.SECTION_COMDAT_SELECTION,\
					NEWCOFF.SECTION_COMDAT_ASSOCIATION,\
					NEWCOFF.XM_INIT,\
					NEWCOFF.XM_TAIL,\
					0
			end virtual

		end if

	end namespace
end macro

macro section? declaration*
	namespace NEWCOFF

		section

		DEFINED_SECTION = 1
		SECTION_FLAGS = 0
		SECTION_OFFSET = $%
		SECTION_ALIGN = 4
		SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_NODUPLICATES
		SECTION_COMDAT_ASSOCIATION = 0

		match name attributes, declaration

			SECTION_NAME = name

			local seq,list
			match flags =align? boundary, attributes
				SECTION_ALIGN = boundary
				define seq flags
			else match =align? boundary, attributes
				SECTION_ALIGN = boundary
				define seq
			else
				define seq attributes
			end match
			match flags =comdat? =associative? target, seq
				SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_LNK_COMDAT
				SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_ASSOCIATIVE
				SECTION_COMDAT_ASSOCIATION = 1 scaleof (1 metadataof (target))
				define seq flags
			end match
			match =comdat? =associative? target, seq
				SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_LNK_COMDAT
				SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_ASSOCIATIVE
				SECTION_COMDAT_ASSOCIATION = 1 scaleof (1 metadataof (target))
				define seq
			end match
			while 1
				match car cdr, seq
					define list car
					define seq cdr
				else
					match any, seq
						define list any
					end match
					break
				end match
			end while
			irpv attribute, list
				match =code?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_CNT_CODE
				else match =data?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_CNT_INITIALIZED_DATA
				else match =readable?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_MEM_READ
				else match =writeable?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_MEM_WRITE
				else match =executable?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_MEM_EXECUTE
				else match =shareable?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_MEM_SHARED
				else match =discardable?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_MEM_DISCARDABLE
				else match =notpageable?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_MEM_NOT_PAGED
				else match =linkremove?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_LNK_REMOVE
				else match =linkinfo?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_LNK_INFO
				else match =comdat?, attribute
					SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_LNK_COMDAT
				else match =noduplicates?, attribute
					SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_NODUPLICATES
				else match =any?, attribute
					SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_ANY
				else match =samesize?, attribute
					SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_SAME_SIZE
				else match =largest?, attribute
					SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_LARGEST
				else match =exactmatch?, attribute
					SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_EXACT_MATCH
				else match =newest?, attribute
					SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_NEWEST
				else
					err 'unknown attribute "',`attribute,'"'
				end match
			end irpv
			if SECTION_COMDAT_SELECTION <> IMAGE_COMDAT_SELECT_NODUPLICATES & ~ (SECTION_FLAGS and IMAGE_SCN_LNK_COMDAT)
				err 'COMDAT selection attribute requires "comdat"'
			end if

		else

			SECTION_NAME = declaration

		end match

		repeat 1, i:SECTION_ALIGN
			if defined IMAGE_SCN_ALIGN_#i#BYTES
				SECTION_FLAGS = SECTION_FLAGS or IMAGE_SCN_ALIGN_#i#BYTES
			else
				err 'invalid section alignment'
			end if
		end repeat

		section_start

		virtual reloc_records
			SECTION_RELOC_FIRST = ($ - $$) / sizeof RELOC_RECORD
		end virtual

	end namespace
end macro

macro public? declaration*
	namespace NEWCOFF
		match =static? value =as? str, declaration
			SYMBOL_VALUE = value
			SYMBOL_NAME = string str
			SYMBOL_CLASS = IMAGE_SYM_CLASS_STATIC
		else match value =as? str, declaration
			SYMBOL_VALUE = value
			SYMBOL_NAME = string str
			SYMBOL_CLASS = IMAGE_SYM_CLASS_EXTERNAL
		else match =static? value, declaration
			SYMBOL_VALUE = value
			SYMBOL_NAME = `value
			SYMBOL_CLASS = IMAGE_SYM_CLASS_STATIC
		else
			SYMBOL_VALUE = declaration
			SYMBOL_NAME = `declaration
			SYMBOL_CLASS = IMAGE_SYM_CLASS_EXTERNAL
		end match
		SYMBOL_AUX = 0
		SYMBOL_TAG = 0
		if SYMBOL_VALUE relativeto 1 elementof SYMBOL_VALUE & 1 elementof (1 metadataof SYMBOL_VALUE) relativeto relocatable
			if 1 scaleof (1 metadataof SYMBOL_VALUE) > 0
				SYMBOL_SECTION_NUMBER = 1 scaleof (1 metadataof SYMBOL_VALUE)
				SYMBOL_VALUE = SYMBOL_VALUE - 1 elementof SYMBOL_VALUE
			else if SYMBOL_CLASS = IMAGE_SYM_CLASS_EXTERNAL
				; weak external: an alias whose default resolution is the
				; extern the value refers to; carries one aux record
				SYMBOL_TAG = 0 scaleof (1 metadataof SYMBOL_VALUE)
				SYMBOL_VALUE = 0
				SYMBOL_SECTION_NUMBER = IMAGE_SYM_UNDEFINED
				SYMBOL_CLASS = IMAGE_SYM_CLASS_WEAK_EXTERNAL
				SYMBOL_AUX = 1
			else
				SYMBOL_SECTION_NUMBER = IMAGE_SYM_UNDEFINED
				SYMBOL_VALUE = 0
			end if
		else
			SYMBOL_SECTION_NUMBER = IMAGE_SYM_ABSOLUTE
		end if
		register_name SYMBOL_NAME
		virtual public_records
			PUBLIC_RECORD NEWCOFF.NAME_SHORT, NEWCOFF.NAME_POS, NEWCOFF.SYMBOL_VALUE,\
				NEWCOFF.SYMBOL_SECTION_NUMBER, NEWCOFF.SYMBOL_CLASS,\
				NEWCOFF.SYMBOL_AUX, NEWCOFF.SYMBOL_TAG
		end virtual
	end namespace
end macro

macro extrn? declaration*
	namespace NEWCOFF
		virtual extern_records
			EXT_ORDINAL = ($ - $$) / sizeof EXTERN_RECORD
		end virtual
		local sym
		element sym : relocatable * (-1) + EXT_ORDINAL
		match str =as? name:size, declaration
			label name : size at sym
			SYMBOL_NAME = string str
		else match name:size, declaration
			label name : size at sym
			SYMBOL_NAME = `name
		else match str =as? name, declaration
			label name at sym
			SYMBOL_NAME = string str
		else
			label declaration at sym
			SYMBOL_NAME = `declaration
		end match
		register_name SYMBOL_NAME
		virtual extern_records
			EXTERN_RECORD NEWCOFF.NAME_SHORT, NEWCOFF.NAME_POS
		end virtual
	end namespace
end macro

; CodeView debug support: cvline/cvproc/cvframe/cvendp/cvlabel markers,
; the NEWCOFF.DEBUG per-line interceptor, and the POSTPONE synthesis of
; '.debug$S' and '.pdata'/'.xdata' sections - see newcoffcv.md
include 'format/newcoffcv.inc'

element NEWCOFF.IMAGE_BASE
RVA? equ -NEWCOFF.IMAGE_BASE+

calminstruction dword? value
	local	relsize, reltype
	compute value, value
	check	~ value relativeto 0 & value relativeto 1 elementof value & 1 elementof (1 metadataof value) relativeto NEWCOFF.relocatable
	jyes	addr32
	check	~ value relativeto 0 & (value + NEWCOFF.IMAGE_BASE) relativeto 1 elementof (value + NEWCOFF.IMAGE_BASE) & 1 elementof (1 metadataof (value + NEWCOFF.IMAGE_BASE)) relativeto NEWCOFF.relocatable
	jyes	addr32nb
	check	~ value relativeto 0 & (value + NEWCOFF.SECTION_BASE) relativeto 1 elementof (value + NEWCOFF.SECTION_BASE)
	jno	plain
	check	1 elementof (1 metadataof (value + NEWCOFF.SECTION_BASE)) relativeto NEWCOFF.relocatable
	jyes	rel32
    plain:
	emit	4, value
	exit
    addr32:
	compute relsize, 4
	compute reltype, NEWCOFF.RELOC_ABS32
	call	NEWCOFF.record, relsize, value, reltype
	exit
    addr32nb:
	compute value, value + NEWCOFF.IMAGE_BASE
	compute relsize, 4
	compute reltype, NEWCOFF.RELOC_RVA32
	call	NEWCOFF.record, relsize, value, reltype
	exit
    rel32:
	compute value, value + (NEWCOFF.SECTION_BASE + $% + 4 - NEWCOFF.SECTION_OFFSET)
	compute relsize, 4
	compute reltype, NEWCOFF.RELOC_REL32
	call	NEWCOFF.record, relsize, value, reltype
	exit
end calminstruction

calminstruction qword? value
	local	relsize, reltype
	compute value, value
	check	~ value relativeto 0 & value relativeto 1 elementof value & 1 elementof (1 metadataof value) relativeto NEWCOFF.relocatable
	jyes	addr64
	check	~ value relativeto 0 & (value + NEWCOFF.IMAGE_BASE) relativeto 1 elementof (value + NEWCOFF.IMAGE_BASE) & 1 elementof (1 metadataof (value + NEWCOFF.IMAGE_BASE)) relativeto NEWCOFF.relocatable
	jyes	addr64nb
    plain:
	emit	8, value
	exit
    addr64:
	compute relsize, 8
	compute reltype, NEWCOFF.RELOC_ABS64
	call	NEWCOFF.record, relsize, value, reltype
	exit
    addr64nb:
	; legal on i386 (DIR32NB patches the low dword); POSTPONE rejects it
	; for AMD64, where no 64-bit RVA relocation type exists (the legacy
	; backend references an imaginary IMAGE_REL_AMD64_ADDR64NB here)
	compute value, value + NEWCOFF.IMAGE_BASE
	compute relsize, 8
	compute reltype, NEWCOFF.RELOC_RVA64
	call	NEWCOFF.record, relsize, value, reltype
end calminstruction

postpone
	purge section?
	section
	namespace NEWCOFF

		; the machine is a property of the code assembled, not of the
		; FORMAT variant: read the mode in effect at the end of the source
		if x86.mode = 64
			MACHINE := IMAGE_FILE_MACHINE_AMD64
		else
			MACHINE := IMAGE_FILE_MACHINE_I386
		end if

		; CodeView + unwind synthesis (newcoffcv.inc): debug sections are
		; appended as ordinary records, before the counts are taken
		codeview_synthesize

		; counts fall out of the staging block lengths
		virtual section_records
			NUMBER_OF_SECTIONS := ($ - $$) / sizeof SECTION_RECORD
		end virtual
		virtual reloc_records
			NUMBER_OF_RELOCATIONS := ($ - $$) / sizeof RELOC_RECORD
		end virtual
		virtual extern_records
			NUMBER_OF_EXTERNS := ($ - $$) / sizeof EXTERN_RECORD
		end virtual
		virtual public_records
			PUB_COUNT = ($ - $$) / sizeof PUBLIC_RECORD
		end virtual

		; per-section facts derived from the immutable public records:
		;  +0 dd ordinal+1 of an offset-0 external public (0 = none)
		;  +4 dd section has a public (COMDAT leader exists)
		virtual at 0
			section_pub_lu:: db NUMBER_OF_SECTIONS * 8 dup 0
		end virtual
		repeat PUB_COUNT
			load P_SEC : 4 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.secnum
			if P_SEC > 0 & P_SEC <= NUMBER_OF_SECTIONS
				store 1 : 4 at section_pub_lu : (P_SEC-1) * 8 + 4
				load P_VAL : 4 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.value
				load P_CLS : 4 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.class
				if P_CLS = IMAGE_SYM_CLASS_EXTERNAL & P_VAL = 0
					store % : 4 at section_pub_lu : (P_SEC-1) * 8
				end if
			end if
		end repeat

		; a COMDAT section that never received a public gets a synthetic
		; static symbol named after the section (individually discardable
		; by /OPT:REF, but no cross-object identity - so selections other
		; than NODUPLICATES require a real public)
		repeat NUMBER_OF_SECTIONS
			load SEC_FLAGS : 4 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.flags
			if SEC_FLAGS and IMAGE_SCN_LNK_COMDAT
				load SEC_HASPUB : 4 from section_pub_lu : (%-1) * 8 + 4
				load SEC_SEL : 4 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.selection
				if SEC_HASPUB = 0 & SEC_SEL <> IMAGE_COMDAT_SELECT_ASSOCIATIVE
					; associative sections need no leader symbol
					if SEC_SEL <> IMAGE_COMDAT_SELECT_NODUPLICATES
						err 'COMDAT selection requires a PUBLIC symbol in the section'
					end if
					load S_NAME : 8 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.shortname
					load S_POS : 4 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.name_pos
					virtual public_records
						PUBLIC_RECORD NEWCOFF.S_NAME, NEWCOFF.S_POS, 0, %, IMAGE_SYM_CLASS_STATIC, 0, 0
					end virtual
				end if
			end if
		end repeat
		virtual public_records
			NUMBER_OF_PUBLIC_RECORDS := ($ - $$) / sizeof PUBLIC_RECORD
		end virtual

		; canonical symbol layout. Weak externals carry one aux record,
		; so per-public symbol indices come from one prefix scan
		PUB_BASE = NUMBER_OF_SECTIONS * 2
		virtual at 0
			pub_index_lu:: db NUMBER_OF_PUBLIC_RECORDS * 4 dup 0
		end virtual
		PUB_ENTRIES = 0
		repeat NUMBER_OF_PUBLIC_RECORDS
			store (PUB_BASE + PUB_ENTRIES) and 0xFFFFFFFF : 4 at pub_index_lu : (%-1) * 4
			load P_AUX : 4 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.aux
			PUB_ENTRIES = PUB_ENTRIES + 1 + P_AUX
		end repeat
		EXT_BASE = PUB_BASE + PUB_ENTRIES
		NUMBER_OF_SYMBOLS := EXT_BASE + NUMBER_OF_EXTERNS

		; EXACT_MATCH checksums: reflected CRC-32 (poly 0xEDB88320), seed 0,
		; no final inversion, over the raw bytes read back from the output
		; area in place, then the zero-padded uninitialized tail
		irpv xm_cell, exactmatch_list
			match xm_area =, xm_base =, xm_sec, xm_cell
				load XM_INIT : 4 from section_records : xm_sec * sizeof SECTION_RECORD + SECTION_RECORD.xm_init
				if XM_INIT > 0
					load XM_TAIL : 4 from section_records : xm_sec * sizeof SECTION_RECORD + SECTION_RECORD.xm_tail
					XM_CRC = 0
					repeat XM_INIT, i:0
						load XM_BYTE : 1 from xm_area : (xm_base + i)
						XM_CRC = XM_CRC xor XM_BYTE
						load XM_FOLD : 4 from crc32_table : (XM_CRC and 15) shl 2
						XM_CRC = (XM_CRC shr 4) xor XM_FOLD
						load XM_FOLD : 4 from crc32_table : (XM_CRC and 15) shl 2
						XM_CRC = (XM_CRC shr 4) xor XM_FOLD
					end repeat
					repeat XM_TAIL
						load XM_FOLD : 4 from crc32_table : (XM_CRC and 15) shl 2
						XM_CRC = (XM_CRC shr 4) xor XM_FOLD
						load XM_FOLD : 4 from crc32_table : (XM_CRC and 15) shl 2
						XM_CRC = (XM_CRC shr 4) xor XM_FOLD
					end repeat
					store XM_CRC and 0xFFFFFFFF : 4 at section_records : xm_sec * sizeof SECTION_RECORD + SECTION_RECORD.xm_crc
				end if
			end match
		end irpv

		; ---- relocations: resolve symbolic targets and kinds, emit ----
		; per section, so a count above 65535 can prepend the overflow
		; placeholder (IMAGE_SCN_LNK_NRELOC_OVFL: real count+1 in the
		; VirtualAddress of an extra first relocation)
		RELOCATIONS_OFFSET = $%
		repeat NUMBER_OF_SECTIONS
		    RSEC_I = % - 1
		    load RSEC_1ST : 4 from section_records : RSEC_I * sizeof SECTION_RECORD + SECTION_RECORD.reloc_1st
		    load RSEC_N : 4 from section_records : RSEC_I * sizeof SECTION_RECORD + SECTION_RECORD.reloc_count
		    if RSEC_N > 65535
			dd RSEC_N + 1, 0
			dw 0
		    end if
		    repeat RSEC_N
			R_I = RSEC_1ST + % - 1
			load R_OFF : 4 from reloc_records : R_I * sizeof RELOC_RECORD + RELOC_RECORD.offset
			load R_TGT : 4 from reloc_records : R_I * sizeof RELOC_RECORD + RELOC_RECORD.target
			load R_KIND : 4 from reloc_records : R_I * sizeof RELOC_RECORD + RELOC_RECORD.target_kind
			load R_RELOC : 4 from reloc_records : R_I * sizeof RELOC_RECORD + RELOC_RECORD.kind
			if R_KIND = 1
				R_SYM = EXT_BASE + R_TGT
			else
				; fold-safe: prefer the section's offset-0 external, so
				; a reference into a folded COMDAT rebinds to the survivor
				load R_REFPUB : 4 from section_pub_lu : R_TGT * 8
				if R_REFPUB > 0
					load R_SYM : 4 from pub_index_lu : (R_REFPUB - 1) * 4
				else
					R_SYM = R_TGT * 2
				end if
			end if
			; semantic kind -> machine relocation type
			if MACHINE = IMAGE_FILE_MACHINE_AMD64
				if R_RELOC = RELOC_ABS32
					R_TYPE = IMAGE_REL_AMD64_ADDR32
				else if R_RELOC = RELOC_RVA32
					R_TYPE = IMAGE_REL_AMD64_ADDR32NB
				else if R_RELOC = RELOC_REL32
					R_TYPE = IMAGE_REL_AMD64_REL32
				else if R_RELOC = RELOC_ABS64
					R_TYPE = IMAGE_REL_AMD64_ADDR64
				else if R_RELOC = RELOC_SECREL32
					R_TYPE = IMAGE_REL_AMD64_SECREL
				else if R_RELOC = RELOC_SECTION
					R_TYPE = IMAGE_REL_AMD64_SECTION
				else
					err 'AMD64 COFF has no 64-bit RVA relocation type'
					R_TYPE = 0
				end if
			else
				; a 64-bit field on i386 takes the 32-bit relocation
				; against its low dword, as the legacy backend does
				if R_RELOC = RELOC_ABS32 | R_RELOC = RELOC_ABS64
					R_TYPE = IMAGE_REL_I386_DIR32
				else if R_RELOC = RELOC_RVA32 | R_RELOC = RELOC_RVA64
					R_TYPE = IMAGE_REL_I386_DIR32NB
				else if R_RELOC = RELOC_SECREL32
					R_TYPE = IMAGE_REL_I386_SECREL
				else if R_RELOC = RELOC_SECTION
					R_TYPE = IMAGE_REL_I386_SECTION
				else
					R_TYPE = IMAGE_REL_I386_REL32
				end if
			end if
			dd R_OFF and 0xFFFFFFFF
			dd R_SYM and 0xFFFFFFFF
			dw R_TYPE and 0xFFFF
		    end repeat
		end repeat

		; ---- symbol table: sections, publics, synthetics, externs ----
		SYMBOL_TABLE_OFFSET = $%
		repeat NUMBER_OF_SECTIONS
			load SEC_NAMEPOS : 4 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.name_pos
			if SEC_NAMEPOS > 0
				dd 0, SEC_NAMEPOS
			else
				load SEC_NAME : 8 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.shortname
				dq SEC_NAME
			end if
			dd 0				; Value
			dd %				; SectionNumber (32-bit in EX)
			dw 0				; Type
			db IMAGE_SYM_CLASS_STATIC, 1	; one aux record always
			; aux: section definition (20 bytes; association is split
			; low/high around Selection, so it reaches all sections)
			load SEC_SIZE : 4 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.size
			load SEC_NRELOC : 4 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.reloc_count
			load SEC_FLAGS : 4 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.flags
			load SEC_CRC : 4 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.xm_crc
			dd SEC_SIZE
			if SEC_NRELOC > 65535
				dw 0xFFFF, 0
			else
				dw SEC_NRELOC and 0xFFFF, 0
			end if
			dd SEC_CRC
			SEC_ASSOC = 0
			SEC_SEL = 0
			if SEC_FLAGS and IMAGE_SCN_LNK_COMDAT
				load SEC_SEL : 4 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.selection
				if SEC_SEL = IMAGE_COMDAT_SELECT_ASSOCIATIVE
					load SEC_ASSOC : 4 from section_records : (%-1) * sizeof SECTION_RECORD + SECTION_RECORD.association
					if SEC_ASSOC < 1 | SEC_ASSOC > % - 1
						err 'associated section must precede the associative one'
						SEC_ASSOC = 0
					end if
				end if
			end if
			dw SEC_ASSOC and 0xFFFF		; Number, low part
			db SEC_SEL and 0xFF		; Selection
			db 0				; unused
			dw (SEC_ASSOC shr 16) and 0xFFFF ; Number, high part
			dw 0				; pad to 20 bytes
		end repeat
		repeat NUMBER_OF_PUBLIC_RECORDS
			load PUB_NAMEPOS : 4 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.name_pos
			if PUB_NAMEPOS > 0
				dd 0, PUB_NAMEPOS
			else
				load PUB_NAME : 8 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.shortname
				dq PUB_NAME
			end if
			load PUB_VALUE : 4 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.value
			load PUB_SECNUM : 4 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.secnum
			load PUB_CLASS : 4 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.class
			load PUB_AUX : 4 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.aux
			dd PUB_VALUE
			dd PUB_SECNUM and 0xFFFFFFFF	; 32-bit, signed (-1 absolute)
			dw 0
			db PUB_CLASS and 0xFF, PUB_AUX and 0xFF
			if PUB_AUX > 0
				; weak external aux: tag symbol + search characteristics
				load PUB_TAG : 4 from public_records : (%-1) * sizeof PUBLIC_RECORD + PUBLIC_RECORD.tag
				dd (EXT_BASE + PUB_TAG) and 0xFFFFFFFF
				dd IMAGE_WEAK_EXTERN_SEARCH_ALIAS
				dd 0, 0, 0		; pad to 20 bytes
			end if
		end repeat
		repeat NUMBER_OF_EXTERNS
			load EXT_NAMEPOS : 4 from extern_records : (%-1) * sizeof EXTERN_RECORD + EXTERN_RECORD.name_pos
			if EXT_NAMEPOS > 0
				dd 0, EXT_NAMEPOS
			else
				load EXT_NAME : 8 from extern_records : (%-1) * sizeof EXTERN_RECORD + EXTERN_RECORD.shortname
				dq EXT_NAME
			end if
			dd 0
			dd 0				; IMAGE_SYM_UNDEFINED
			dw 0
			db IMAGE_SYM_CLASS_EXTERNAL, 0
		end repeat

		virtual string_table
			STRING_TABLE_SIZE := $
		end virtual
		load byte_sequence : STRING_TABLE_SIZE from string_table : 0
		db byte_sequence

		; ---- section headers, into the block reserved at file start ---
		OVFL_BEFORE = 0
		repeat NUMBER_OF_SECTIONS
			SEC_I = % - 1
			load SEC_NAMEPOS : 4 from section_records : SEC_I * sizeof SECTION_RECORD + SECTION_RECORD.name_pos
			if SEC_NAMEPOS > 0
				if SEC_NAMEPOS > 9999999
					err 'string table too large to reference long section names (offset above /9999999)'
				end if
				repeat 1, pos:SEC_NAMEPOS
					store ('/' + (`pos shl 8)) : 8 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.Name
				end repeat
			else
				load SEC_NAME : 8 from section_records : SEC_I * sizeof SECTION_RECORD + SECTION_RECORD.shortname
				store SEC_NAME : 8 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.Name
			end if
			load SEC_FLAGS : 4 from section_records : SEC_I * sizeof SECTION_RECORD + SECTION_RECORD.flags
			load SEC_RAWOFF : 4 from section_records : SEC_I * sizeof SECTION_RECORD + SECTION_RECORD.raw_offset
			load SEC_SIZE : 4 from section_records : SEC_I * sizeof SECTION_RECORD + SECTION_RECORD.size
			load SEC_RELOC1 : 4 from section_records : SEC_I * sizeof SECTION_RECORD + SECTION_RECORD.reloc_1st
			load SEC_NRELOC : 4 from section_records : SEC_I * sizeof SECTION_RECORD + SECTION_RECORD.reloc_count
			store SEC_SIZE : 4 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.SizeOfRawData
			store SEC_RAWOFF : 4 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.PointerToRawData
			if SEC_NRELOC > 0
				store (RELOCATIONS_OFFSET + (SEC_RELOC1 + OVFL_BEFORE) * sizeof IMAGE_RELOCATION) and 0xFFFFFFFF : 4 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.PointerToRelocations
				if SEC_NRELOC > 65535
					SEC_FLAGS = SEC_FLAGS or IMAGE_SCN_LNK_NRELOC_OVFL
					store 0xFFFF : 2 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.NumberOfRelocations
					OVFL_BEFORE = OVFL_BEFORE + 1
				else
					store SEC_NRELOC and 0xFFFF : 2 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.NumberOfRelocations
				end if
			end if
			store SEC_FLAGS : 4 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.Characteristics
		end repeat

	end namespace
end postpone
