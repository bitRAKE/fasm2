; smoke32.asm - NEWCOFF backend, i386 flavour (machine detected from use32)
; Assembled twice by _build.cmd:
;   fasm2 smoke32.asm smoke32_new.obj                       -> new backend
;   fasm2 -i"SMOKE_LEGACY=1" smoke32.asm smoke32_legacy.obj -> legacy MS COFF
; Linked with no libraries at all (returning from the entry point exits the
; process with eax), so the 32-bit test needs no x86 import libraries on
; hand. Both must exit 97 ('a').

if definite SMOKE_LEGACY
	format MS COFF		; forwarded to the legacy handler
else
	format MS NEWCOFF	; intercepted: ground-up backend, i386 via use32

	; out-of-order publics: legal in NEWCOFF by construction
	public start32 as '_start32'
	public tab
end if

if definite SMOKE_LEGACY
	; upstream coffms: no COMDAT, and section names cap at 8 chars
	section '.rdata$t' data readable align 16
	public tab
else
	section '.rdata$tab' data readable comdat exactmatch align 16
end if
tab db '0123456789abcdef'

section '.data' data readable writeable
	tabptr	dd tab			; DIR32 -> external 'tab' (fold-safe)
	tabq	dq tab			; 64-bit field: DIR32 on the low dword

if definite SMOKE_LEGACY
	section '.text$s' code readable executable align 16
	public start32 as '_start32'
else
	section '.text$start' code readable executable comdat align 16
end if
start32:
	mov	eax, [tabptr]		; DIR32 -> '.data' section symbol
	movzx	eax, byte [eax + 10]	; 'a' = 61h = 97
	mov	edx, dword [tabq]	; same table through the 64-bit slot
	cmp	byte [edx + 10], al	; DIR32 -> external 'tab'
	je	.ok
	mov	eax, 1
    .ok:
	ret				; exit code = eax
