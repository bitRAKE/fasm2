; addon/windows.inc - small Win64 GUI executable policy for modular examples.
;
; Optional knobs before include:
;   ADDON_WINDOWS_RESOURCE equ 'program.res'

format PE64 NX GUI 6.0

include 'win64wx.inc'
include 'macro/globwstr.inc'
include 'macro/globstr.inc'
include 'encoding/utf8.inc'

GLOBWSTR.reuse := 1
GLOBSTR.reuse := 1

; Keep proc bodies stable across invoke/fastcall calls.
prologue@proc	equ static_rsp_prologue
epilogue@proc	equ static_rsp_epilogue
close@proc	equ static_rsp_close procname

; A/W-dispatching inline string materialiser. The W default follows win64w.inc.
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

macro _code
	section '.text' code readable executable
	if defined _winx.entry
		entry _winx.entry
	else
		entry $
	end if
end macro

.code

postpone
	section '.data' data readable writeable
		GLOBWSTR.here
		GLOBSTR.here

		align 16
		irpv xdata,__GLOBAL_DATA__
			xdata
		end irpv

		align 16
		irpv xbss,__GLOBAL_BSS__
			xbss
		end irpv

	if defined ADDON_WINDOWS_RESOURCE
		section '.rsrc' resource from ADDON_WINDOWS_RESOURCE data readable
	end if

	.end start
end postpone
