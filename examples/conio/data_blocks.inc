
; late processing data groups
;	- dedup A/W string interfaces
;	- CONST/DATA/BSS gatherers

include 'macro\globwstr.inc'
GLOBWSTR.reuse := 1
include 'macro\globstr.inc'
GLOBSTR.reuse := 1

; Explicit A/W-dispatching or TCHAR based inline string materialiser.
macro fastcall?.inline_string var
	local data
	match =A =, value,var
		data GLOBSTR value,0
	else match =W =, value,var
		data GLOBWSTR value,0
	else
		if sizeof.TCHAR = 2
			data GLOBWSTR var,0
		else
			data GLOBSTR var,0
		end if
	end match
	redefine var data
end macro


iterate block,\
	≡,\	; CONST, readonly data
	∵,\	; DATA, initialized data
	□	; BSS, uninitialized data

	define block block
	namespace block
		define GATHER
		calminstruction item line& ; namespace aware
			match ,line
			jyes ⌽
			take GATHER,line
			exit

		⌽:	take line,GATHER
			jyes ⌽
		out:	assemble line
			take ,line
			take line,line
			jyes out
			arrange line,=block=.=end:
			assemble line
			arrange GATHER,=err 'block expressed'
		end calminstruction
		restore GATHER
	end namespace
	mvmacro block,block.item

	block align 64		; cache line separation
	block block.start:
end iterate

postpone

; Note: Constant data is allowed in code section.
;	section '.rdata' data readable align 64
	GLOBWSTR.here
	GLOBSTR.here

	≡	; output readonly constant data

	if □.end - ∵.start
		section '.data' data readable writeable align 64
	end if
	∵	; output initialized data
	□	; output uninitialized data

	repeat 1,\
		I:≡.end - GLOBWSTR.baseaddr,\
		J:∵.end - ∵.start,\
		K:□.end - □.start

		display 10,9,`I,' CONST bytes',\
			10,9,`J,' DATA bytes',\
			10,9,`K,' BSS bytes'
	end repeat
end postpone
