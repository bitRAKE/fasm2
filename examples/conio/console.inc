format MS64 COFF
include 'win64a.inc'
include 'wincon.g' ; console interface support

include 'data_blocks.inc' ; globwstr, globstr, ≡ ∵ □

; We might desire a terse method to write constant strings to the console.
;	<< 'Hello, World!' >>
calminstruction ? line&
	local var,i
	match << line >>, line
	jno done
	init i, 0
	; unique local label
	arrange var, .=const.i
	compute i, i + 1
	arrange line, var =GLOBSTR line
	assemble line
	arrange line, ::=WriteFile [.=hOutput], =addr var, =sizeof var, 0, 0
done:	assemble line
end calminstruction

; Better yet, a terse method to copy string fragments to RDI, the working
; buffer, seems more useful! GLOBSTR handles de-duplication.
calminstruction ? line&
	local var,i
	match <| line |>, line
	jno done
	init i, 0
	; TODO: special case for short strings?
	; unique local label
	arrange var, .=buildup.i
	compute i, i + 1
	arrange line, var =GLOBSTR line
	assemble line
	arrange line, =lea =rsi, [var]
	assemble line
	arrange line, =mov =ecx, =sizeof var
	assemble line
	arrange line, =rep =movsb
done:	assemble line
end calminstruction


; Using a compact syntax external libraries can be referenced in a way that both includes the library and specifies the function as external. The LIBRARY file name (without the extension) is limited to the symbol naming within fasmg.
;	:LIBRARY:FUNCTION [PARAMETERS,...]
;
; The MS linker doesn't have a concept that connects an external symbol to a specific library - the first library in the search order matching the symbol halts the search. Given that the same functions are present and linked differently for different execution environments - it's often desireable to NOT specify the libraries in the object files (i.e. select the target environment at link-time).
;	::FUNCTION [PARAMETERS,...]

; Note: If feature is unused then nothing is output.

; Use the COFF namespace to track libraries and functions.
namespace COFF
	define LIBRARIES LIBRARIES
	define FUNCTIONS FUNCTIONS
end namespace

calminstruction ?? line&
	local var, library, function, parameters
	match : library : function parameters?, line
	jyes lib_filter
	match :: function parameters?, line
	jyes fun_filter
	assemble line
	exit
lib_filter:
	; only one instance per library
	transform library, COFF.LIBRARIES
	jyes fun_filter
	; create a case-insensative reference
	arrange var, =COFF.=LIBRARIES.library?
	; we double up to prevent forward referencing (infinite loop)
	publish var, library
	publish var, library
	; gather unique vector of uses
	take COFF..libraries, library
fun_filter:
	; only one instance per function
	transform function, COFF.FUNCTIONS
	jyes pprocess
	; create a case-sensative reference
	arrange var, =COFF.=FUNCTIONS.function
	; we double up to prevent forward referencing (infinite loop)
	publish var, function
	publish var, function
	; gather unique vector of uses, preserve function
	transform var
	take COFF..functions, var
pprocess:
	arrange line, =fastcall [function],parameters
	assemble line
end calminstruction


postpone ; communicate with the linker

	; TODO: Specify size based on compiler mode, to allow mixing 32-bit and 64-bit code.
	irpv fun, COFF..functions
		extrn '__imp_' bappend `fun as fun: QWORD
	end irpv

;====================================================== Commands for the Linker:

virtual as 'response'			; Configure linker from here:
	db '/NOLOGO',10			; don't show linker version header

; Use to debug build process:
;	db '/VERBOSE',10
;	db '/TIME+',10

; Create unique binary using image version and checksum:
	db '/RELEASE',10		; set program checksum in header
repeat 1,T:__TIME__ shr 16,t:__TIME__ and 0xFFFF
	db '/VERSION:',`T,'.',`t,10	; time based binary version
end repeat

;	db '/ENTRY:WinMain',10		; or use default

	db '/IGNORE:4281',10		; ASLR doesn't happen for /FIXED!
	db '/FIXED',10			; don't generate relocation information
;	db '/DYNAMICBASE:NO',10		; same as /FIXED
	db '/BASE:0x7FFF0000',10	; above KUSER_SHARED_DATA
	db '/SUBSYSTEM:CONSOLE,6.02',10

	db '/NOCOFFGRPINFO',10		; no debug info, UNDOCUMENTED

;	db '/MERGE:.rdata=.text',10	; reduce executable size

; Load DLLs into 32-bit address space (though ntdll still high!):
;	db '/LARGEADDRESSAWARE:NO',10

; Default is 1MB reserve for each:
	db '/HEAP:0,0',10		; allocation granularity sufficient
	db '/STACK:0,0',10		; allocation granularity sufficient

	db '/HIGHENTROPYVA:NO',10	;
	db '/GUARD:NO',10		; requires /DYNAMICBASE
	db '/CETCOMPAT:NO',10		;
;	db '/DELAYLOAD:kernel32.dll',10	;
;	db '/DEPENDENTLOADFLAG:0x800',10; LOAD_LIBRARY_SEARCH_SYSTEM32

; Ask for libraries based on usage:
	db '/NODEFAULTLIB',10
	irpv lib, COFF..libraries
		db `lib,'.lib',10
	end irpv

; The first object file defines the EXE name, unless /OUT: is used:
	__BASE__ = __SOURCE__ bswap lengthof __SOURCE__
	while '.' <> __BASE__ and 0xFF
		__BASE__ = __BASE__ shr 8
	end while
	__BASE__ = __BASE__ bswap lengthof __BASE__
	db __BASE__,'obj',10
end virtual
end postpone


section '.text' code readable executable align 64
