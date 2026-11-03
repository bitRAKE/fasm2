; Instruction Encoding Verifier for fasm 1

virtual at 0
	__hex:: db '0123456789ABCDEF'
end virtual

macro @ line&
{
	local proxy, offset, length, reference, reference_length, a, b, c
	define proxy line
	match instruction =@ comment, proxy
	\{
		offset = $ - $$
		instruction
		length = $ - $$ - offset
		virtual at 0
			reference::
			irps chunk, comment
			\\{
				db 0x\\#chunk
			\\}
			reference_length = $
		end virtual
		c = 0
		if length = reference_length
			repeat length
				load a byte from $$+offset+%-1
				load b byte from reference:%-1
				if a <> b
					c = 1
					break
				end if
			end repeat
		else
			c = 1
		end if
		if c
			display 'Assembled:'
			repeat length
				load a byte from $$+offset+%-1
				load b byte from __hex:a shr 4
				load c byte from __hex:a and 0Fh
				display ' ',b,c
			end repeat
			display 13,10,'Reference:'
			repeat reference_length
				load a byte from reference:%-1
				load b byte from __hex:a shr 4
				load c byte from __hex:a and 0Fh
				display ' ',b,c
			end repeat
			display 13,10
			err
		end if
	\}
}