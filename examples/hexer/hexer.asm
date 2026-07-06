; hexer.asm  ->  hexer.obj
; ============================================================================
; Author: Rickey Bowers Jr. (bitRAKE); co-developed with Claude (Anthropic).
;
; Console front end: read a file named on the command line, write its bytes as
; hexadecimal to stdout. The program uses exactly one external symbol,
; u8_as_hex; a command-line string chooses what it resolves to:
;
;   -i"HEXER_ISA='base'"     u8_as_hex := u8_as_hex_base
;   -i"HEXER_ISA='avx2'"     u8_as_hex := u8_as_hex_avx2
;   -i"HEXER_ISA='avx512'"   u8_as_hex := u8_as_hex_avx512
;   (undefined)              dispatch build: u8_as_hex re-routes on first use
;
; A single-ISA build references exactly one variant, so /OPT:REF discards the
; rest. The dispatch build pulls in dispatch.obj, which references them all.
; ============================================================================
include 'windows.inc'

if ~ definite HEXER_ISA
	extrn u8_as_hex	; use runtime dispatcher
else if "" eqtype HEXER_ISA ; use selected ISA
	extrn "u8_as_hex_" bappend HEXER_ISA as u8_as_hex
else
	err 'HEXER_ISA string expected: base, avx2, avx512'
end if

extrn '__imp_GetStdHandle' as GetStdHandle:qword
extrn '__imp_GetCommandLineA' as GetCommandLineA:qword
extrn '__imp_CreateFileA' as CreateFileA:qword
extrn '__imp_ReadFile' as ReadFile:qword
extrn '__imp_WriteFile' as WriteFile:qword
extrn '__imp_ExitProcess' as ExitProcess:qword

STD_OUTPUT_HANDLE	= -11
STD_ERROR_HANDLE	= -12
GENERIC_READ		= 0x80000000
FILE_SHARE_READ		= 0x00000001
OPEN_EXISTING		= 3
INVALID_HANDLE_VALUE	= -1

CHUNK			= 0x10000		; bytes read per pass

; No 'data' attribute: the content is entirely reserved (rb / dq ?), so the
; format flags this section uninitialized (.bss) - it occupies no file space.
section '.bss' readable writeable align 16
	hStdOut		dq ?
	hFile		dq ?
	nread		dq ?
	inbuf		rb CHUNK
	hexbuf		rb CHUNK * 2

section '.data' data readable writeable
	usage		db 'usage: hexer <file>',13,10
	usage.len = $ - usage

section '.text' code readable executable

public mainCRTStartup
proc mainCRTStartup
	invoke	GetStdHandle, STD_OUTPUT_HANDLE
	mov	[hStdOut], rax

	; ---- locate argv[1] in the raw command line -----------------------
	invoke	GetCommandLineA
	mov	rdx, rax			; scan pointer
	; skip argv[0] (may be quoted)
	movzx	ecx, byte [rdx]
	cmp	cl, '"'
	jne	.skip_bare
	inc	rdx
    .skip_quote:
	movzx	ecx, byte [rdx]
	inc	rdx
	test	cl, cl
	jz	.no_arg
	cmp	cl, '"'
	jne	.skip_quote
	jmp	.skip_spaces
    .skip_bare:
	movzx	ecx, byte [rdx]
	inc	rdx
	test	cl, cl
	jz	.no_arg
	cmp	cl, ' '
	jne	.skip_bare
    .skip_spaces:
	movzx	ecx, byte [rdx]
	cmp	cl, ' '
	jne	.have_arg
	inc	rdx
	jmp	.skip_spaces
    .have_arg:
	test	cl, cl
	jz	.no_arg
	mov	rsi, rdx			; argv[1]

	; ---- open the file ------------------------------------------------
	invoke	CreateFileA, rsi, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0
	cmp	rax, INVALID_HANDLE_VALUE
	je	.no_arg
	mov	[hFile], rax

	; ---- stream: read a chunk, hex it, write 2x --------------------
    .chunk:
	invoke	ReadFile, [hFile], addr inbuf, CHUNK, addr nread, 0
	mov	r8, [nread]
	test	r8, r8
	jz	.eof
	fastcall u8_as_hex, addr hexbuf, addr inbuf, r8
	; rax = hexbuf + 2*nread
	mov	r8, [nread]
	shl	r8, 1				; hex output is 2 bytes per input byte
	invoke	WriteFile, [hStdOut], addr hexbuf, r8, 0, 0
	jmp	.chunk
    .eof:
	invoke	ExitProcess, 0

    .no_arg:
	invoke	GetStdHandle, STD_ERROR_HANDLE
	invoke	WriteFile, rax, addr usage, usage.len, 0, 0
	invoke	ExitProcess, 1
endp
