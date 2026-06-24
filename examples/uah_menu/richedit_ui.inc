define __GLOBAL_DATA__ rich_edit_data

struct GETTEXTLENGTHEX
	flags dd ?
	codepage dd ?
ends

struct INPUT
	type dd ?
	dd ?
	wVk dw ?
	wScan dw ?
	dwFlags dd ?
	time dd ?
	dd ?
	dwExtraInfo dq ?
	rb 8
ends

macro rich_edit_data
	; High Levenshtein-candidate-density seed for RichEdit proofing popups.
	proofing_sample_text du 'Yesturday our teh comittee reccomended that we definately seperate ',\
		'the goverment enviroment from the privite sector, wich occured becuase ',\
		'noone beleived it was neccessary to acheive a wierd concensus. I recieved ',\
		'thier embarassing memo on the occassion of the aniversary, and it was truely ',\
		'unforgetable — a maintenence nightmare no buisness could of forseen. The ',\
		'supercalifragilisticexpialidoshus paperwork, riddled with ',\
		'pneumonoultramicroscopicsilicovolcanoconiosiss diagnoses and ',\
		'antidisestablishmentarianisim slogans, left every proffesional flabbergasted ',\
	'by its sheer floccinaucinihilipilificaton.',13,10,0
	rich_wake_saved_sel CHARRANGE
	rich_wake_pending dd ?
	rich_wake_inputs rb sizeof.INPUT * RICH_QUEUED_WAKE_INPUTS
purge rich_edit_data
end macro

proc ConfigureRichEdit
	; EM_SETTEXTMODE is only accepted while the control has no text.
	mov	eax,TM_RICHTEXT or TM_MULTILEVELUNDO or TM_MULTICODEPAGE
	invoke	SendMessage,[hRich],EM_SETTEXTMODE,eax,0

	mov	eax,TO_ADVANCEDTYPOGRAPHY or TO_ADVANCEDLAYOUT
	invoke	SendMessage,[hRich],EM_SETTYPOGRAPHYOPTIONS,eax,eax

	fastcall RichEnableProofing

	mov	eax,SES_EXTENDBACKCOLOR or SES_USECTF or\
		SES_CTFALLOWEMBED or SES_CTFALLOWSMARTTAG or SES_CTFALLOWPROOFING or\
		SES_HYPERLINKTOOLTIPS or SES_NOFOCUSLINKNOTIFY or\
		SES_WORDDRAGDROP or SES_SMARTDRAGDROP
	invoke	SendMessage,[hRich],EM_SETEDITSTYLE,eax,eax
	invoke	SendMessage,[hRich],EM_SETEDITSTYLEEX,\
		SES_EX_HANDLEFRIENDLYURL,SES_EX_HANDLEFRIENDLYURL

	mov	eax,ECO_AUTOWORDSELECTION or ECO_NOHIDESEL or ECO_SAVESEL or ECO_SELECTIONBAR
	invoke	SendMessage,[hRich],EM_SETOPTIONS,ECOOP_OR,eax

	invoke	SendMessage,[hRich],EM_SETUNDOLIMIT,512,0

	invoke	SendMessage,[hRich],EM_AUTOURLDETECT,AURL_ENABLEURL,\
		"callto:file:ftp:gopher:http:https:mailto:news:notes:nntp:onenote:outlook:prospero:tel:telnet:wais:webcal:"

	mov	eax,ENM_CHANGE or ENM_UPDATE or ENM_SCROLL or ENM_SCROLLEVENTS or\
		ENM_DRAGDROPDONE or ENM_KEYEVENTS or ENM_MOUSEEVENTS or\
		ENM_REQUESTRESIZE or ENM_SELCHANGE or ENM_DROPFILES or ENM_LINK
	invoke	SendMessage,[hRich],EM_SETEVENTMASK,0,eax

	ret
endp

proc RichEnableProofing
	cmp	qword [hRich],0
	je	.done
	invoke	SendMessage,[hRich],EM_GETLANGOPTIONS,0,0
	or	eax,RICH_LANG_OPTIONS
	invoke	SendMessage,[hRich],EM_SETLANGOPTIONS,0,eax
  .done:
	ret
endp

proc ApplyRichTheme
    locals
	cf CHARFORMATW
	color dd ?
    endl
	cmp	qword [hRich],0
	je	.done
	invoke	RtlZeroMemory,addr cf,sizeof.CHARFORMATW
	fastcall ThemeText
	mov	[cf.cbSize],sizeof.CHARFORMATW
	mov	[cf.dwMask],CFM_COLOR
	mov	[cf.crTextColor],eax
	invoke	SendMessage,[hRich],EM_SETCHARFORMAT,SCF_ALL,addr cf
	fastcall ThemeBg
	invoke	SendMessage,[hRich],EM_SETBKGNDCOLOR,0,eax
	invoke	InvalidateRect,[hRich],0,1
  .done:
	ret
endp

proc ResizeRich
	cmp	qword [hRich],0
	je	.done
	invoke	GetClientRect,[hMain],addr client_rect
	invoke	MoveWindow,[hRich],0,0,\
		dword [client_rect.right],dword [client_rect.bottom],1
  .done:
	ret
endp

proc CreateRichEdit hwnd
	mov	[hwnd],rcx
	invoke	LoadLibrary,'msftedit.dll'
	mov	[hMsftEdit],rax
	test	rax,rax
	jz	.fail

	invoke	CreateWindowEx,WS_EX_ACCEPTFILES,'RICHEDIT50W',0,\
		RICHEDIT_STYLE,\
		0,0,0,0,[hwnd],ID_RICHEDIT,[hInstance],0
	mov	[hRich],rax
	test	rax,rax
	jz	.fail
	fastcall ConfigureRichEdit
	fastcall ApplyRichTheme
	fastcall ResizeRich
	fastcall RichInsertSample
	mov	eax,1
	ret

  .fail:
	xor	eax,eax
	ret
endp

proc CleanupRichEdit
	mov	qword [hRich],0
	mov	rcx,[hMsftEdit]
	jrcxz	.done
	invoke	FreeLibrary,rcx
	mov	qword [hMsftEdit],0
  .done:
	ret
endp

proc RichAppendText textp
    locals
	length dd ?
    endl
	mov	[textp],rcx
	cmp	qword [hRich],0
	je	.done
	invoke	GetWindowTextLength,[hRich]
	mov	[length],eax
	invoke	SendMessage,[hRich],EM_SETSEL,dword [length],dword [length]
	invoke	SendMessage,[hRich],EM_REPLACESEL,1,[textp]
  .done:
	ret
endp

proc RichAppendProofingSample
	fastcall RichAppendText,addr proofing_sample_text
	ret
endp

proc RichInsertSample
	cmp	qword [hRich],0
	je	.done
	invoke	GetWindowTextLength,[hRich]
	test	eax,eax
	jz	.sample
	fastcall RichAppendText,<13,10,13,10>
  .sample:
	fastcall RichAppendProofingSample
  .done:
	ret
endp

proc RichInsertLink
	fastcall RichAppendText,<13,10,'https://flatassembler.net/',13,10>
	ret
endp

proc RichClear
	cmp	qword [hRich],0
	je	.done
	invoke	SetWindowTextW,[hRich],''
	fastcall ApplyRichTheme
  .done:
	ret
endp

proc RichLoadFixtureDirect
	fastcall RichClear
	fastcall RichInsertSample
	ret
endp

proc RichFocusForeground
	cmp	qword [hRich],0
	je	.done
	invoke	ShowWindow,[hMain],SW_RESTORE
	invoke	SetForegroundWindow,[hMain]
	invoke	SetFocus,[hRich]
  .done:
	ret
endp

proc RichAttachForegroundFocus
    locals
	me dd ?
	foreground_thread dd ?
	attached dd ?
	foreground dq ?
    endl
	mov	dword [attached],0
	cmp	qword [hRich],0
	je	.done
	invoke	GetCurrentThreadId
	mov	[me],eax
	invoke	GetForegroundWindow
	mov	[foreground],rax
	test	rax,rax
	jz	.focus
	invoke	GetWindowThreadProcessId,[foreground],0
	mov	[foreground_thread],eax
	cmp	eax,[me]
	je	.focus
	invoke	AttachThreadInput,dword [me],dword [foreground_thread],1
	mov	[attached],eax

  .focus:
	invoke	ShowWindow,[hMain],SW_RESTORE
	invoke	SetForegroundWindow,[hMain]
	invoke	SetFocus,[hRich]
	cmp	dword [attached],0
	je	.done
	invoke	AttachThreadInput,dword [me],dword [foreground_thread],0
  .done:
	ret
endp

proc RichOpenCtfStatus
	cmp	qword [hRich],0
	je	.done
	fastcall RichAttachForegroundFocus
	invoke	SendMessage,[hRich],EM_SETCTFOPENSTATUS,1,0
  .done:
	ret
endp

proc RichWakeProofer
    locals
	save CHARRANGE
	tail CHARRANGE
	gtl GETTEXTLENGTHEX
    endl
	cmp	qword [hRich],0
	je	.done
	invoke	SendMessage,[hRich],EM_EXGETSEL,0,addr save
	mov	[gtl.flags],GTL_NUMCHARS
	mov	[gtl.codepage],RICHEDIT_CODEPAGE_UNICODE
	invoke	SendMessage,[hRich],EM_GETTEXTLENGTHEX,addr gtl,0
	mov	[tail.cpMin],eax
	mov	[tail.cpMax],eax
	invoke	SendMessage,[hRich],EM_EXSETSEL,0,addr tail
	invoke	SendMessage,[hRich],WM_CHAR,RICH_PROOFER_WAKE_CHAR,0
	invoke	SendMessage,[hRich],WM_CHAR,VK_BACK,0
	invoke	SendMessage,[hRich],EM_EXSETSEL,0,addr save
	invoke	SendMessage,[hRich],EM_EMPTYUNDOBUFFER,0,0
  .done:
	ret
endp

proc RichFinishQueuedWake
	cmp	dword [rich_wake_pending],0
	je	.done
	mov	dword [rich_wake_pending],0
	cmp	qword [hRich],0
	je	.done
	invoke	SendMessage,[hRich],EM_EXSETSEL,0,addr rich_wake_saved_sel
	invoke	SendMessage,[hRich],EM_EMPTYUNDOBUFFER,0,0
  .done:
	ret
endp

proc RichQueueInputWake
    locals
	tail CHARRANGE
	gtl GETTEXTLENGTHEX
    endl
	cmp	qword [hRich],0
	je	.done
	invoke	SendMessage,[hRich],EM_EXGETSEL,0,addr rich_wake_saved_sel
	mov	[gtl.flags],GTL_NUMCHARS
	mov	[gtl.codepage],RICHEDIT_CODEPAGE_UNICODE
	invoke	SendMessage,[hRich],EM_GETTEXTLENGTHEX,addr gtl,0
	mov	[tail.cpMin],eax
	mov	[tail.cpMax],eax
	invoke	SetFocus,[hRich]
	invoke	SendMessage,[hRich],EM_EXSETSEL,0,addr tail
	invoke	RtlZeroMemory,addr rich_wake_inputs,sizeof.INPUT * RICH_QUEUED_WAKE_INPUTS
	mov	dword [rich_wake_inputs + INPUT.type],INPUT_KEYBOARD
	mov	word [rich_wake_inputs + INPUT.wVk],VK_SPACE
	mov	dword [rich_wake_inputs + sizeof.INPUT + INPUT.type],INPUT_KEYBOARD
	mov	word [rich_wake_inputs + sizeof.INPUT + INPUT.wVk],VK_SPACE
	mov	dword [rich_wake_inputs + sizeof.INPUT + INPUT.dwFlags],KEYEVENTF_KEYUP
	mov	dword [rich_wake_inputs + 2 * sizeof.INPUT + INPUT.type],INPUT_KEYBOARD
	mov	word [rich_wake_inputs + 2 * sizeof.INPUT + INPUT.wVk],VK_BACK
	mov	dword [rich_wake_inputs + 3 * sizeof.INPUT + INPUT.type],INPUT_KEYBOARD
	mov	word [rich_wake_inputs + 3 * sizeof.INPUT + INPUT.wVk],VK_BACK
	mov	dword [rich_wake_inputs + 3 * sizeof.INPUT + INPUT.dwFlags],KEYEVENTF_KEYUP
	invoke	SendInput,RICH_QUEUED_WAKE_INPUTS,addr rich_wake_inputs,sizeof.INPUT
	cmp	eax,RICH_QUEUED_WAKE_INPUTS
	jne	.restore_now
	mov	dword [rich_wake_pending],1
	invoke	SetTimer,[hMain],RICH_WAKE_FINISH_TIMER_ID,RICH_WAKE_TIMER_MS,0
	jmp	.done

  .restore_now:
	invoke	SendMessage,[hRich],EM_EXSETSEL,0,addr rich_wake_saved_sel
  .done:
	ret
endp

proc RichScheduleInputWake
	cmp	qword [hRich],0
	je	.done
	invoke	SetTimer,[hMain],RICH_WAKE_START_TIMER_ID,RICH_WAKE_TIMER_MS,0
  .done:
	ret
endp

proc RichQueueAttachedUnicodeInputWake
    locals
	tail CHARRANGE
	gtl GETTEXTLENGTHEX
    endl
	cmp	qword [hRich],0
	je	.done
	invoke	SendMessage,[hRich],EM_EXGETSEL,0,addr rich_wake_saved_sel
	mov	[gtl.flags],GTL_NUMCHARS
	mov	[gtl.codepage],RICHEDIT_CODEPAGE_UNICODE
	invoke	SendMessage,[hRich],EM_GETTEXTLENGTHEX,addr gtl,0
	mov	[tail.cpMin],eax
	mov	[tail.cpMax],eax
	fastcall RichAttachForegroundFocus
	invoke	SendMessage,[hRich],EM_EXSETSEL,0,addr tail
	invoke	RtlZeroMemory,addr rich_wake_inputs,sizeof.INPUT * RICH_QUEUED_WAKE_INPUTS
	mov	dword [rich_wake_inputs + INPUT.type],INPUT_KEYBOARD
	mov	word [rich_wake_inputs + INPUT.wScan],RICH_PROOFER_WAKE_CHAR
	mov	dword [rich_wake_inputs + INPUT.dwFlags],KEYEVENTF_UNICODE
	mov	dword [rich_wake_inputs + sizeof.INPUT + INPUT.type],INPUT_KEYBOARD
	mov	word [rich_wake_inputs + sizeof.INPUT + INPUT.wScan],RICH_PROOFER_WAKE_CHAR
	mov	dword [rich_wake_inputs + sizeof.INPUT + INPUT.dwFlags],KEYEVENTF_UNICODE or KEYEVENTF_KEYUP
	mov	dword [rich_wake_inputs + 2 * sizeof.INPUT + INPUT.type],INPUT_KEYBOARD
	mov	word [rich_wake_inputs + 2 * sizeof.INPUT + INPUT.wVk],VK_BACK
	mov	dword [rich_wake_inputs + 3 * sizeof.INPUT + INPUT.type],INPUT_KEYBOARD
	mov	word [rich_wake_inputs + 3 * sizeof.INPUT + INPUT.wVk],VK_BACK
	mov	dword [rich_wake_inputs + 3 * sizeof.INPUT + INPUT.dwFlags],KEYEVENTF_KEYUP
	invoke	SendInput,RICH_QUEUED_WAKE_INPUTS,addr rich_wake_inputs,sizeof.INPUT
	cmp	eax,RICH_QUEUED_WAKE_INPUTS
	jne	.restore_now
	mov	dword [rich_wake_pending],1
	invoke	SetTimer,[hMain],RICH_WAKE_FINISH_TIMER_ID,RICH_WAKE_TIMER_MS,0
	jmp	.done

  .restore_now:
	invoke	SendMessage,[hRich],EM_EXSETSEL,0,addr rich_wake_saved_sel
  .done:
	ret
endp

proc RichSetProofingFixtureClipboard
    locals
	bytes dq ?
	handle dq ?
	buffer dq ?
    endl
	invoke	lstrlenW,addr proofing_sample_text
	inc	eax
	shl	rax,1
	mov	[bytes],rax
	invoke	GlobalAlloc,GMEM_MOVEABLE,[bytes]
	mov	[handle],rax
	test	rax,rax
	jz	.fail
	invoke	GlobalLock,[handle]
	mov	[buffer],rax
	test	rax,rax
	jz	.free
	invoke	RtlMoveMemory,[buffer],addr proofing_sample_text,[bytes]
	invoke	GlobalUnlock,[handle]
	invoke	OpenClipboard,[hMain]
	test	eax,eax
	jz	.free
	invoke	EmptyClipboard
	test	eax,eax
	jz	.close_free
	invoke	SetClipboardData,CF_UNICODETEXT,[handle]
	test	rax,rax
	jz	.close_free
	invoke	CloseClipboard
	mov	eax,1
	ret

  .close_free:
	invoke	CloseClipboard
  .free:
	invoke	GlobalFree,[handle]
  .fail:
	xor	eax,eax
	ret
endp

proc RichLoadFixtureViaPaste
	cmp	qword [hRich],0
	je	.done
	fastcall RichSetProofingFixtureClipboard
	test	eax,eax
	jz	.done
	fastcall RichClear
	invoke	SendMessage,[hRich],WM_PASTE,0,0
  .done:
	ret
endp

proc RichRebindCtf
	cmp	qword [hRich],0
	je	.done
	invoke	SendMessage,[hRich],EM_SETEDITSTYLE,0,SES_USECTF
	invoke	SendMessage,[hRich],EM_SETEDITSTYLE,SES_USECTF,SES_USECTF
  .done:
	ret
endp

proc RichExecuteTestCommand command
	cmp	[hRich],0
	jz	.done
	iterate <function,	command_id>,\
		.direct_load,	ID_TEST_DIRECT_LOAD,\
		.direct_wake,	ID_TEST_DIRECT_LOAD_WAKE,\
		.wake,		ID_TEST_WAKE_INPUT,\
		.queued_wake,	ID_TEST_QUEUED_WAKE,\
		.delayed_wake,	ID_TEST_DELAYED_WAKE,\
		.focus_rich,	ID_TEST_FOCUS_RICH,\
		.attach_focus,	ID_TEST_ATTACH_FOCUS,\
		.ctf_open,	ID_TEST_CTF_OPEN_STATUS,\
		.attach_unicode_wake, ID_TEST_ATTACH_UNICODE_WAKE,\
		.paste_load,	ID_TEST_PASTE_LOAD,\
		.rebind_ctf,	ID_TEST_REBIND_CTF,\
		.proof_flags,	ID_TEST_REAPPLY_PROOFING

		cmp	ecx,command_id
		jz	function
	end iterate
  .done:
	ret
  .direct_load:
	fastcall RichLoadFixtureDirect
	jmp	.done
  .direct_wake:
	fastcall RichLoadFixtureDirect
	fastcall RichWakeProofer
	jmp	.done
  .wake:
	fastcall RichWakeProofer
	jmp	.done
  .queued_wake:
	fastcall RichQueueInputWake
	jmp	.done
  .delayed_wake:
	fastcall RichScheduleInputWake
	jmp	.done
  .focus_rich:
	fastcall RichFocusForeground
	jmp	.done
  .attach_focus:
	fastcall RichAttachForegroundFocus
	jmp	.done
  .ctf_open:
	fastcall RichOpenCtfStatus
	jmp	.done
  .attach_unicode_wake:
	fastcall RichQueueAttachedUnicodeInputWake
	jmp	.done
  .paste_load:
	fastcall RichLoadFixtureViaPaste
	jmp	.done
  .rebind_ctf:
	fastcall RichRebindCtf
	jmp	.done
  .proof_flags:
	fastcall RichEnableProofing
	jmp	.done
endp

proc RichExecuteEditCommand command
	cmp [hRich],0
	jz .done
	iterate <function,	command_id>,\
		.undo,		ID_EDIT_UNDO,\
		.redo,		ID_EDIT_REDO,\
		.cut,		ID_EDIT_CUT,\
		.copy,		ID_EDIT_COPY,\
		.paste,		ID_EDIT_PASTE,\
		.delete,	ID_EDIT_DELETE,\
		.select_all,	ID_EDIT_SELECT_ALL,\
		.insert_link,	ID_EDIT_INSERT_LINK,\
		.clear,		ID_EDIT_CLEAR,\
		.reproof,	ID_EDIT_REPROOF

		cmp ecx, command_id
		jz function
	end iterate
  .done:
	ret
  .undo:
	invoke	SendMessage,[hRich],EM_UNDO,0,0
	jmp	.done
  .redo:
	invoke	SendMessage,[hRich],EM_REDO,0,0
	jmp	.done
  .cut:
	invoke	SendMessage,[hRich],WM_CUT,0,0
	jmp	.done
  .copy:
	invoke	SendMessage,[hRich],WM_COPY,0,0
	jmp	.done
  .paste:
	invoke	SendMessage,[hRich],WM_PASTE,0,0
	jmp	.done
  .delete:
	invoke	SendMessage,[hRich],WM_CLEAR,0,0
	jmp	.done
  .select_all:
	invoke	SendMessage,[hRich],EM_SETSEL,0,-1
	jmp	.done
  .insert_link:
	fastcall RichInsertLink
	jmp	.done
  .reproof:
	fastcall RichEnableProofing
	jmp	.done
  .clear:
	fastcall RichClear
	jmp	.done
endp
