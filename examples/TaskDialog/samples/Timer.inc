BUTTON_RESET = 100h

TimerTitle	GLOBWSTR 'Timer Sample',0
TimerResetText	GLOBWSTR 'Reset',0
TimerFormat	GLOBWSTR 'Time elapsed: %d.%.3d seconds',0

define __GLOBAL_DATA__ timer_data

macro timer_data
	TD_BUTTON_ARRAY TimerButtons,\
		BUTTON_RESET,TimerResetText
purge timer_data
end macro

proc TimerSample
    locals
	td TASKDIALOG_CONTEXT
	td_reset dd ?
    endl

	fastcall TaskDialogInit,addr td
	mov	dword [td_reset],S_OK
	td_set_ptr td.cfg.pszWindowTitle,TimerTitle
	or	dword [td.cfg.dwFlags],TDF_CALLBACK_TIMER
	lea	rax,[TimerButtons]
	mov	[td.cfg.pButtons],rax
	mov	dword [td.cfg.cButtons],TimerButtons.count
	td_set_handler td.msgOnTimer,OnTimer
	td_set_handler td.msgOnButton,OnButton

	invoke	TaskDialogIndirect,addr td.cfg,0,0,0
	ret

OnButton:
	cmp	r8d,BUTTON_RESET
	je	.reset
	xor	eax,eax
	retn
  .reset:
	td_local td_reset,.reset_flag,rbx
	mov	dword [.reset_flag],S_FALSE
	mov	eax,S_FALSE
	retn

OnTimer:
	virtual at rbp - .frame
			rq 4
		.hTDlg	dq ?
		.seconds dq ?
		.millis	dd ?
		label .buffer:128
			rw sizeof .buffer
			align.assume rbp,16
			align 16
		.frame := $ - $$
	end virtual
	enter	.frame,0
	mov	[.hTDlg],rcx
	td_local td_reset,.reset_flag,rbx
	mov	eax,r8d
	xor	edx,edx
	mov	ecx,1000
	div	ecx
	mov	[.seconds],rax
	mov	[.millis],edx
	invoke	wsprintfW,addr .buffer,TimerFormat,dword [.seconds],dword [.millis]
	invoke	SendMessageW,[.hTDlg],TDM_SET_ELEMENT_TEXT,TDE_MAIN_INSTRUCTION,addr .buffer
	mov	eax,[.reset_flag]
	mov	dword [.reset_flag],S_OK
	leave
	retn
endp
