ASYNC_BTN_INDEX		= 301h
ASYNC_BTN_REPAIR	= 302h
ASYNC_BTN_ELEVATE	= 303h

ASYNC_MODE_INDEX	= 0
ASYNC_MODE_REPAIR	= 1
ASYNC_MODE_ELEVATED	= 2

ASYNC_PHASE_SETUP	= 0
ASYNC_PHASE_RUN		= 1
ASYNC_PHASE_CANCEL	= 2

AsyncTitle		GLOBWSTR 'Async Operation Sample',0
AsyncPickInstruction	GLOBWSTR 'Choose an operation to run:',0
AsyncPickContent	GLOBWSTR 'This sample keeps one dialog open while navigating between TaskDialog pages. The timer callback behaves like a worker completion pump.',0
AsyncIndexText		GLOBWSTR 'Index workspace',10,'Simulates a successful background operation.',0
AsyncRepairText		GLOBWSTR 'Repair protected settings',10,'Simulates a failed operation that can be retried with elevation.',0
AsyncWorkTitle		GLOBWSTR 'Async Operation Sample - Running',0
AsyncSetupText		GLOBWSTR 'Starting worker and opening the progress channel...',0
AsyncRunFormat		GLOBWSTR '%ls: %d%%',0
AsyncCancelText		GLOBWSTR 'Cancel requested. Waiting for worker cleanup...',0
AsyncIndexName		GLOBWSTR 'Indexing workspace',0
AsyncRepairName	GLOBWSTR 'Repairing protected settings',0
AsyncElevatedName	GLOBWSTR 'Repairing protected settings as administrator',0
AsyncSuccessTitle	GLOBWSTR 'Async Operation Sample - Complete',0
AsyncSuccessInstruction GLOBWSTR 'The operation completed.',0
AsyncSuccessContent	GLOBWSTR 'The workflow switched from setup marquee to determinate progress, then navigated to this completion page.',0
AsyncSuccessExpanded	GLOBWSTR 'Simulated log:',10,'worker: initialized',10,'worker: processed queue',10,'worker: committed result',0
AsyncFailedTitle	GLOBWSTR 'Async Operation Sample - Failed',0
AsyncFailedInstruction GLOBWSTR 'The repair needs elevation.',0
AsyncFailedContent	GLOBWSTR 'The simulated worker reached a protected setting. The shielded command link restarts the operation in elevated mode.',0
AsyncElevateText	GLOBWSTR 'Run repair as admin',10,'Restarts the same workflow and completes successfully.',0
AsyncCancelledTitle	GLOBWSTR 'Async Operation Sample - Cancelled',0
AsyncCancelledInstruction GLOBWSTR 'The operation was cancelled.',0
AsyncCancelledContent GLOBWSTR 'The Cancel button returned S_FALSE, disabled itself, and let the timer callback navigate here after cleanup.',0

define __GLOBAL_DATA__ async_operation_data

macro async_operation_data
	TD_BUTTON_ARRAY AsyncPickButtons,\
		ASYNC_BTN_INDEX,AsyncIndexText,\
		ASYNC_BTN_REPAIR,AsyncRepairText
	TD_BUTTON_ARRAY AsyncFailedButtons,\
		ASYNC_BTN_ELEVATE,AsyncElevateText
purge async_operation_data
end macro

proc AsyncOperationSample
    locals
	td TASKDIALOG_CONTEXT
	td_progress dd ?
	td_phase dd ?
	td_cancelled dd ?
	td_mode dd ?
	td_operationName dq ?
	work TASKDIALOG_CONTEXT
	success TASKDIALOG_CONTEXT
	failed TASKDIALOG_CONTEXT
	cancelled TASKDIALOG_CONTEXT
    endl

	fastcall TaskDialogInit,addr td
	fastcall TaskDialogInit,addr work
	fastcall TaskDialogInit,addr success
	fastcall TaskDialogInit,addr failed
	fastcall TaskDialogInit,addr cancelled

	lea	rax,[td.msgOnCreate]
	mov	[work.cfg.lpCallbackData],rax
	mov	[success.cfg.lpCallbackData],rax
	mov	[failed.cfg.lpCallbackData],rax
	mov	[cancelled.cfg.lpCallbackData],rax

	td_set_ptr td.cfg.pszWindowTitle,AsyncTitle
	td_set_ptr td.cfg.pszMainInstruction,AsyncPickInstruction
	td_set_ptr td.cfg.pszContent,AsyncPickContent
	or	dword [td.cfg.dwFlags],TDF_USE_COMMAND_LINKS
	lea	rax,[AsyncPickButtons]
	mov	[td.cfg.pButtons],rax
	mov	dword [td.cfg.cButtons],AsyncPickButtons.count
	td_set_handler td.msgOnConstructed,PickOnConstructed
	td_set_handler td.msgOnButton,PickOnButton

	td_set_ptr work.cfg.pszWindowTitle,AsyncWorkTitle
	td_set_ptr work.cfg.pszMainInstruction,AsyncSetupText
	or	dword [work.cfg.dwFlags],\
		TDF_SHOW_MARQUEE_PROGRESS_BAR or TDF_CALLBACK_TIMER
	mov	dword [work.cfg.dwCommonButtons],TDCBF_CANCEL_BUTTON

	td_set_ptr success.cfg.pszWindowTitle,AsyncSuccessTitle
	td_set_ptr success.cfg.pszMainInstruction,AsyncSuccessInstruction
	td_set_ptr success.cfg.pszContent,AsyncSuccessContent
	td_set_ptr success.cfg.pszExpandedInformation,AsyncSuccessExpanded
	mov	dword [success.cfg.dwCommonButtons],TDCBF_CLOSE_BUTTON

	td_set_ptr failed.cfg.pszWindowTitle,AsyncFailedTitle
	td_set_ptr failed.cfg.pszMainInstruction,AsyncFailedInstruction
	td_set_ptr failed.cfg.pszContent,AsyncFailedContent
	or	dword [failed.cfg.dwFlags],TDF_USE_COMMAND_LINKS
	lea	rax,[AsyncFailedButtons]
	mov	[failed.cfg.pButtons],rax
	mov	dword [failed.cfg.cButtons],AsyncFailedButtons.count
	mov	dword [failed.cfg.dwCommonButtons],TDCBF_CLOSE_BUTTON

	td_set_ptr cancelled.cfg.pszWindowTitle,AsyncCancelledTitle
	td_set_ptr cancelled.cfg.pszMainInstruction,AsyncCancelledInstruction
	td_set_ptr cancelled.cfg.pszContent,AsyncCancelledContent
	mov	dword [cancelled.cfg.dwCommonButtons],TDCBF_CLOSE_BUTTON

	invoke	TaskDialogIndirect,addr td.cfg,0,0,0
	ret

PickOnConstructed:
	enter	32,0
	invoke	SendMessageW,rcx,TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE,\
		ASYNC_BTN_REPAIR,TRUE
	leave
	xor	eax,eax
	retn

PickOnButton:
	cmp	r8d,ASYNC_BTN_INDEX
	je	.index
	cmp	r8d,ASYNC_BTN_REPAIR
	je	.repair
	xor	eax,eax
	retn
  .index:
	mov	r10d,ASYNC_MODE_INDEX
	lea	r11,[AsyncIndexName]
	jmp	.start
  .repair:
	mov	r10d,ASYNC_MODE_REPAIR
	lea	r11,[AsyncRepairName]
  .start:
	td_local td_progress,.progress,rbx
	td_local td_phase,.phase,rbx
	td_local td_cancelled,.cancelled,rbx
	td_local td_mode,.mode,rbx
	td_local td_operationName,.operationName,rbx
	td_local work,.work,rbx
	td_local td.msgOnButton,.msgOnButton,rbx
	td_local td.msgOnTimer,.msgOnTimer,rbx
	td_local td.msgOnNavigate,.msgOnNavigate,rbx
	mov	dword [.progress],0
	mov	dword [.phase],ASYNC_PHASE_SETUP
	mov	dword [.cancelled],0
	mov	[.mode],r10d
	mov	[.operationName],r11
	lea	r10,[WorkOnButton]
	mov	[.msgOnButton],r10
	lea	r10,[WorkOnTimer]
	mov	[.msgOnTimer],r10
	lea	r10,[WorkOnNavigate]
	mov	[.msgOnNavigate],r10
	enter	32,0
	invoke	SendMessageW,rcx,TDM_NAVIGATE_PAGE,0,addr .work
	leave
	mov	eax,S_FALSE
	retn

WorkOnNavigate:
	enter	32,0
	invoke	SendMessageW,rcx,TDM_SET_MARQUEE_PROGRESS_BAR,TRUE,0
	invoke	SendMessageW,rcx,TDM_SET_PROGRESS_BAR_MARQUEE,TRUE,25
	invoke	SendMessageW,rcx,TDM_ENABLE_BUTTON,IDCANCEL,TRUE
	leave
	xor	eax,eax
	retn

WorkOnButton:
	cmp	r8d,IDCANCEL
	je	.cancel
	xor	eax,eax
	retn
  .cancel:
	td_local td_phase,.phase,rbx
	td_local td_cancelled,.cancelled,rbx
	mov	dword [.phase],ASYNC_PHASE_CANCEL
	mov	dword [.cancelled],1
	enter	32,0
	invoke	SendMessageW,rcx,TDM_ENABLE_BUTTON,IDCANCEL,FALSE
	invoke	SendMessageW,rcx,TDM_SET_ELEMENT_TEXT,TDE_MAIN_INSTRUCTION,AsyncCancelText
	leave
	mov	eax,S_FALSE
	retn

WorkOnTimer:
	virtual at rbp - .frame
			rq 4
		.hTDlg	dq ?
		label .buffer:160
			rw sizeof .buffer
			align.assume rbp,16
			align 16
		.frame := $ - $$
	end virtual
	enter	.frame,0
	mov	[.hTDlg],rcx
	td_local td_progress,.progress,rbx
	td_local td_phase,.phase,rbx
	td_local td_mode,.mode,rbx
	td_local td_operationName,.operationName,rbx
	td_local success,.success,rbx
	td_local failed,.failedPage,rbx
	td_local cancelled,.cancelledPage,rbx
	td_local td.msgOnButton,.msgOnButton,rbx
	td_local td.msgOnTimer,.msgOnTimer,rbx
	td_local td.msgOnNavigate,.msgOnNavigate,rbx

	cmp	dword [.phase],ASYNC_PHASE_CANCEL
	je	.cancelled
	cmp	dword [.phase],ASYNC_PHASE_RUN
	je	.run

	mov	dword [.phase],ASYNC_PHASE_RUN
	invoke	SendMessageW,[.hTDlg],TDM_SET_MARQUEE_PROGRESS_BAR,FALSE,0
	invoke	SendMessageW,[.hTDlg],TDM_SET_PROGRESS_BAR_RANGE,0,100 shl 16
	jmp	.update_text

  .run:
	add	dword [.progress],7
	cmp	dword [.progress],100
	jb	.update_text
	mov	dword [.progress],100
	cmp	dword [.mode],ASYNC_MODE_REPAIR
	je	.fail_page
	lea	r10,[ResultOnButton]
	mov	[.msgOnButton],r10
	lea	r10,[TaskDialogCallback.S_OK]
	mov	[.msgOnTimer],r10
	mov	[.msgOnNavigate],r10
	invoke	SendMessageW,[.hTDlg],TDM_NAVIGATE_PAGE,0,addr .success
	jmp	.done

  .fail_page:
	lea	r10,[FailedOnButton]
	mov	[.msgOnButton],r10
	lea	r10,[TaskDialogCallback.S_OK]
	mov	[.msgOnTimer],r10
	lea	r10,[FailedOnNavigate]
	mov	[.msgOnNavigate],r10
	invoke	SendMessageW,[.hTDlg],TDM_NAVIGATE_PAGE,0,addr .failedPage
	jmp	.done

  .cancelled:
	lea	r10,[ResultOnButton]
	mov	[.msgOnButton],r10
	lea	r10,[TaskDialogCallback.S_OK]
	mov	[.msgOnTimer],r10
	mov	[.msgOnNavigate],r10
	invoke	SendMessageW,[.hTDlg],TDM_NAVIGATE_PAGE,0,addr .cancelledPage
	jmp	.done

  .update_text:
	invoke	wsprintfW,addr .buffer,AsyncRunFormat,[.operationName],dword [.progress]
	invoke	SendMessageW,[.hTDlg],TDM_SET_ELEMENT_TEXT,TDE_MAIN_INSTRUCTION,addr .buffer
	invoke	SendMessageW,[.hTDlg],TDM_SET_PROGRESS_BAR_POS,dword [.progress],0
  .done:
	leave
	xor	eax,eax
	retn

FailedOnNavigate:
	enter	32,0
	invoke	SendMessageW,rcx,TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE,\
		ASYNC_BTN_ELEVATE,TRUE
	leave
	xor	eax,eax
	retn

FailedOnButton:
	cmp	r8d,ASYNC_BTN_ELEVATE
	je	.elevate
	xor	eax,eax
	retn
  .elevate:
	td_local td_progress,.progress,rbx
	td_local td_phase,.phase,rbx
	td_local td_cancelled,.cancelled,rbx
	td_local td_mode,.mode,rbx
	td_local td_operationName,.operationName,rbx
	td_local work,.work,rbx
	td_local td.msgOnButton,.msgOnButton,rbx
	td_local td.msgOnTimer,.msgOnTimer,rbx
	td_local td.msgOnNavigate,.msgOnNavigate,rbx
	mov	dword [.progress],0
	mov	dword [.phase],ASYNC_PHASE_SETUP
	mov	dword [.cancelled],0
	mov	dword [.mode],ASYNC_MODE_ELEVATED
	lea	r10,[AsyncElevatedName]
	mov	[.operationName],r10
	lea	r10,[WorkOnButton]
	mov	[.msgOnButton],r10
	lea	r10,[WorkOnTimer]
	mov	[.msgOnTimer],r10
	lea	r10,[WorkOnNavigate]
	mov	[.msgOnNavigate],r10
	enter	32,0
	invoke	SendMessageW,rcx,TDM_NAVIGATE_PAGE,0,addr .work
	leave
	mov	eax,S_FALSE
	retn

ResultOnButton:
	xor	eax,eax
	retn
endp
