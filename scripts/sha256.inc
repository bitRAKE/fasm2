; SHA-256: (fasmg implementation by bitRAKE (Rickey Bowers Jr.))

; fasmg sha256.inc

; Some test messages to confirm algorithm:
; (@ https://www.cosic.esat.kuleuven.be/nessie/testvectors/hash/sha/index.html)
iterate __MESSAGE__, ,\; <empty>
db "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",\
db "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",\
db 8 dup "1234567890",file __FILE__

; Expected output:
;	E3B0C442 98FC1C14 9AFBF4C8 996FB924 27AE41E4 649B934C A495991B 7852B855
;	248D6A61 D20638B8 E5C02693 0C3E6039 A33CE459 64FF2167 F6ECEDD4 19DB06C1
;	CF5B16A7 78AF8380 036CE59E 7B049237 0B249B11 E8F07A51 AFAC4503 7AFEE9D1
;	F371BC4A 311F2B00 9EEF952D D83CA80E 2B60026C 8E935592 D0F9C308 453C813E
;	<unknown>

; Initialize hash values (first 32 bits of the fractional part of the square
; root of the first 8 primes 2..19):
iterate H,0x6A09E667,0xBB67AE85,0x3C6EF372,0xA54FF53A,0x510E527F,0x9B05688C,0x1F83D9AB,0x5BE0CD19
        h#% = H ; (note how these and v% below are one based)
end iterate

; Message pre-processing (padding such that the bits in the message (L) are:
;       L 1 00..00 <L as 64 bit integer> = k*512 total bits
virtual at 0
__MESSAGE__
message_bits = ($ shl 3)
db 0x80 ; first pad byte has high bit set (must output one bit)
while ($ + 8) mod 64
        db 0
end while
dq message_bits bswap 8 ; Note: big-endian

repeat $ shr 6,current_block:0
        ; Load block into working space [w_]:
        repeat 16,_:0
                load w#_ dword from (current_block shl 6)+(_ shl 2)
                w#_ = w#_ bswap 4 ; Note: big-endian
        end repeat
        repeat 48,_:16,_16:0,_15:1,_7:9,_2:14
                s0_a rightrotate w#_15,7,32
                s0_b rightrotate w#_15,18,32
                s0 = s0_a xor s0_b xor (w#_15 shr 3)

                s1_a rightrotate w#_2,17,32
                s1_b rightrotate w#_2,19,32
                s1 = s1_a xor s1_b xor (w#_2 shr 10)
                w#_ = (w#_16 + w#_7 + s1 + s0) and $FFFF'FFFF
        end repeat

        ; Initialize working variables [v_] to current hash value:
        repeat 8
                v#% = h#%
        end repeat

        ; Compression function main loop:
        repeat 64,_:0
                s0_a rightrotate v1,2,32
                s0_b rightrotate v1,13,32
                s0_c rightrotate v1,22,32
                temp2 = ((v1 and v2) xor (v1 and v3) xor (v2 and v3))\; majority
                        + (s0_a xor s0_b xor s0_c)

                s1_a rightrotate v5,6,32
                s1_b rightrotate v5,11,32
                s1_c rightrotate v5,25,32
                temp1 = ((v5 and v6) xor ((not v5) and v7))\; choice
                        + (s1_a xor s1_b xor s1_c)\
                        + v8 + K#_ + w#_
                v8 = v7
                v7 = v6
                v6 = v5
                v5 = v4 + temp1
                v4 = v3
                v3 = v2
                v2 = v1
                v1 = temp1 + temp2
        end repeat ; compression

	; Add the compressed chunk to the current hash value (clamp):
	repeat 8
		h#% = (h#% + v#%) and $FFFF'FFFF
	end repeat

end repeat ; message_blocks

end virtual

; Display resulting hash:
repeat 8
        h#% Show_Hexadecimal 8
        display ' '
end repeat
display 10

end iterate ; __MESSAGE__

; Array of round constants (first 32 bits of the fractional part of the cube root of the first 64 primes 2..311):
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


;---------------------------------------
; minor support routines:

calminstruction (name) rightrotate value*,bits*,range*
	compute bits,bits mod range
	compute value,(((value) and ((1 shl range)-1)) shr bits) \
		or (((value) and ((1 shl bits)-1)) shl (range - bits))
	publish name,value
end calminstruction
;struc rightrotate value*,bits*,range*
;	local rshift
;	; clamp rotate to range
;	rshift = bits mod range
;	; ignore bits outside range
;	. = (((value) and ((1 shl range)-1)) shr rshift) \
;	or (((value) and ((1 shl rshift)-1)) shl (range - rshift))
;end struc

virtual at 0
	HexDigits:: db '0123456789ABCDEF'
end virtual

struc Show_Hexadecimal force_digits=0
	local digits,nibble,char
	if force_digits > 0
		; to zero pad field or truncate output
		digits = force_digits
	else if . = 0
		digits = 1
	else ; variable length nibble calculation
		digits = ((bsr .)+3) shr 2
	end if

	repeat digits
		; need to output high nibbles first (or zero padding)
		nibble = (. shr (4 * (%% - %))) and $F
		load char byte from HexDigits:nibble
		display char
	end repeat
	; update for possible further use
	. = . shr (4*digits)
end struc
    