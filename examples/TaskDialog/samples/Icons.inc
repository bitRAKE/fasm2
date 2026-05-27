RBTN_NONE		= 100h
RBTN_WARNING		= 101h
RBTN_ERROR		= 102h
RBTN_INFORMATION	= 103h
RBTN_SHIELD		= 104h
RBTN_S_BSHIELD		= 105h
RBTN_S_REDX		= 106h
RBTN_S_YELLOW		= 107h
RBTN_S_CHECK		= 108h
RBTN_S_GSHIELD		= 109h

IconsTitle		GLOBWSTR 'Icons Sample',0
IconsInstruction	GLOBWSTR 'Main Instruction',0
IconsContent		GLOBWSTR 'Security icons are undocumented.',0
IconsFooter		GLOBWSTR 'Footer',0
IconNoneText		GLOBWSTR 'None',0
IconWarningText		GLOBWSTR 'Warning',0
IconInformationText	GLOBWSTR 'Information',0
IconErrorText		GLOBWSTR 'Error',0
IconShieldText		GLOBWSTR 'Shield',0
IconBlueShieldText	GLOBWSTR 'Security Shield Blue',0
IconRedXText		GLOBWSTR 'Security Warning',0
IconYellowText		GLOBWSTR 'Security Error',0
IconCheckText		GLOBWSTR 'Security Success',0
IconGrayShieldText	GLOBWSTR 'Security Shield Gray',0

define __GLOBAL_DATA__ icons_data

macro icons_data
	TD_BUTTON_ARRAY IconRadioButtons,\
		RBTN_NONE,IconNoneText,\
		RBTN_WARNING,IconWarningText,\
		RBTN_INFORMATION,IconInformationText,\
		RBTN_ERROR,IconErrorText,\
		RBTN_SHIELD,IconShieldText,\
		RBTN_S_BSHIELD,IconBlueShieldText,\
		RBTN_S_REDX,IconRedXText,\
		RBTN_S_YELLOW,IconYellowText,\
		RBTN_S_CHECK,IconCheckText,\
		RBTN_S_GSHIELD,IconGrayShieldText
purge icons_data
end macro

proc IconsSample
    locals
	td TASKDIALOG_CONTEXT
    endl

	fastcall TaskDialogInit,addr td
	td_set_ptr td.cfg.pszWindowTitle,IconsTitle
	td_set_ptr td.cfg.pszMainInstruction,IconsInstruction
	td_set_ptr td.cfg.pszContent,IconsContent
	td_set_ptr td.cfg.pszFooter,IconsFooter
	lea	rax,[IconRadioButtons]
	mov	[td.cfg.pRadioButtons],rax
	mov	dword [td.cfg.cRadioButtons],IconRadioButtons.count
	td_set_handler td.msgOnRadio,OnRadio

	invoke	TaskDialogIndirect,addr td.cfg,0,0,0
	ret

OnRadio:
	xor	r9d,r9d
	cmp	r8d,RBTN_NONE
	je	.update
	movzx	r9,r8b
	neg	r9w
  .update:
	virtual at rbp - .frame
			rq 4
		.hTDlg	dq ?
		.icon	dq ?
		.frame := $ - $$
	end virtual
	enter	.frame,0
	mov	[.hTDlg],rcx
	mov	[.icon],r9
	invoke	SendMessageW,[.hTDlg],TDM_UPDATE_ICON,TDIE_ICON_MAIN,[.icon]
	invoke	SendMessageW,[.hTDlg],TDM_UPDATE_ICON,TDIE_ICON_FOOTER,[.icon]
	leave
	xor	eax,eax
	retn
endp
