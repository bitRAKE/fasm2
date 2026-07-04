; u8_as_hex.inc - shared surface for the hexer example
; =====================================================
; Author: Rickey Bowers Jr. (bitRAKE)
; Co-developed with Claude (Anthropic).
;
; This file is included by every object that implements a u8_as_hex variant.
; It defines hextab - the nibble -> ASCII lookup - in an EXACT_MATCH COMDAT, so
; the linker folds the identical copies from somehex.obj and
; u8_as_hex_avx512.obj into one. Because the bytes are defined here, in one
; place, the two copies are byte-identical by construction and their CRC-32
; checksums match.
;
; Each variant then opens its own COMDAT code section named for its public
; symbol (e.g. '.text$u8_as_hex_base'), so /OPT:REF can discard the variants a
; given build never calls.
;
; ---------------------------------------------------------------------------
; Prototype shared by every variant (Microsoft x64 calling convention):
;
;   u8_as_hex_<isa>(rcx = dst, rdx = src, r8 = len)
;
;   Writes 2*len lowercase ASCII hex bytes to dst, in source order
;   (dst[2i] = high nibble of src[i], dst[2i+1] = low nibble).
;   Returns rax = dst + 2*len. Leaf function: clobbers only volatile GPRs
;   and vector registers, touches no memory but [dst) and [src).
;
; Every variant honours this contract exactly, which is what lets the
; dispatcher and the single-ISA builds swap one body for another freely.
; ---------------------------------------------------------------------------

; The shared, order-sensitive lookup: nibble value -> ASCII hex digit.
; 16 bytes, over-aligned to 64 so the AVX-512 body can broadcast it. Defined
; identically in every library object; EXACT_MATCH lets the linker keep one.
;
; Each variant that follows opens its own COMDAT code section, e.g.
;   section '.text$u8_as_hex_base' code readable executable comdat align 16
; so an unreferenced variant is removed by /OPT:REF without dragging the
; others along.
section '.rdata$hextab' data readable comdat exactmatch align 64
public hextab
hextab db '0123456789abcdef'
