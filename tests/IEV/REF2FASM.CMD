@goto equ

	MAX_INSTRUCTION_LENGTH := 40

	retaincomments

	macro processor
		PROCESSED = __FILE__
		macro ?! line&
			if __FILE__ = PROCESSED
				match instruction;comment, line
					db '@ ',`instruction,MAX_INSTRUCTION_LENGTH-lengthof `instruction dup ' ',' @ ',`comment
				else
					db `line
				end match
				db 13,10
			end if
		end macro
	end macro

	db "include 'iev.inc'",13,10

	include SRCFILE, processor

:equ

@echo off
if not exist "%~1" goto info

fasmg "%~f0" "%~n1.fasm" -i"SRCFILE='%1'"

goto end

:info

echo Please provide a name of file containing x86 test reference.
echo This tool converts the reference into IEV test for fasm.

:end