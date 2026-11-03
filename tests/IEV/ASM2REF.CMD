@goto equ

	MAX_INSTRUCTION_LENGTH := 40

	virtual at 0
		opcodes:: file BINFILE
		TOP_OFFSET = $
	end virtual

	virtual at 0
		file FASFILE

		load preprocessed_offset:dword from 32
		load dump_offset:dword from 40
		load dump_length:dword from 44
		PREVIOUS_OFFSET = 0
		LINE_NUMBER = 0
		repeat dump_length/28, i:0
			load CURRENT_OFFSET:dword from dump_offset+i*28
			if (CURRENT_OFFSET>PREVIOUS_OFFSET)
				repeat 1, n:LINE_NUMBER
					load DATA#n:CURRENT_OFFSET-PREVIOUS_OFFSET from opcodes:PREVIOUS_OFFSET
				end repeat
				PREVIOUS_OFFSET = CURRENT_OFFSET
			end if
			load line_offset:dword from dump_offset+i*28+4
			load LINE_NUMBER:dword from preprocessed_offset+line_offset+4
		end repeat
		if (TOP_OFFSET>PREVIOUS_OFFSET)
			repeat 1, n:LINE_NUMBER
				load DATA#n:TOP_OFFSET-PREVIOUS_OFFSET from opcodes:PREVIOUS_OFFSET
			end repeat
		end if
	end virtual

	calminstruction hex_nibble digit*, command: db
		compute digit, 0FFh and '0123456789ABCDEF' shr (digit*8)
		arrange command, command digit
		assemble command
	end calminstruction

	calminstruction hex_dump data*, command: db
		local	digit, i
		compute i, 0
	loop:
		local	digit
		compute digit, (data shr (i*8+4)) and 0Fh
		call	hex_nibble, digit, command
		compute digit, (data shr (i*8)) and 0Fh
		call	hex_nibble, digit, command
		compute i, i + 1
		check	lengthof data > i
		jno	done
		local	separator
		arrange separator, command ' '
		assemble separator
		jump	loop
	done:
	end calminstruction

	macro processor
		PROCESSED = __FILE__
		macro ?! line&
			if __FILE__ = PROCESSED
				db `line
				repeat 1, n:__LINE__
					if defined DATA#n
						db MAX_INSTRUCTION_LENGTH-lengthof `line dup ' ', '; '
						hex_dump DATA#n
					end if
				end repeat
				db 13,10
			end if
		end macro
	end macro

	include SRCFILE, processor

:equ

@echo off
if not exist "%~1" goto info

fasm "%~1" "%~n1.bin" -s "%~n1.fas"
fasmg "%~f0" "%~n1.ref" -i"SRCFILE='%1'" -i"BINFILE='%~n1.bin'" -i"FASFILE='%~n1.fas'"
del "%~n1.bin"
del "%~n1.fas"

goto end

:info

echo Please provide a name of file containing x86 instructions.
echo This tool assembles them with fasm and generates a test reference.

:end