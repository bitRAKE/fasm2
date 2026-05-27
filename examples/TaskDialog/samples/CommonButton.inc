CommonButtonsTitle	GLOBWSTR 'Common Buttons Sample',0
CommonButtonUnknown	GLOBWSTR 'Unknown',0
CommonButtonOk		GLOBWSTR 'OK clicked',0
CommonButtonCancel	GLOBWSTR 'Cancel clicked',0
CommonButtonRetry	GLOBWSTR 'Retry clicked',0
CommonButtonYes		GLOBWSTR 'Yes clicked',0
CommonButtonNo		GLOBWSTR 'No clicked',0
CommonButtonClose	GLOBWSTR 'Close clicked',0

define __GLOBAL_DATA__ common_buttons_data

macro common_buttons_data
	align 8
	CommonButtonTextTable:
		dq CommonButtonUnknown
		dq CommonButtonOk
		dq CommonButtonCancel
		dq CommonButtonUnknown
		dq CommonButtonRetry
		dq CommonButtonUnknown
		dq CommonButtonYes
		dq CommonButtonNo
		dq CommonButtonClose
	CommonButtonTextTable.count = ($ - CommonButtonTextTable) shr 3
purge common_buttons_data
end macro

proc CommonButtonsSample
    locals
	td TASKDIALOG_CONTEXT
    endl

	fastcall TaskDialogInit,addr td
	td_set_ptr td.cfg.pszWindowTitle,CommonButtonsTitle
	mov	dword [td.cfg.dwCommonButtons],\
		TDCBF_OK_BUTTON or TDCBF_YES_BUTTON or TDCBF_NO_BUTTON or \
		TDCBF_CANCEL_BUTTON or TDCBF_RETRY_BUTTON or TDCBF_CLOSE_BUTTON
	td_set_handler td.msgOnButton,OnButton

	invoke	TaskDialogIndirect,addr td.cfg,0,0,0
	ret

OnButton:
	cmp	r8d,IDCANCEL
	je	.close
	lea	r10,[CommonButtonUnknown]
	cmp	r8d,CommonButtonTextTable.count
	jae	.have_text
	lea	r11,[CommonButtonTextTable]
	mov	r10,[r11+r8*8]
  .have_text:
	enter	32,0
	invoke	SendMessageW,rcx,TDM_SET_ELEMENT_TEXT,TDE_MAIN_INSTRUCTION,r10
	leave
	mov	eax,S_FALSE
	retn
  .close:
	xor	eax,eax
	retn
endp
