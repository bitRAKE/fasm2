; dispatch.asm  ->  dispatch.obj
; ============================================================================
; Author: Rickey Bowers Jr. (bitRAKE); co-developed with Claude (Anthropic).
;
; Runtime ISA detection and dispatch, kept in its own object so the single-ISA
; builds never pull it in. Because this object references every variant (to
; choose among them), linking it keeps them all; a single-ISA build omits it
; and /OPT:REF discards the variants the chosen one does not reference.
;
;   u8_as_hex_init()   detect CPU, point u8_as_hex_impl at the best variant
;   u8_as_hex(...)      tail-call through u8_as_hex_impl (same prototype)
; ============================================================================

include 'windows.inc'

extrn u8_as_hex_base
extrn u8_as_hex_avx2
extrn u8_as_hex_avx512


section '.data' data readable writeable align 8
	u8_as_hex_impl dq u8_as_hex_init


section '.text$dispatch' code readable executable align 16

; First time through dispatcher configures for future runs!
public u8_as_hex
u8_as_hex:
cvlabel u8_as_hex
	jmp	[u8_as_hex_impl]		; drop-in tail dispatch


; Pick the widest supported variant once. Checks both the CPUID feature bits
; and the XCR0 bits so a variant is used only when the OS also saves its state.

; Needs the same proto as dispatch functions:
;	u8_as_hex(rcx=dst, rdx=src, r8=len) -> rax = dst + 2*len
proc u8_as_hex_init uses rbx,dst,src,len
	mov	[dst], rcx
	mov	[src], rdx
	lea	r9, [u8_as_hex_base]

	mov	eax, 1
	cpuid
	and	ecx, (1 shl 27) or (1 shl 28)	; OSXSAVE and AVX
	cmp	ecx, (1 shl 27) or (1 shl 28)
	jne	.store
	xor	ecx, ecx
	xgetbv					; edx:eax = XCR0
	and	eax, 6				; SSE and AVX state saved?
	cmp	eax, 6
	jne	.store
	mov	eax, 7
	xor	ecx, ecx
	cpuid
	test	ebx, 1 shl 5			; AVX2
	jz	.store
	lea	r9, [u8_as_hex_avx2]

	test	ebx, 1 shl 16			; AVX-512 Foundation
	jz	.store
	xor	ecx, ecx
	xgetbv
	and	eax, 0xE0			; opmask + ZMM_Hi256 + Hi16_ZMM
	cmp	eax, 0xE0
	jne	.store
	lea	r9, [u8_as_hex_avx512]

    .store:
	mov	[u8_as_hex_impl], r9

	fastcall r9,[dst],[src],r8
	ret
endp
