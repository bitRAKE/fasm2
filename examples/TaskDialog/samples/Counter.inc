BUTTON_INCREMENT = 101h
BUTTON_DECREMENT = 102h

CounterTitle	GLOBWSTR 'Counter Sample',0
CounterFormat	GLOBWSTR 'We have %d bugs.',0
CounterIncText	GLOBWSTR 'Increment',0
CounterDecText	GLOBWSTR 'Decrement',0

define __GLOBAL_DATA__ counter_data

macro counter_data
	TD_BUTTON_ARRAY CounterButtons,\
		BUTTON_INCREMENT,CounterIncText,\
		BUTTON_DECREMENT,CounterDecText
purge counter_data
end macro

proc CounterSample
    locals
	td TASKDIALOG_CONTEXT
	td_count dd ?
    endl

	fastcall TaskDialogInit,addr td
	mov	dword [td_count],0
	td_set_ptr td.cfg.pszWindowTitle,CounterTitle
	lea	rax,[CounterButtons]
	mov	[td.cfg.pButtons],rax
	mov	dword [td.cfg.cButtons],CounterButtons.count
	td_set_handler td.msgOnButton,OnButton
	td_set_handler td.msgOnConstructed,OnConstructed

	invoke	TaskDialogIndirect,addr td.cfg,0,0,0
	ret

OnButton:
	td_local td_count,.count,rbx
	mov	edx,-1
	cmp	r8d,BUTTON_DECREMENT
	je	.change
	neg	edx
	cmp	r8d,BUTTON_INCREMENT
	je	.change
	xor	eax,eax
	retn
  .change:
	add	dword [.count],edx

OnConstructed:
	virtual at rbp - .frame
			rq 4
		.hTDlg	dq ?
		label .buffer:128
			rw sizeof .buffer
			align.assume rbp,16
			align 16
		.frame := $ - $$
	end virtual
	enter	.frame,0
	mov	[.hTDlg],rcx
	td_local td_count,.count,rbx
	invoke	wsprintfW,addr .buffer,CounterFormat,dword [.count]
	invoke	SendMessageW,[.hTDlg],TDM_SET_ELEMENT_TEXT,TDE_MAIN_INSTRUCTION,addr .buffer
	leave
	mov	eax,S_FALSE
	retn
endp
