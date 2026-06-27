; addon/windows.inc - small Win64 GUI executable policy for modular examples.
;
; Optional knobs before include:
;   ADDON_WINDOWS_RESOURCE equ 'program.res'
;   ADDON_WINDOWS_RESOURCE_CUSTOM = 1
;   macro ADDON_WINDOWS_RESOURCE
;     ; emit custom resource section(s)
;   end macro

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


macro ✓ line& ; fatal error reporting
	local message
	repeat 1,L:__LINE__
		message equ '"',`line,'" failed at line ',`L,'.'
	end repeat

	line ; execute line

	; Let TCHAR decide string type:
	fastcall.inline_string message
	lea rdx, [message] ; prepare for error reporting
end macro

.code

postpone
	align 64 ; HACK: later aligns in macros are nop for empty data

	if COFF._data_bytes_
		section '.data' data readable writeable
	end if

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

	COFF._data_bytes_ := $ - GLOBWSTR.baseaddr

	if definite ADDON_WINDOWS_RESOURCE_CUSTOM \
	& ADDON_WINDOWS_RESOURCE_CUSTOM ; non-zero
		ADDON_WINDOWS_RESOURCE
	else
		if defined ADDON_WINDOWS_RESOURCE
			section '.rsrc' resource from ADDON_WINDOWS_RESOURCE data readable
		end if
	end if

	.end start
end postpone
