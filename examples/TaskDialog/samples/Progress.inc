ProgressTitle	GLOBWSTR 'Progress Sample',0
ProgressFormat	GLOBWSTR '%d%%',0

proc ProgressSample
    locals
	td TASKDIALOG_CONTEXT
	td_percent dd ?
    endl

	fastcall TaskDialogInit,addr td
	td_set_ptr td.cfg.pszWindowTitle,ProgressTitle
	or	dword [td.cfg.dwFlags],TDF_SHOW_PROGRESS_BAR or TDF_CALLBACK_TIMER
	td_set_handler td.msgOnTimer,OnTimer
	td_set_handler td.msgOnConstructed,OnConstructed

	invoke	TaskDialogIndirect,addr td.cfg,0,0,0
	ret

MaxRange = 5000
MinRange = 0

OnConstructed:
	enter	32,0
	invoke	SendMessageW,rcx,TDM_SET_PROGRESS_BAR_RANGE,0,\
		(MaxRange shl 16) + MinRange
	leave
	xor	eax,eax
	retn

OnTimer:
	cmp	r8,MaxRange
	jbe	.update
	mov	eax,S_FALSE
	retn

  .update:
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
	mov	[.ppos],r8
	td_local td_percent,.percent,rbx
	mov	ecx,MaxRange
	mov	eax,r8d
	imul	eax,100
	cdq
	div	ecx
	mov	[.percent],eax
	invoke	wsprintfW,addr .buffer,ProgressFormat,dword [.percent]
	invoke	SendMessageW,[.hTDlg],TDM_SET_ELEMENT_TEXT,TDE_MAIN_INSTRUCTION,addr .buffer
	invoke	SendMessageW,[.hTDlg],TDM_SET_PROGRESS_BAR_POS,[.ppos],0
	leave
	xor	eax,eax
	retn
endp
