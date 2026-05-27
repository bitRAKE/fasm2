; TaskDialog.asm - modular Win64 TaskDialog samples.

include 'windows.inc'
include 'taskdialog.inc'

BUTTON_COMMONBUTTONS		= 100h
BUTTON_COUNTER			= 101h
BUTTON_ELEVATION		= 102h
BUTTON_ENABLEDISABLE		= 103h
BUTTON_ERROR			= 104h
BUTTON_ICONS			= 105h
BUTTON_PROGRESS			= 106h
BUTTON_PROGRESSEFFECTS		= 107h
BUTTON_TIMER			= 108h
BUTTON_UPDATETEXT		= 109h
BUTTON_ASYNCOPERATION		= 10Ah
BUTTON_FIRST			= BUTTON_COMMONBUTTONS

AppTitle		GLOBWSTR 'TaskDialog Samples (originally by Kenny Kerr)',0
AppInstruction		GLOBWSTR 'Pick a sample to try:',0
AppContent		GLOBWSTR 'Use the dialog close button, ESC, or Alt-F4 to exit a sample.',0
AppFooter		GLOBWSTR 'x64 fasm2 port by <a href="https://github.com/bitRAKE">bitRAKE</a>.',0

SampleCommonText	GLOBWSTR 'Common Buttons Sample',0
SampleCounterText	GLOBWSTR 'Counter Sample',0
SampleElevationText	GLOBWSTR 'Elevation Required Sample',0
SampleEnableText	GLOBWSTR 'Enable/Disable Sample',0
SampleErrorText		GLOBWSTR 'Error Sample',0
SampleIconsText		GLOBWSTR 'Icons Sample',0
SampleProgressText	GLOBWSTR 'Progress Sample',0
SampleProgressFxText	GLOBWSTR 'Progress Effects Sample',0
SampleTimerText		GLOBWSTR 'Timer Sample',0
SampleUpdateText	GLOBWSTR 'Update Text Sample',0
SampleAsyncText		GLOBWSTR 'Async Operation Sample',0

include 'samples/CommonButton.inc'
include 'samples/Counter.inc'
include 'samples/Elevation.inc'
include 'samples/EnableDisable.inc'
include 'samples/Error.inc'
include 'samples/Icons.inc'
include 'samples/Progress.inc'
include 'samples/ProgressEffects.inc'
include 'samples/Timer.inc'
include 'samples/UpdateText.inc'
include 'samples/AsyncOperation.inc'

define __GLOBAL_DATA__ main_dialog_data

macro main_dialog_data
	align 8
	SampleTable:
		dq CommonButtonsSample
		dq CounterSample
		dq ElevationSample
		dq EnableDisableSample
		dq ErrorSample
		dq IconsSample
		dq ProgressSample
		dq ProgressEffectsSample
		dq TimerSample
		dq UpdateTextSample
		dq AsyncOperationSample
	SampleTable.count = ($ - SampleTable) shr 3

	TD_BUTTON_ARRAY SampleButtons,\
		BUTTON_COMMONBUTTONS,SampleCommonText,\
		BUTTON_COUNTER,SampleCounterText,\
		BUTTON_ELEVATION,SampleElevationText,\
		BUTTON_ENABLEDISABLE,SampleEnableText,\
		BUTTON_ERROR,SampleErrorText,\
		BUTTON_ICONS,SampleIconsText,\
		BUTTON_PROGRESS,SampleProgressText,\
		BUTTON_PROGRESSEFFECTS,SampleProgressFxText,\
		BUTTON_TIMER,SampleTimerText,\
		BUTTON_UPDATETEXT,SampleUpdateText,\
		BUTTON_ASYNCOPERATION,SampleAsyncText
purge main_dialog_data
end macro

proc start
	invoke	GetModuleHandleW,0
	mov	[hInstance],rax
	mov	qword [hOwner],0

	fastcall ShowSamplePicker
	invoke	ExitProcess,eax
endp

proc ShowSamplePicker
    locals
	td TASKDIALOG_CONTEXT
	selectedButtonId dd ?
	selectedRadioButtonId dd ?
	verificationChecked dd ?
    endl

	fastcall TaskDialogInit,addr td
	td_set_ptr td.cfg.pszWindowTitle,AppTitle
	td_set_ptr td.cfg.pszMainInstruction,AppInstruction
	td_set_ptr td.cfg.pszContent,AppContent
	td_set_ptr td.cfg.pszFooter,AppFooter
	or	dword [td.cfg.dwFlags],\
		TDF_USE_COMMAND_LINKS or TDF_CAN_BE_MINIMIZED or TDF_ENABLE_HYPERLINKS
	lea	rax,[SampleButtons]
	mov	[td.cfg.pButtons],rax
	mov	dword [td.cfg.cButtons],SampleButtons.count

	td_set_handler td.msgOnConstructed,OnConstructed
	td_set_handler td.msgOnHyperlink,OnHyperlink
	td_set_handler td.msgOnButton,OnButton

	invoke	TaskDialogIndirect,addr td.cfg,addr selectedButtonId,\
		addr selectedRadioButtonId,addr verificationChecked
	ret

OnConstructed:
	mov	[hOwner],rcx
	xor	eax,eax
	retn

OnButton:
	mov	r10d,r8d
	sub	r10d,BUTTON_FIRST
	cmp	r10d,SampleTable.count
	jae	.close
	enter	32,0
	lea	rax,[SampleTable]
	call	qword [rax+r10*8]
	leave
	test	eax,eax
	jnz	.close
	mov	eax,S_FALSE
	retn
  .close:
	xor	eax,eax
	retn

OnHyperlink:
	enter	32,0
	invoke	ShellExecuteW,rcx,'Open',r9,0,0,SW_SHOWNORMAL
	leave
	xor	eax,eax
	retn
endp
