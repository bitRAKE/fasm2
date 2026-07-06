; u8_as_hex_avx512.asm  ->  u8_as_hex_avx512.obj
; ============================================================================
; Author: Rickey Bowers Jr. (bitRAKE); co-developed with Claude (Anthropic).
;
; A single AVX-512 (F + BW) variant in its own object, kept apart from the
; somehex.obj family on purpose: it re-declares the SAME hextab (via
; u8_as_hex.inc). Because both objects contribute a byte-identical hextab in an
; EXACT_MATCH COMDAT, the linker folds the two copies into one - the payoff
; this example exists to show.
;
;   u8_as_hex_avx512(rcx=dst, rdx=src, r8=len) -> rax = dst + 2*len
;
; 64 source bytes -> 128 hex chars per pass; scalar remainder for the tail.
; ============================================================================

include '..\newcoff.inc'
format MS64 NEWCOFF
include 'u8_as_hex.inc'

use AMD64, AVX512F, AVX512BW
section '.text$u8_as_hex_avx512' code readable executable comdat align 64
public u8_as_hex_avx512
u8_as_hex_avx512:
cvproc u8_as_hex_avx512
	lea	rax, [rcx + r8*2]		; return value
	vbroadcasti32x4 zmm5, xword [hextab]	; nibble->ASCII table in all 4 lanes
	mov	r9d, 0x0F0F0F0F
	vpbroadcastd zmm4, r9d			; 0x0F in every byte
	vmovdqu64 zmm8, zword [.idx_lo]		; 128-bit-lane interleave selectors
	vmovdqu64 zmm9, zword [.idx_hi]
	cmp	r8, 64
	jb	.tail
    .loop64:
	vmovdqu8 zmm0, [rdx]			; 64 source bytes
	vpsrlw	zmm1, zmm0, 4
	vpandd	zmm1, zmm1, zmm4		; high nibble of each byte
	vpandd	zmm0, zmm0, zmm4		; low nibble of each byte
	vpshufb	zmm1, zmm5, zmm1		; high -> ASCII
	vpshufb	zmm0, zmm5, zmm0		; low  -> ASCII
	vpunpcklbw zmm2, zmm1, zmm0		; interleave (per 128-bit lane)
	vpunpckhbw zmm3, zmm1, zmm0
	vmovdqa64 zmm6, zmm8			; fresh index (vpermi2q overwrites it)
	vpermi2q zmm6, zmm2, zmm3		; linear order, chars for bytes 0..31
	vmovdqa64 zmm7, zmm9
	vpermi2q zmm7, zmm2, zmm3		; chars for bytes 32..63
	vmovdqu8 [rcx], zmm6
	vmovdqu8 [rcx+64], zmm7
	add	rdx, 64
	add	rcx, 128
	sub	r8, 64
	cmp	r8, 64
	jae	.loop64
    .tail:
	test	r8, r8
	jz	.done
	lea	r10, [hextab]			; scalar remainder (< 64 bytes)
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
cvendp

; qword selectors for vpermi2q: concatenate zmm2 (indices 0..7) and zmm3
; (indices 8..15), then pick lanes back into linear byte order.
	align 64
    .idx_lo dq 0,1, 8,9, 2,3, 10,11
    .idx_hi dq 4,5, 12,13, 6,7, 14,15
