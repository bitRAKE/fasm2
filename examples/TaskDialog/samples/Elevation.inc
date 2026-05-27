BUTTON_ADMIN_STUFF = 101h

ElevationTitle		GLOBWSTR 'Elevation Required Sample',0
ElevationContent	GLOBWSTR 'Just a button with a shield ...',0
ElevationButtonText	GLOBWSTR 'Admin Stuff',0

define __GLOBAL_DATA__ elevation_data

macro elevation_data
	TD_BUTTON_ARRAY ElevationButtons,\
		BUTTON_ADMIN_STUFF,ElevationButtonText
purge elevation_data
end macro

proc ElevationSample
    locals
	td TASKDIALOG_CONTEXT
    endl

	fastcall TaskDialogInit,addr td
	td_set_ptr td.cfg.pszWindowTitle,ElevationTitle
	td_set_ptr td.cfg.pszContent,ElevationContent
	lea	rax,[ElevationButtons]
	mov	[td.cfg.pButtons],rax
	mov	dword [td.cfg.cButtons],ElevationButtons.count
	td_set_handler td.msgOnConstructed,OnConstructed

	invoke	TaskDialogIndirect,addr td.cfg,0,0,0
	ret

OnConstructed:
	enter	32,0
	invoke	SendMessageW,rcx,TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE,\
		BUTTON_ADMIN_STUFF,TRUE
	leave
	xor	eax,eax
	retn
endp
