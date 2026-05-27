BUTTON_ENABLE	= 101h
BUTTON_DISABLE	= 102h
RBUTTON_ABLE	= 103h

EnableDisableTitle	GLOBWSTR 'Enable/Disable Sample',0
EnableText		GLOBWSTR 'Enable',0
DisableText		GLOBWSTR 'Disable',0
RadioButtonText	GLOBWSTR 'Radio button',0

define __GLOBAL_DATA__ enable_disable_data

macro enable_disable_data
	TD_BUTTON_ARRAY EnableDisableButtons,\
		BUTTON_ENABLE,EnableText,\
		BUTTON_DISABLE,DisableText
	TD_BUTTON_ARRAY EnableDisableRadioButtons,\
		RBUTTON_ABLE,RadioButtonText
purge enable_disable_data
end macro

proc EnableDisableSample
    locals
	td TASKDIALOG_CONTEXT
    endl

	fastcall TaskDialogInit,addr td
	td_set_ptr td.cfg.pszWindowTitle,EnableDisableTitle
	lea	rax,[EnableDisableButtons]
	mov	[td.cfg.pButtons],rax
	mov	dword [td.cfg.cButtons],EnableDisableButtons.count
	lea	rax,[EnableDisableRadioButtons]
	mov	[td.cfg.pRadioButtons],rax
	mov	dword [td.cfg.cRadioButtons],EnableDisableRadioButtons.count
	td_set_handler td.msgOnButton,OnButton

	invoke	TaskDialogIndirect,addr td.cfg,0,0,0
	ret

OnButton:
	mov	eax,1
	cmp	r8d,BUTTON_ENABLE
	je	.update
	xor	eax,1
	cmp	r8d,BUTTON_DISABLE
	je	.update
	retn
  .update:
	virtual at rbp - .frame
			rq 4
		.hTDlg	dq ?
		.state	dq ?
		.frame := $ - $$
	end virtual
	enter	.frame,0
	mov	[.hTDlg],rcx
	mov	[.state],rax
	invoke	SendMessageW,[.hTDlg],TDM_ENABLE_RADIO_BUTTON,RBUTTON_ABLE,[.state]
	invoke	SendMessageW,[.hTDlg],TDM_ENABLE_BUTTON,BUTTON_DISABLE,[.state]
	xor	qword [.state],1
	invoke	SendMessageW,[.hTDlg],TDM_ENABLE_BUTTON,BUTTON_ENABLE,[.state]
	leave
	mov	eax,S_FALSE
	retn
endp
