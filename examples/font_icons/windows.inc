format PE64 NX GUI 6.0

include 'win64wx.inc'
include 'macro/globwstr.inc'
include 'macro/globstr.inc'

GLOBWSTR.reuse := 1
GLOBSTR.reuse := 1

; Keep proc bodies stable across invoke/fastcall calls.
prologue@proc	equ static_rsp_prologue
epilogue@proc	equ static_rsp_epilogue
close@proc	equ static_rsp_close procname

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

	section '.rsrc' resource data readable
		directory RT_MANIFEST, manifests
		resource manifests,\
			1,LANG_ENGLISH+SUBLANG_DEFAULT,manifest

		manifest dd RVA manifest_data,manifest_size,0,0
		manifest_data:
			db '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>',13,10
			db '<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">',13,10
			db '  <assemblyIdentity version="1.0.0.0" processorArchitecture="amd64" name="fasm2.examples.font_icons" type="win32"/>',13,10
			db '  <description>fasm2 font icon samples</description>',13,10
			db '  <dependency>',13,10
			db '    <dependentAssembly>',13,10
			db '      <assemblyIdentity type="win32" name="Microsoft.Windows.Common-Controls" version="6.0.0.0" processorArchitecture="amd64" publicKeyToken="6595b64144ccf1df" language="*"/>',13,10
			db '    </dependentAssembly>',13,10
			db '  </dependency>',13,10
			db '</assembly>',13,10
		manifest_size = $ - manifest_data
		align 4

	.end start
end postpone
