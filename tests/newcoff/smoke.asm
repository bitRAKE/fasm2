; smoke.asm - NEWCOFF backend smoke test
; Assembled twice by _build.cmd:
;   fasm2 smoke.asm smoke_new.obj                     -> new backend
;   fasm2 -i"SMOKE_LEGACY=1" smoke.asm smoke_legacy.obj -> legacy, via forwarding
; Both link (with /OPT:REF) and exit with code 97 ('a').
include 'newcoff.inc'

if definite SMOKE_LEGACY
	format MS64 COFF	; forwarded to the legacy handler
else
	format MS64 NEWCOFF	; intercepted: ground-up backend

	; NEWCOFF lays the symbol table out canonically in POSTPONE, so a
	; PUBLIC may appear anywhere - even before its section. The legacy
	; backend cannot accept this (the linker requires the section symbol
	; to be the first symbol carrying its section number, MSVC LNK1143).
	public mainCRTStartup
	public tab
end if

extrn '__imp_ExitProcess' as ExitProcess:qword

section '.rdata$tab' data readable comdat exactmatch align 16
if definite SMOKE_LEGACY
	public tab
end if
tab db '0123456789abcdef'

section '.data' data readable writeable
	tabptr dq tab			; ADDR64 -> external 'tab' (fold-safe)

section '.bss' readable writeable align 16
	scratch dq ?			; uninitialized: no file space
	rb 56

section '.text$start' code readable executable comdat align 16
if definite SMOKE_LEGACY
	public mainCRTStartup
end if
mainCRTStartup:
	sub	rsp, 40
	mov	rax, [tabptr]		; REL32 -> '.data' section symbol
	movzx	ecx, byte [rax + 10]	; 'a' = 61h = 97
	lea	rdx, [tab]		; REL32 -> external 'tab'
	cmp	byte [rdx + 10], cl
	je	.ok
	mov	ecx, 1			; mismatch: fail loudly
    .ok:
	mov	[scratch], rcx		; REL32 -> '.bss' section symbol
	mov	rcx, [scratch]
	call	[ExitProcess]		; REL32 -> extern
