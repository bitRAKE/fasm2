; newcoffms.inc - ground-up MS64 COFF backend (development skeleton)
; ============================================================================
; A re-engineering of include/format/coffms.inc around three rules that the
; 2026 COMDAT retrofits taught us (see readme.md and the post-mortem in
; docs/coff_comdat_postmortem.md):
;
;   1. RECORDS, NOT STORES. During the pass, every entity (section, public,
;      extern, relocation) appends one fixed-size record to a staging
;      virtual. Nothing writes file images incrementally; the whole object
;      layout is emitted once, in POSTPONE, from the records.
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
;      (section number or extern ordinal), not a symbol table index. The
;      index is resolved in POSTPONE, in one place - which is also where a
;      section target is redirected to its offset-0 external symbol so that
;      references into folded COMDATs rebind to the survivor.
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
; POSTPONE reads x86.mode (use32 -> I386, use64 -> AMD64), and relocations
; record machine-independent SEMANTIC kinds (abs32/rva32/rel32/abs64/rva64)
; that POSTPONE maps to IMAGE_REL_* numbers for the detected machine.
; ============================================================================

macro struct? name
	macro end?.struct?!
			end namespace
		esc end struc
		virtual at 0
			name name
			sizeof.name = $
		end virtual
		purge end?.struct?
	end macro
	esc struc name
		label . : sizeof.name
		namespace .
end macro

struct IMAGE_SECTION_HEADER
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
end struct

struct IMAGE_RELOCATION
	VirtualAddress		dd ?
	SymbolTableIndex	dd ?
	Type			dw ?
end struct

struct IMAGE_SYMBOL_EX
	ShortName		db 8 dup ?
	Value			dd ?
	SectionNumber		dd ?
	Type			dw ?
	StorageClass		db ?
	NumberOfAuxSymbols	db ?
end struct

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

IMAGE_REL_I386_DIR32	 = 0x0006
IMAGE_REL_I386_DIR32NB	 = 0x0007
IMAGE_REL_I386_REL32	 = 0x0014

IMAGE_SYM_UNDEFINED = 0
IMAGE_SYM_ABSOLUTE  = -1

IMAGE_SYM_CLASS_EXTERNAL      = 2
IMAGE_SYM_CLASS_STATIC	      = 3
IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105

IMAGE_COMDAT_SELECT_NODUPLICATES = 1
IMAGE_COMDAT_SELECT_ANY 	 = 2
IMAGE_COMDAT_SELECT_SAME_SIZE	 = 3
IMAGE_COMDAT_SELECT_EXACT_MATCH  = 4
IMAGE_COMDAT_SELECT_ASSOCIATIVE  = 5
IMAGE_COMDAT_SELECT_LARGEST	 = 6
IMAGE_COMDAT_SELECT_NEWEST	 = 7	; deprecated: reproducible builds zero the timestamps it compares

NEWCOFF::

namespace NEWCOFF

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

	; ------------------------------------------------------------------
	; staging records - the single source of truth POSTPONE emits from
	; ------------------------------------------------------------------

	; per-section record, 64 bytes:
	;  +0  dq  short name (zero padded), 0 if long
	;  +8  dd  string table offset of long name (0 = short)
	;  +12 dd  Characteristics
	;  +16 dd  PointerToRawData ($% at open; 0 if fully uninitialized)
	;  +20 dd  section length (SizeOfRawData and aux Length)
	;  +24 dd  first relocation record
	;  +28 dd  relocation count
	;  +32 dd  COMDAT selection
	;  +36 dd  COMDAT association (1-based section number)
	;  +40 dd  offset-0 external public ordinal + 1 (0 = none) - fold-safe target
	;  +44 dd  section has a public (COMDAT leader exists)
	;  +48 dd  EXACT_MATCH initialized length
	;  +52 dd  EXACT_MATCH zero-padded tail length
	;  +56 dd  EXACT_MATCH CRC-32 (computed in POSTPONE)
	virtual at 0
		section_records:: rb NUMBER_OF_SECTIONS * 64
	end virtual

	; per-public record, 32 bytes:
	;  +0 dq short name, +8 dd long name pos, +12 dd value,
	;  +16 dd section number (1-based; 0 undefined, -1 absolute), +20 dd class
	virtual at 0
		public_records:: rb NUMBER_OF_PUBLIC_RECORDS * 32
	end virtual

	; per-extern record, 16 bytes: +0 dq short name, +8 dd long name pos
	virtual at 0
		extern_records:: rb NUMBER_OF_EXTERNS * 16
	end virtual

	; per-relocation record, 16 bytes:
	;  +0 dd offset in section, +4 dd target, +8 dd target kind, +12 dd
	;  semantic relocation kind (machine-independent; POSTPONE maps it to
	;  an IMAGE_REL_* number once x86.mode has decided the machine)
	; target kind 0: a 0-based section index (resolved to the section's
	;                offset-0 external if one exists, else the section symbol)
	; target kind 1: an extern ordinal
	virtual at 0
		reloc_records:: rb NUMBER_OF_RELOCATIONS * 16
	end virtual

	; semantic relocation kinds
	RELOC_ABS32 := 0	; absolute 32-bit address
	RELOC_RVA32 := 1	; 32-bit image-relative (RVA)
	RELOC_REL32 := 2	; 32-bit relative to the end of the field
	RELOC_ABS64 := 3	; absolute 64-bit address
	RELOC_RVA64 := 4	; 64-bit field holding an RVA

	virtual at 0
		string_table:: dd STRING_TABLE_SIZE
		STRING_POSITION = $
		rb STRING_TABLE_SIZE - $
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
	; the ordinal as the constant term.
	calminstruction record? size*, value*, type*
		local	offset, scale, target, kind, rec
		compute offset, $%
		emit	size, 0 scaleof value
		check	$% > offset
		jno	done
		compute scale, 1 scaleof (1 metadataof value)
		check	scale > 0
		jyes	section_target
		compute target, 0 scaleof (1 metadataof value)
		compute kind, 1
		jump	have_target
	    section_target:
		compute target, scale - 1
		compute kind, 0
	    have_target:
		compute offset, offset - NEWCOFF.SECTION_OFFSET
		compute rec, NEWCOFF.RELOC_COUNT * 16
		asm	store offset : 4 at NEWCOFF.reloc_records : rec
		asm	store target : 4 at NEWCOFF.reloc_records : rec + 4
		asm	store kind : 4 at NEWCOFF.reloc_records : rec + 8
		asm	store type : 4 at NEWCOFF.reloc_records : rec + 12
		compute NEWCOFF.RELOC_COUNT, NEWCOFF.RELOC_COUNT + 1
	    done:
	end calminstruction

	macro section_start
		local sym
		element sym : relocatable * (1 + SECTION_INDEX)
		SECTION_BASE = sym
		org sym
		align.assume sym, SECTION_ALIGN
		if lengthof SECTION_NAME > 8
			SECTION_NAME_POS = STRING_POSITION
			store SECTION_NAME : lengthof SECTION_NAME at string_table : STRING_POSITION
			STRING_POSITION = STRING_POSITION + lengthof SECTION_NAME + 1
		else
			SECTION_NAME_POS = 0
		end if
		SECTION_RECORDED = 0
		if DEFINED_SECTION | defined DEFAULT_SECTION
			SECTION_RECORDED = 1
			if SECTION_NAME_POS > 0
				store 0 : 8 at section_records : SECTION_INDEX * 64
				store SECTION_NAME_POS : 4 at section_records : SECTION_INDEX * 64 + 8
			else
				store SECTION_NAME : 8 at section_records : SECTION_INDEX * 64
				store 0 : 4 at section_records : SECTION_INDEX * 64 + 8
			end if
			store SECTION_COMDAT_SELECTION : 4 at section_records : SECTION_INDEX * 64 + 32
			store SECTION_COMDAT_ASSOCIATION : 4 at section_records : SECTION_INDEX * 64 + 36
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

	SECTION_INDEX = 0
	PUB_COUNT = 0
	EXT_COUNT = 0
	RELOC_COUNT = 0
	SECTION_RELOC_FIRST = 0
	define exactmatch_list

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

		if SECTION_RECORDED & (SECTION_FLAGS and IMAGE_SCN_LNK_COMDAT) & SECTION_COMDAT_SELECTION = IMAGE_COMDAT_SELECT_EXACT_MATCH & $%% > SECTION_OFFSET
			store $%% - SECTION_OFFSET : 4 at section_records : SECTION_INDEX * 64 + 48
			store $% - $%% : 4 at section_records : SECTION_INDEX * 64 + 52
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

			store SECTION_FLAGS : 4 at section_records : SECTION_INDEX * 64 + 12
			store SECTION_OFFSET : 4 at section_records : SECTION_INDEX * 64 + 16
			store SECTION_SIZE : 4 at section_records : SECTION_INDEX * 64 + 20
			store SECTION_RELOC_FIRST : 4 at section_records : SECTION_INDEX * 64 + 24
			store RELOC_COUNT - SECTION_RELOC_FIRST : 4 at section_records : SECTION_INDEX * 64 + 28

			SECTION_INDEX = SECTION_INDEX + 1

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

		SECTION_RELOC_FIRST = RELOC_COUNT

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
		if SYMBOL_VALUE relativeto 1 elementof SYMBOL_VALUE & 1 elementof (1 metadataof SYMBOL_VALUE) relativeto relocatable
			if 1 scaleof (1 metadataof SYMBOL_VALUE) > 0
				SYMBOL_SECTION_NUMBER = 1 scaleof (1 metadataof SYMBOL_VALUE)
				SYMBOL_VALUE = SYMBOL_VALUE - 1 elementof SYMBOL_VALUE
				store 1 : 4 at section_records : (SYMBOL_SECTION_NUMBER-1) * 64 + 44
				if SYMBOL_CLASS = IMAGE_SYM_CLASS_EXTERNAL & SYMBOL_VALUE = 0
					store PUB_COUNT + 1 : 4 at section_records : (SYMBOL_SECTION_NUMBER-1) * 64 + 40
				end if
			else if SYMBOL_CLASS = IMAGE_SYM_CLASS_EXTERNAL
				err 'NEWCOFF: weak externals not implemented yet (readme.md roadmap)'
			else
				SYMBOL_SECTION_NUMBER = IMAGE_SYM_UNDEFINED
				SYMBOL_VALUE = 0
			end if
		else
			SYMBOL_SECTION_NUMBER = IMAGE_SYM_ABSOLUTE
		end if
		if lengthof SYMBOL_NAME > 8
			store 0 : 8 at public_records : PUB_COUNT * 32
			store STRING_POSITION : 4 at public_records : PUB_COUNT * 32 + 8
			store SYMBOL_NAME : lengthof SYMBOL_NAME at string_table : STRING_POSITION
			STRING_POSITION = STRING_POSITION + lengthof SYMBOL_NAME + 1
		else
			store SYMBOL_NAME : 8 at public_records : PUB_COUNT * 32
			store 0 : 4 at public_records : PUB_COUNT * 32 + 8
		end if
		store SYMBOL_VALUE : 4 at public_records : PUB_COUNT * 32 + 12
		store SYMBOL_SECTION_NUMBER : 4 at public_records : PUB_COUNT * 32 + 16
		store SYMBOL_CLASS : 4 at public_records : PUB_COUNT * 32 + 20
		PUB_COUNT = PUB_COUNT + 1
	end namespace
end macro

macro extrn? declaration*
	namespace NEWCOFF
		local sym
		element sym : relocatable * (-1) + EXT_COUNT
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
		if lengthof SYMBOL_NAME > 8
			store 0 : 8 at extern_records : EXT_COUNT * 16
			store STRING_POSITION : 4 at extern_records : EXT_COUNT * 16 + 8
			store SYMBOL_NAME : lengthof SYMBOL_NAME at string_table : STRING_POSITION
			STRING_POSITION = STRING_POSITION + lengthof SYMBOL_NAME + 1
		else
			store SYMBOL_NAME : 8 at extern_records : EXT_COUNT * 16
			store 0 : 4 at extern_records : EXT_COUNT * 16 + 8
		end if
		EXT_COUNT = EXT_COUNT + 1
	end namespace
end macro

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

		NUMBER_OF_SECTIONS := SECTION_INDEX
		NUMBER_OF_RELOCATIONS := RELOC_COUNT
		NUMBER_OF_EXTERNS := EXT_COUNT
		STRING_TABLE_SIZE := STRING_POSITION

		; a COMDAT section that never received a public gets a synthetic
		; static symbol named after the section (individually discardable
		; by /OPT:REF, but no cross-object identity - so selections other
		; than NODUPLICATES require a real public)
		SYNTH_COUNT = 0
		repeat NUMBER_OF_SECTIONS
			load SEC_FLAGS : 4 from section_records : (%-1) * 64 + 12
			if SEC_FLAGS and IMAGE_SCN_LNK_COMDAT
				load SEC_HASPUB : 4 from section_records : (%-1) * 64 + 44
				if SEC_HASPUB = 0
					load SEC_SEL : 4 from section_records : (%-1) * 64 + 32
					if SEC_SEL <> IMAGE_COMDAT_SELECT_NODUPLICATES
						err 'COMDAT selection requires a PUBLIC symbol in the section'
					end if
					load SEC_NAME : 8 from section_records : (%-1) * 64
					load SEC_NAMEPOS : 4 from section_records : (%-1) * 64 + 8
					store SEC_NAME : 8 at public_records : (PUB_COUNT + SYNTH_COUNT) * 32
					store SEC_NAMEPOS : 4 at public_records : (PUB_COUNT + SYNTH_COUNT) * 32 + 8
					store 0 : 4 at public_records : (PUB_COUNT + SYNTH_COUNT) * 32 + 12
					store % : 4 at public_records : (PUB_COUNT + SYNTH_COUNT) * 32 + 16
					store IMAGE_SYM_CLASS_STATIC : 4 at public_records : (PUB_COUNT + SYNTH_COUNT) * 32 + 20
					SYNTH_COUNT = SYNTH_COUNT + 1
				end if
			end if
		end repeat
		NUMBER_OF_PUBLIC_RECORDS := PUB_COUNT + SYNTH_COUNT

		; canonical symbol layout - indices are pure arithmetic
		PUB_BASE = NUMBER_OF_SECTIONS * 2
		EXT_BASE = PUB_BASE + NUMBER_OF_PUBLIC_RECORDS
		NUMBER_OF_SYMBOLS := EXT_BASE + NUMBER_OF_EXTERNS

		; EXACT_MATCH checksums: reflected CRC-32 (poly 0xEDB88320), seed 0,
		; no final inversion, over the raw bytes read back from the output
		; area in place, then the zero-padded uninitialized tail
		irpv xm_cell, exactmatch_list
			match xm_area =, xm_base =, xm_sec, xm_cell
				load XM_INIT : 4 from section_records : xm_sec * 64 + 48
				if XM_INIT > 0
					load XM_TAIL : 4 from section_records : xm_sec * 64 + 52
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
					store XM_CRC and 0xFFFFFFFF : 4 at section_records : xm_sec * 64 + 56
				end if
			end match
		end irpv

		; ---- relocations: resolve symbolic targets and kinds, emit ----
		RELOCATIONS_OFFSET = $%
		repeat NUMBER_OF_RELOCATIONS
			load R_OFF : 4 from reloc_records : (%-1) * 16
			load R_TGT : 4 from reloc_records : (%-1) * 16 + 4
			load R_KIND : 4 from reloc_records : (%-1) * 16 + 8
			load R_RELOC : 4 from reloc_records : (%-1) * 16 + 12
			if R_KIND = 1
				R_SYM = EXT_BASE + R_TGT
			else
				; fold-safe: prefer the section's offset-0 external, so
				; a reference into a folded COMDAT rebinds to the survivor
				load R_REFPUB : 4 from section_records : R_TGT * 64 + 40
				if R_REFPUB > 0
					R_SYM = PUB_BASE + R_REFPUB - 1
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
				else
					R_TYPE = IMAGE_REL_I386_REL32
				end if
			end if
			dd R_OFF and 0xFFFFFFFF
			dd R_SYM and 0xFFFFFFFF
			dw R_TYPE and 0xFFFF
		end repeat

		; ---- symbol table: sections, publics, synthetics, externs ----
		SYMBOL_TABLE_OFFSET = $%
		repeat NUMBER_OF_SECTIONS
			load SEC_NAMEPOS : 4 from section_records : (%-1) * 64 + 8
			if SEC_NAMEPOS > 0
				dd 0, SEC_NAMEPOS
			else
				load SEC_NAME : 8 from section_records : (%-1) * 64
				dq SEC_NAME
			end if
			dd 0				; Value
			dd %				; SectionNumber (32-bit in EX)
			dw 0				; Type
			db IMAGE_SYM_CLASS_STATIC, 1	; one aux record always
			; aux: section definition (20 bytes; association is split
			; low/high around Selection, so it reaches all sections)
			load SEC_SIZE : 4 from section_records : (%-1) * 64 + 20
			load SEC_NRELOC : 4 from section_records : (%-1) * 64 + 28
			load SEC_FLAGS : 4 from section_records : (%-1) * 64 + 12
			load SEC_CRC : 4 from section_records : (%-1) * 64 + 56
			dd SEC_SIZE
			dw SEC_NRELOC and 0xFFFF, 0
			dd SEC_CRC
			SEC_ASSOC = 0
			SEC_SEL = 0
			if SEC_FLAGS and IMAGE_SCN_LNK_COMDAT
				load SEC_SEL : 4 from section_records : (%-1) * 64 + 32
				if SEC_SEL = IMAGE_COMDAT_SELECT_ASSOCIATIVE
					load SEC_ASSOC : 4 from section_records : (%-1) * 64 + 36
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
			load PUB_NAMEPOS : 4 from public_records : (%-1) * 32 + 8
			if PUB_NAMEPOS > 0
				dd 0, PUB_NAMEPOS
			else
				load PUB_NAME : 8 from public_records : (%-1) * 32
				dq PUB_NAME
			end if
			load PUB_VALUE : 4 from public_records : (%-1) * 32 + 12
			load PUB_SECNUM : 4 from public_records : (%-1) * 32 + 16
			load PUB_CLASS : 4 from public_records : (%-1) * 32 + 20
			dd PUB_VALUE
			dd PUB_SECNUM and 0xFFFFFFFF	; 32-bit, signed (-1 absolute)
			dw 0
			db PUB_CLASS and 0xFF, 0
		end repeat
		repeat NUMBER_OF_EXTERNS
			load EXT_NAMEPOS : 4 from extern_records : (%-1) * 16 + 8
			if EXT_NAMEPOS > 0
				dd 0, EXT_NAMEPOS
			else
				load EXT_NAME : 8 from extern_records : (%-1) * 16
				dq EXT_NAME
			end if
			dd 0
			dd 0				; IMAGE_SYM_UNDEFINED
			dw 0
			db IMAGE_SYM_CLASS_EXTERNAL, 0
		end repeat

		load byte_sequence : STRING_TABLE_SIZE from string_table : 0
		db byte_sequence

		; ---- section headers, into the block reserved at file start ---
		repeat NUMBER_OF_SECTIONS
			SEC_I = % - 1
			load SEC_NAMEPOS : 4 from section_records : SEC_I * 64 + 8
			if SEC_NAMEPOS > 0
				if SEC_NAMEPOS > 9999999
					err 'string table too large to reference long section names (offset above /9999999)'
				end if
				repeat 1, pos:SEC_NAMEPOS
					store ('/' + (`pos shl 8)) : 8 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.Name
				end repeat
			else
				load SEC_NAME : 8 from section_records : SEC_I * 64
				store SEC_NAME : 8 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.Name
			end if
			load SEC_FLAGS : 4 from section_records : SEC_I * 64 + 12
			load SEC_RAWOFF : 4 from section_records : SEC_I * 64 + 16
			load SEC_SIZE : 4 from section_records : SEC_I * 64 + 20
			load SEC_RELOC1 : 4 from section_records : SEC_I * 64 + 24
			load SEC_NRELOC : 4 from section_records : SEC_I * 64 + 28
			store SEC_SIZE : 4 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.SizeOfRawData
			store SEC_RAWOFF : 4 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.PointerToRawData
			if SEC_NRELOC > 0
				if SEC_NRELOC > 65535
					err 'NEWCOFF: IMAGE_SCN_LNK_NRELOC_OVFL not implemented yet (readme.md roadmap)'
				end if
				store (RELOCATIONS_OFFSET + SEC_RELOC1 * sizeof IMAGE_RELOCATION) and 0xFFFFFFFF : 4 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.PointerToRelocations
				store SEC_NRELOC and 0xFFFF : 2 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.NumberOfRelocations
			end if
			store SEC_FLAGS : 4 at NEWCOFF:Sections + SEC_I * sizeof IMAGE_SECTION_HEADER + IMAGE_SECTION_HEADER.Characteristics
		end repeat

	end namespace
end postpone
