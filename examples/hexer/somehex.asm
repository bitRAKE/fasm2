; somehex.asm  ->  somehex.obj
; ============================================================================
; Author: Rickey Bowers Jr. (bitRAKE); co-developed with Claude (Anthropic).
;
; Several byte->hex implementations sharing one prototype, each postfixed with
; the ISA it targets and placed in its own COMDAT so /OPT:REF can drop the ones
; a given build never calls. The shared hextab lives in u8_as_hex.inc as an
; EXACT_MATCH COMDAT, so this object and u8_as_hex_avx512.obj contribute
; byte-identical copies the linker folds into one.
;
; Prototype (see u8_as_hex.inc):
;   u8_as_hex_<isa>(rcx=dst, rdx=src, r8=len) -> rax = dst + 2*len
; ============================================================================

format MS64 COFF
include 'u8_as_hex.inc'

; ---- scalar baseline -------------------------------------------------------
use AMD64
section '.text$u8_as_hex_base' code readable executable comdat align 16
public u8_as_hex_base
u8_as_hex_base:
	lea	r10, [hextab]
	lea	rax, [rcx + r8*2]	; return value = end of dst
	test	r8, r8
	jz	.done
    .next:
	movzx	r9d, byte [rdx]
	inc	rdx
	mov	r11d, r9d
	shr	r11d, 4			; high nibble
	and	r9d, 15			; low nibble
	mov	r11b, [r10 + r11]	; -> ASCII
	mov	[rcx], r11b
	mov	r9b, [r10 + r9]
	mov	[rcx+1], r9b
	add	rcx, 2
	dec	r8
	jnz	.next
    .done:
	retn

; ---- AVX2: 32 input bytes -> 64 hex chars per pass -------------------------
use AMD64, AVX, AVX2
section '.text$u8_as_hex_avx2' code readable executable comdat align 16
public u8_as_hex_avx2
u8_as_hex_avx2:
	lea	rax, [rcx + r8*2]		; return value
	vbroadcasti128 ymm5, xword [hextab]	; nibble->ASCII table in both lanes
	mov	r9d, 0x0F0F0F0F
	vmovd	xmm4, r9d
	vpbroadcastd ymm4, xmm4			; 0x0F in every byte
	cmp	r8, 32
	jb	.tail
    .loop32:
	vmovdqu	ymm0, [rdx]			; 32 source bytes
	vpsrlw	ymm1, ymm0, 4
	vpand	ymm1, ymm1, ymm4		; high nibble of each byte
	vpand	ymm0, ymm0, ymm4		; low nibble of each byte
	vpshufb	ymm1, ymm5, ymm1		; high -> ASCII
	vpshufb	ymm0, ymm5, ymm0		; low  -> ASCII
	vpunpcklbw ymm2, ymm1, ymm0		; interleave (per 128-bit lane)
	vpunpckhbw ymm3, ymm1, ymm0
	vperm2i128 ymm6, ymm2, ymm3, 0x20	; restore linear byte order
	vperm2i128 ymm7, ymm2, ymm3, 0x31
	vmovdqu	[rcx], ymm6
	vmovdqu	[rcx+32], ymm7
	add	rdx, 32
	add	rcx, 64
	sub	r8, 32
	cmp	r8, 32
	jae	.loop32
    .tail:
	test	r8, r8
	jz	.done
	lea	r10, [hextab]			; scalar remainder (< 32 bytes)
    .tnext:
	movzx	r9d, byte [rdx]
	inc	rdx
	mov	r11d, r9d
	shr	r11d, 4
	and	r9d, 15
	mov	r11b, [r10 + r11]
	mov	[rcx], r11b
	mov	r9b, [r10 + r9]
	mov	[rcx+1], r9b
	add	rcx, 2
	dec	r8
	jnz	.tnext
    .done:
	vzeroupper
	retn
