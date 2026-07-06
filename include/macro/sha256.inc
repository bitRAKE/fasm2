; SHA-256 as a generic byte-generator consumer (fasmg implementation by
; bitRAKE (Rickey Bowers Jr.); interface co-developed with Claude
; (Anthropic))
;
;	SHA256.calc file 'source.asm'		; any byte generator:
;	SHA256.calc db 'abc'			; file / db / a macro / ...
;	...
;	db SHA256.result			; 32-byte digest string
;
; The generator statement runs ONCE, directly into the hash's padding
; area - the data is never copied or regenerated. SHA256.result is a
; 32-character string (trailing zero bytes preserved), so other hash
; functions can share the interface regardless of digest size.
;
; THE ANCHOR RULE (why the SHA256:: line exists): dotted names resolve
; their first component like any identifier - current namespace, then
; the fallback chain - but a namespace anchor auto-created by a bare
; NAMESPACE directive is invisible to that lookup, so from inside any
; other namespace SHA256.calc would become CALLER.SHA256.calc and fail.
; With the anchor DEFINED as a real symbol, invocation, reads and even
; writes of SHA256.* resolve to this global namespace from anywhere.
; (Macro bodies still resolve at the CALL site, so everything in calc
; is spelled SHA256.* or hangs off the macro-local pad.)
;
; Verified against the NESSIE test vectors: scripts/sha256.inc.

if ~ definite SHA256_INC
SHA256_INC := 1

struc rightrotate? value*,bits*,range*
	local rshift
	rshift = bits mod range
	. = (((value) and ((1 shl range)-1)) shr rshift) \
		or (((value) and ((1 shl rshift)-1)) shl (range - rshift))
end struc

virtual at 0
	SHA256::	; the defined anchor - see THE ANCHOR RULE above
end virtual

namespace SHA256

	; round constants (first 32 bits of the fractional part of the cube
	; roots of the first 64 primes 2..311)
	iterate k,\
		0x428A2F98,0x71374491,0xB5C0FBCF,0xE9B5DBA5,0x3956C25B,0x59F111F1,0x923F82A4,0xAB1C5ED5,\
		0xD807AA98,0x12835B01,0x243185BE,0x550C7DC3,0x72BE5D74,0x80DEB1FE,0x9BDC06A7,0xC19BF174,\
		0xE49B69C1,0xEFBE4786,0x0FC19DC6,0x240CA1CC,0x2DE92C6F,0x4A7484AA,0x5CB0A9DC,0x76F988DA,\
		0x983E5152,0xA831C66D,0xB00327C8,0xBF597FC7,0xC6E00BF3,0xD5A79147,0x06CA6351,0x14292967,\
		0x27B70A85,0x2E1B2138,0x4D2C6DFC,0x53380D13,0x650A7354,0x766A0ABB,0x81C2C92E,0x92722C85,\
		0xA2BFE8A1,0xA81A664B,0xC24B8B70,0xC76C51A3,0xD192E819,0xD6990624,0xF40E3585,0x106AA070,\
		0x19A4C116,0x1E376C08,0x2748774C,0x34B0BCB5,0x391C0CB3,0x4ED8AA4A,0x5B9CCA4F,0x682E6FF3,\
		0x748F82EE,0x78A5636F,0x84C87814,0x8CC70208,0x90BEFFFA,0xA4506CEB,0xBEF9A3F7,0xC67178F2
		repeat 1,_:%-1
			K#_ := k
		end repeat
	end iterate

	result = ''

	macro calc? bytes&	; generic byte generator: file / db / macro
		local pad
		; the generator emits straight into the padded message area:
		; L 1 00..00 <L as 64-bit big-endian>
		virtual at 0
			pad::
			bytes
			pad.bits = $ shl 3
			db 0x80
			while ($ + 8) mod 64
				db 0
			end while
			dq pad.bits bswap 8
		end virtual
		; initial hash values (first 32 bits of the fractional part of
		; the square roots of the first 8 primes 2..19)
		iterate H,0x6A09E667,0xBB67AE85,0x3C6EF372,0xA54FF53A,0x510E527F,0x9B05688C,0x1F83D9AB,0x5BE0CD19
			pad.h#% = H
		end iterate
		repeat sizeof pad shr 6, current_block:0
			; message schedule
			repeat 16,_:0
				load pad.w#_ : 4 from pad : (current_block shl 6)+(_ shl 2)
				pad.w#_ = pad.w#_ bswap 4
			end repeat
			repeat 48,_:16,_16:0,_15:1,_7:9,_2:14
				pad.s0_a rightrotate pad.w#_15,7,32
				pad.s0_b rightrotate pad.w#_15,18,32
				pad.s0 = pad.s0_a xor pad.s0_b xor (pad.w#_15 shr 3)
				pad.s1_a rightrotate pad.w#_2,17,32
				pad.s1_b rightrotate pad.w#_2,19,32
				pad.s1 = pad.s1_a xor pad.s1_b xor (pad.w#_2 shr 10)
				pad.w#_ = (pad.w#_16 + pad.w#_7 + pad.s1 + pad.s0) and $FFFF'FFFF
			end repeat
			; working variables from current hash value
			repeat 8
				pad.v#% = pad.h#%
			end repeat
			; compression
			repeat 64,_:0
				pad.s0_a rightrotate pad.v1,2,32
				pad.s0_b rightrotate pad.v1,13,32
				pad.s0_c rightrotate pad.v1,22,32
				pad.temp2 = ((pad.v1 and pad.v2) xor (pad.v1 and pad.v3) xor (pad.v2 and pad.v3))\; majority
					+ (pad.s0_a xor pad.s0_b xor pad.s0_c)
				pad.s1_a rightrotate pad.v5,6,32
				pad.s1_b rightrotate pad.v5,11,32
				pad.s1_c rightrotate pad.v5,25,32
				pad.temp1 = ((pad.v5 and pad.v6) xor ((not pad.v5) and pad.v7))\; choice
					+ (pad.s1_a xor pad.s1_b xor pad.s1_c)\
					+ pad.v8 + SHA256.K#_ + pad.w#_
				pad.v8 = pad.v7
				pad.v7 = pad.v6
				pad.v6 = pad.v5
				pad.v5 = pad.v4 + pad.temp1
				pad.v4 = pad.v3
				pad.v3 = pad.v2
				pad.v2 = pad.v1
				pad.v1 = pad.temp1 + pad.temp2
			end repeat
			; fold the compressed chunk into the hash value (clamp)
			repeat 8
				pad.h#% = (pad.h#% + pad.v#%) and $FFFF'FFFF
			end repeat
		end repeat
		; the digest as a 32-character string: big-endian words in digest
		; order, trailing zero bytes preserved by the sized load
		virtual at 0
			pad.dig::
			repeat 8
				dd pad.h#% bswap 4
			end repeat
		end virtual
		load pad.digest : 32 from pad.dig : 0
		SHA256.result = pad.digest
	end macro

end namespace

end if ;~ definite SHA256_INC
