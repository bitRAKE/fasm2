
define GLOBWSTR GLOBWSTR

virtual at GLOBWSTR.baseaddr
	GLOBWSTR.buffer::
end virtual

postpone
	virtual GLOBWSTR.buffer
		assert ($-$$) and 1 = 0
		load GLOBWSTR.data:$-$$ from $$
	end virtual
	if ~ defined GLOBWSTR.where
		display 'Global string data has been placed automatically at the end of the executable.',13,10
		display 'To place it elsewhere, use "GLOBWSTR.here" macro in the intended location.',13,10
		align 2
		GLOBWSTR.baseaddr db GLOBWSTR.data
	end if
end postpone

macro GLOBWSTR.here
	align 2
	GLOBWSTR.baseaddr db GLOBWSTR.data
	GLOBWSTR.where := GLOBWSTR.baseaddr
end macro

if defined GLOBWSTR.reuse & GLOBWSTR.reuse

	struc GLOBWSTR data&
		local buffer,size,a,b,c,p,found
		virtual at 0
			buffer:: du data
			size := $
		end virtual
		virtual GLOBWSTR.buffer
			found = -1
			p = 0
			while p + size <= $-$$
				c = 0
				while c < size
					load a:byte from $$+p+c
					load b:byte from buffer:c
					if a <> b
						c = -1
						break
					end if
					c = c + 1
				end while
				if c = size
					found = p
					break
				end if
				p = p + 2
			end while
			if found >= 0
				label .:size shr 1 at $$+found
			else
				label .:size shr 1
				du data
			end if
		end virtual
	end struc

else

	struc GLOBWSTR data&
		local size
		virtual GLOBWSTR.buffer
			label .:size
			du data
			size := ($ - .) shr 1
		end virtual
	end struc

end if
