BUTTON_SENDFEEDBACK = 100h
BUTTON_NOTHANKS	    = 101h

ErrorTitle1		GLOBWSTR 'Error Sample (page 1)',0
ErrorInstruction1	GLOBWSTR 'An unexpected error occurred. Please send feedback now!',0
ErrorVerification	GLOBWSTR 'Do not ask me again',0
ErrorContent1		GLOBWSTR 'Error message goes here...',0
ErrorExpanded1		GLOBWSTR 'Stack trace goes here...',0
ErrorSendText		GLOBWSTR 'Send Feedback',10,'I am in a giving mood',0
ErrorNoThanksText	GLOBWSTR 'No Thanks',10,'I do not feel like being helpful',0
ErrorTitle2		GLOBWSTR 'Error Sample (page 2)',0
ErrorContent2		GLOBWSTR 'Copying your passwords  ;-)',0
ErrorSendingFormat	GLOBWSTR 'Sending... %d%%',0
ErrorThanks		GLOBWSTR 'Thanks for the feedback!',0
ErrorDevelopers		GLOBWSTR 'Our developers will get right on that...',0

define __GLOBAL_DATA__ error_data

macro error_data
	TD_BUTTON_ARRAY ErrorButtons,\
		BUTTON_SENDFEEDBACK,ErrorSendText,\
		BUTTON_NOTHANKS,ErrorNoThanksText
purge error_data
end macro

proc ErrorSample
    locals
	td TASKDIALOG_CONTEXT
	verificationChecked dd ?
	td2 TASKDIALOG_CONTEXT
	feedbackPercent dd ?
    endl

	fastcall TaskDialogInit,addr td
	td_set_ptr td.cfg.pszWindowTitle,ErrorTitle1
	td_set_ptr td.cfg.pszMainInstruction,ErrorInstruction1
	td_set_ptr td.cfg.pszVerificationText,ErrorVerification
	td_set_ptr td.cfg.pszContent,ErrorContent1
	td_set_ptr td.cfg.pszExpandedInformation,ErrorExpanded1
	or	dword [td.cfg.dwFlags],\
		TDF_USE_COMMAND_LINKS or TDF_VERIFICATION_FLAG_CHECKED or TDF_EXPAND_FOOTER_AREA
	lea	rax,[ErrorButtons]
	mov	[td.cfg.pButtons],rax
	mov	dword [td.cfg.cButtons],ErrorButtons.count
	td_set_handler td.msgOnButton,OnButton

	invoke	TaskDialogIndirect,addr td.cfg,0,0,addr verificationChecked
	ret

OnButton:
	cmp	r8d,BUTTON_SENDFEEDBACK
	je	SendFeedback
	xor	eax,eax
	retn

SendFeedback:
	virtual at rbp - .frame
			rq 4
		.hTDlg	dq ?
			align.assume rbp,16
			align 16
		.frame := $ - $$
	end virtual
	enter	.frame,0
	mov	[.hTDlg],rcx
	td_local td2,.td2,rbx
	lea	r10,[.td2]
	fastcall TaskDialogInit,r10
	lea	rax,[ErrorTitle2]
	mov	[r10 + TASKDIALOG_CONTEXT.cfg.pszWindowTitle],rax
	lea	rax,[ErrorContent2]
	mov	[r10 + TASKDIALOG_CONTEXT.cfg.pszContent],rax
	or	dword [r10 + TASKDIALOG_CONTEXT.cfg.dwFlags],\
		TDF_SHOW_PROGRESS_BAR or TDF_CALLBACK_TIMER
	lea	rax,[OnTimer]
	mov	[r10 + TASKDIALOG_CONTEXT.msgOnTimer],rax
	lea	rax,[OnNavigate]
	mov	[r10 + TASKDIALOG_CONTEXT.msgOnNavigate],rax
	invoke	SendMessageW,[.hTDlg],TDM_NAVIGATE_PAGE,0,r10
	leave
	mov	eax,S_FALSE
	retn

MaxRange = 5000
MinRange = 0

OnNavigate:
	enter	32,0
	invoke	SendMessageW,rcx,TDM_SET_PROGRESS_BAR_RANGE,0,\
		(MaxRange shl 16) + MinRange
	leave
	xor	eax,eax
	retn

OnTimer:
	virtual at rbp - .frame
			rq 4
		.hTDlg	dq ?
		.ppos	dq ?
		label .buffer:128
			rw sizeof .buffer
			align.assume rbp,16
			align 16
		.frame := $ - $$
	end virtual
	enter	.frame,0
	mov	[.hTDlg],rcx
	cmp	r8,MaxRange
	ja	.finished
	label .percent:(sizeof feedbackPercent) at rbx + feedbackPercent - td2.msgOnCreate
	mov	ecx,MaxRange
	mov	eax,r8d
	imul	eax,100
	cdq
	div	ecx
	mov	[.percent],eax
	mov	[.ppos],r8
	invoke	wsprintfW,addr .buffer,ErrorSendingFormat,dword [.percent]
	invoke	SendMessageW,[.hTDlg],TDM_SET_ELEMENT_TEXT,TDE_MAIN_INSTRUCTION,addr .buffer
	jmp	.update_pos
  .finished:
	invoke	SendMessageW,[.hTDlg],TDM_SET_ELEMENT_TEXT,TDE_MAIN_INSTRUCTION,ErrorThanks
	invoke	SendMessageW,[.hTDlg],TDM_SET_ELEMENT_TEXT,TDE_CONTENT,ErrorDevelopers
	mov	qword [.ppos],MaxRange
  .update_pos:
	invoke	SendMessageW,[.hTDlg],TDM_SET_PROGRESS_BAR_POS,[.ppos],0
	leave
	xor	eax,eax
	retn
endp
