BUTTON_STATE = 100h
BUTTON_MODE  = 101h

ProgressEffectsTitle	GLOBWSTR 'Progress Effects Sample',0
ProgressStateText	GLOBWSTR 'Cycle State',0
ProgressModeText	GLOBWSTR 'Toggle Mode',0
ProgressFxFormat	GLOBWSTR 'State: %ls',10,'Marquee: %ls',0
ProgressStateNormal	GLOBWSTR 'Normal',0
ProgressStateError	GLOBWSTR 'Error',0
ProgressStatePaused	GLOBWSTR 'Paused',0
ProgressMarqueeFalse	GLOBWSTR 'False',0
ProgressMarqueeTrue	GLOBWSTR 'True (only togglable in Normal state)',0

define __GLOBAL_DATA__ progress_effects_data

macro progress_effects_data
	TD_BUTTON_ARRAY ProgressEffectsButtons,\
		BUTTON_STATE,ProgressStateText,\
		BUTTON_MODE,ProgressModeText
	align 8
	ProgressStateTable:
		dq ProgressStateNormal
		dq ProgressStateError
		dq ProgressStatePaused
purge progress_effects_data
end macro

proc ProgressEffectsSample
    locals
	td TASKDIALOG_CONTEXT
	td_marquee dd ?
	td_state dd ?
    endl

	fastcall TaskDialogInit,addr td
	mov	dword [td_marquee],0
	mov	dword [td_state],PBST_NORMAL
	td_set_ptr td.cfg.pszWindowTitle,ProgressEffectsTitle
	or	dword [td.cfg.dwFlags],TDF_SHOW_PROGRESS_BAR
	lea	rax,[ProgressEffectsButtons]
	mov	[td.cfg.pButtons],rax
	mov	dword [td.cfg.cButtons],ProgressEffectsButtons.count
	td_set_handler td.msgOnButton,OnButton
	td_set_handler td.msgOnConstructed,OnConstructed

	invoke	TaskDialogIndirect,addr td.cfg,0,0,0
	ret

OnButton:
	cmp	r8d,BUTTON_STATE
	je	.cycle_request
	cmp	r8d,BUTTON_MODE
	je	ToggleMode
	xor	eax,eax
	retn
  .cycle_request:
	mov	r8d,BUTTON_STATE
	jmp	ToggleMode

OnConstructed:
	mov	r8d,BUTTON_MODE

ToggleMode:
	virtual at rbp - .frame
			rq 4
		.hTDlg	dq ?
		.statep dq ?
		.marqueep dq ?
		label .buffer:160
			rw sizeof .buffer
			align.assume rbp,16
			align 16
		.frame := $ - $$
	end virtual
	enter	.frame,0
	mov	[.hTDlg],rcx
	td_local td_state,.state,rbx
	td_local td_marquee,.marquee,rbx
	lea	r10,[.state]
	lea	r11,[.marquee]
	mov	[.statep],r10
	mov	[.marqueep],r11
	cmp	r8d,BUTTON_STATE
	je	.cycle
	xor	dword [.marquee],1
	jmp	.apply

  .cycle:
	mov	eax,[.state]
	inc	eax
	cmp	eax,PBST_PAUSED
	jbe	.store_state
	mov	eax,PBST_NORMAL
  .store_state:
	mov	[.state],eax
  .apply:
	mov	r8d,[.state]
	invoke	SendMessageW,[.hTDlg],TDM_SET_PROGRESS_BAR_STATE,r8,0
	mov	r8d,[.marquee]
	invoke	SendMessageW,[.hTDlg],TDM_SET_MARQUEE_PROGRESS_BAR,r8,0
	test	dword [.marquee],-1
	jz	.marquee_off
	invoke	SendMessageW,[.hTDlg],TDM_SET_PROGRESS_BAR_MARQUEE,1,0
	jmp	.text
  .marquee_off:
	invoke	SendMessageW,[.hTDlg],TDM_SET_PROGRESS_BAR_POS,25,0
  .text:
	mov	eax,[.state]
	dec	eax
	lea	r10,[ProgressStateTable]
	mov	r10,[r10+rax*8]
	lea	r11,[ProgressMarqueeFalse]
	cmp	dword [.marquee],0
	je	.have_marquee_text
	lea	r11,[ProgressMarqueeTrue]
  .have_marquee_text:
	invoke	wsprintfW,addr .buffer,ProgressFxFormat,r10,r11
	invoke	SendMessageW,[.hTDlg],TDM_SET_ELEMENT_TEXT,TDE_MAIN_INSTRUCTION,addr .buffer
	leave
	mov	eax,S_FALSE
	retn
endp
