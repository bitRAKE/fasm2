; SHA-256 self-test: the library (include/macro/sha256.inc) against the
; NESSIE vectors and FIPS 180 'abc'.
;	(from a repo prompt)  fasm2 scripts\sha256.inc nul
;
; fasmg implementation by bitRAKE (Rickey Bowers Jr.);
; test vectors @ https://www.cosic.esat.kuleuven.be/nessie/testvectors/hash/sha/index.html

include 'macro/sha256.inc'

; assert the published digest bytes area.0 .. area.31 against the
; expected digest expressed as its eight big-endian words
macro sha_verify? area*, w1*,w2*,w3*,w4*,w5*,w6*,w7*,w8*
	iterate w, w1,w2,w3,w4,w5,w6,w7,w8
		repeat 1, wi:(%-1)*4
			repeat 4, b:0
				repeat 1, j:(wi+b)
					assert area.j = (w shr (8*(3-b))) and 0xFF
				end repeat
			end repeat
		end repeat
	end iterate
end macro

virtual at 0
	m0::	; <empty>
end virtual
AreaSha256 m0
sha_verify m0, 0xE3B0C442,0x98FC1C14,0x9AFBF4C8,0x996FB924,0x27AE41E4,0x649B934C,0xA495991B,0x7852B855

virtual at 0
	m1:: db 'abc'
end virtual
AreaSha256 m1
sha_verify m1, 0xBA7816BF,0x8F01CFEA,0x414140DE,0x5DAE2223,0xB00361A3,0x96177A9C,0xB410FF61,0xF20015AD

virtual at 0
	m2:: db "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
end virtual
AreaSha256 m2
sha_verify m2, 0x248D6A61,0xD20638B8,0xE5C02693,0x0C3E6039,0xA33CE459,0x64FF2167,0xF6ECEDD4,0x19DB06C1

virtual at 0
	m3:: db "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
end virtual
AreaSha256 m3
sha_verify m3, 0xCF5B16A7,0x78AF8380,0x036CE59E,0x7B049237,0x0B249B11,0xE8F07A51,0xAFAC4503,0x7AFEE9D1

virtual at 0
	m4:: db 8 dup "1234567890"
end virtual
AreaSha256 m4
sha_verify m4, 0xF371BC4A,0x311F2B00,0x9EEF952D,0xD83CA80E,0x2B60026C,0x8E935592,0xD0F9C308,0x453C813E

display 'sha256 self-test: OK',10
