BTN_UPDATECONTENT		= 100h
BTN_UPDATEEXPANDEDINFORMATION	= 101h
BTN_UPDATEFOOTER		= 102h
BTN_UPDATEMAININSTRUCTION	= 103h
BTN_UPDATEWINDOWTITLE		= 3FFh

UpdateTitleText		GLOBWSTR 'Update Text Sample (Window Title)',0
UpdateInstruction	GLOBWSTR 'Main instruction',0
UpdateContent		GLOBWSTR 'Content',0
UpdateVerification	GLOBWSTR 'Verification text',0
UpdateExpanded		GLOBWSTR 'Expanded information',0
UpdateExpandedCtl	GLOBWSTR 'Expanded control text',0
UpdateCollapsedCtl	GLOBWSTR 'Collapsed control text',0
UpdateFooter		GLOBWSTR 'Footer',0
UpdateTitleButton	GLOBWSTR 'Window title',0
UpdateInstructionButton	GLOBWSTR 'Main instruction',0
UpdateContentButton	GLOBWSTR 'Content',0
UpdateFooterButton	GLOBWSTR 'Footer',0
UpdateExpandedButton	GLOBWSTR 'Expanded info',0
UpdateContentNew	GLOBWSTR 'Content (updated)',0
UpdateExpandedNew	GLOBWSTR 'Expanded information (updated)',0
UpdateFooterNew	GLOBWSTR 'Footer (updated)',0
UpdateInstructionNew	GLOBWSTR 'Main instruction (updated)',0
UpdateTitleNew		GLOBWSTR 'Update Text Sample (Updated Title)',0

define __GLOBAL_DATA__ update_text_data

macro update_text_data
	TD_BUTTON_ARRAY UpdateTextButtons,\
		BTN_UPDATEWINDOWTITLE,UpdateTitleButton,\
		BTN_UPDATEMAININSTRUCTION,UpdateInstructionButton,\
		BTN_UPDATECONTENT,UpdateContentButton,\
		BTN_UPDATEFOOTER,UpdateFooterButton,\
		BTN_UPDATEEXPANDEDINFORMATION,UpdateExpandedButton
	align 8
	UpdateTextTable:
		dq UpdateContentNew
		dq UpdateExpandedNew
		dq UpdateFooterNew
		dq UpdateInstructionNew
	UpdateTextTable.count = ($ - UpdateTextTable) shr 3
purge update_text_data
end macro

proc UpdateTextSample
    locals
	td TASKDIALOG_CONTEXT
	verificationChecked dd ?
    endl

	fastcall TaskDialogInit,addr td
	or	dword [td.cfg.dwFlags],TDF_EXPANDED_BY_DEFAULT
	td_set_ptr td.cfg.pszWindowTitle,UpdateTitleText
	td_set_ptr td.cfg.pszMainInstruction,UpdateInstruction
	td_set_ptr td.cfg.pszContent,UpdateContent
	td_set_ptr td.cfg.pszVerificationText,UpdateVerification
	td_set_ptr td.cfg.pszExpandedInformation,UpdateExpanded
	td_set_ptr td.cfg.pszExpandedControlText,UpdateExpandedCtl
	td_set_ptr td.cfg.pszCollapsedControlText,UpdateCollapsedCtl
	td_set_ptr td.cfg.pszFooter,UpdateFooter
	lea	rax,[UpdateTextButtons]
	mov	[td.cfg.pButtons],rax
	mov	dword [td.cfg.cButtons],UpdateTextButtons.count
	td_set_handler td.msgOnButton,OnButton

	invoke	TaskDialogIndirect,addr td.cfg,0,0,addr verificationChecked
	ret

OnButton:
	cmp	r8d,IDCANCEL
	jne	.check
	xor	eax,eax
	retn
  .check:
	enter	32,0
	cmp	r8d,BTN_UPDATEWINDOWTITLE
	je	.title
	movzx	r8,r8b
	cmp	r8d,UpdateTextTable.count
	jae	.done
	lea	r9,[UpdateTextTable]
	mov	r9,[r9+r8*8]
	invoke	SendMessageW,rcx,TDM_SET_ELEMENT_TEXT,r8,r9
	jmp	.done
  .title:
	invoke	SetWindowTextW,rcx,UpdateTitleNew
  .done:
	leave
	mov	eax,S_FALSE
	retn
endp
