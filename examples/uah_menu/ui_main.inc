proc HandleCommand hwnd,command
	mov	[hwnd],rcx
	mov	dword [command],edx
	cmp	edx,ID_FILE_EXIT
	je	.exit
	cmp	edx,ID_VIEW_SHOW
	je	.show
	cmp	edx,ID_VIEW_HIDE
	je	.hide
	cmp	edx,ID_THEME_TOGGLE
	je	.theme
	cmp	edx,ID_TEXT_CLEAR
	je	.clear
	cmp	edx,ID_INSERT_SAMPLE
	je	.sample
	cmp	edx,ID_HELP_ABOUT
	je	.about
	cmp	edx,ID_TEST_DIRECT_LOAD
	jb	.maybe_edit
	cmp	edx,ID_TEST_REAPPLY_PROOFING
	ja	.maybe_edit
	fastcall RichExecuteTestCommand,dword [command]
	jmp	.handled
  .maybe_edit:
	cmp	edx,ID_EDIT_UNDO
	jb	.unhandled
	cmp	edx,ID_EDIT_REPROOF
	ja	.unhandled
	fastcall RichExecuteEditCommand,dword [command]
	jmp	.handled

  .exit:
	invoke	DestroyWindow,[hwnd]
	jmp	.handled
  .show:
	invoke	ShowWindow,[hwnd],SW_RESTORE
	invoke	SetForegroundWindow,[hwnd]
	jmp	.handled
  .hide:
	invoke	ShowWindow,[hwnd],SW_HIDE
	jmp	.handled
  .theme:
	fastcall ToggleTheme
	jmp	.handled
  .clear:
	fastcall RichClear
	jmp	.handled
  .sample:
	fastcall RichInsertSample
	jmp	.handled
  .about:
	invoke	MessageBox,[hwnd],\
		'Thread CBT hook subclasses #32768 popup menu windows, then this example paints native menus through WM_UAH* while hosting a configured RichEdit control and tray icon.',\
		'UAH Menu RichEdit',MB_ICONINFORMATION
  .handled:
	mov	eax,1
	ret
  .unhandled:
	xor	eax,eax
	ret
endp

proc WindowProc hwnd,wmsg,wparam,lparam
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	mov	eax,dword [msgTaskbar]
	test	eax,eax
	jz	.route
	cmp	dword [wmsg],eax
	je	.wm_taskbarcreated

  .route:
	cmp	edx,WM_CREATE
	je	.wm_create
	cmp	edx,WM_SIZE
	je	.wm_size
	cmp	edx,WM_SETFOCUS
	je	.wm_setfocus
	cmp	edx,WM_INITMENU
	je	.wm_initmenu
	cmp	edx,WM_INITMENUPOPUP
	je	.wm_initmenu
	cmp	edx,WM_COMMAND
	je	.wm_command
	cmp	edx,WM_TIMER
	je	.wm_timer
	cmp	edx,WM_CONTEXTMENU
	je	.wm_contextmenu
	cmp	edx,WM_TRAY_CALLBACK
	je	.wm_tray
	cmp	edx,WM_CLOSE
	je	.wm_close
	cmp	edx,WM_DESTROY
	je	.wm_destroy

	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_create:
	mov	rax,[hwnd]
	mov	[hMain],rax
	invoke	RegisterWindowMessage,'TaskbarCreated'
	mov	[msgTaskbar],eax
	fastcall UahOpen
	fastcall ApplyMenuAppMode
	fastcall CreateRichEdit,[hwnd]
	test	eax,eax
	jz	.create_fail
	fastcall SetupTrayIcon,[hwnd]
	fastcall UahMenuHook_Install,[hwnd],addr MenuPopupSubclassProc,\
		UAH_MENU_DEFAULT_SUBCLASS_ID,dword [dark_mode]
	xor	eax,eax
	ret
  .create_fail:
	mov	eax,-1
	ret

  .wm_size:
	fastcall ResizeRich
	xor	eax,eax
	ret

  .wm_setfocus:
	cmp	qword [hRich],0
	je	.focus_done
	invoke	SetFocus,[hRich]
  .focus_done:
	xor	eax,eax
	ret

  .wm_initmenu:
	fastcall UpdatePopupMenuState,[wparam]
	xor	eax,eax
	ret

  .wm_command:
	mov	eax,dword [wparam]
	movzx	edx,ax
	fastcall HandleCommand,[hwnd],edx
	test	eax,eax
	jnz	.command_done
	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret
  .command_done:
	xor	eax,eax
	ret

  .wm_timer:
	cmp	qword [wparam],RICH_WAKE_START_TIMER_ID
	je	.timer_start_wake
	cmp	qword [wparam],RICH_WAKE_FINISH_TIMER_ID
	jne	.timer_default
	invoke	KillTimer,[hwnd],RICH_WAKE_FINISH_TIMER_ID
	fastcall RichFinishQueuedWake
	xor	eax,eax
	ret
  .timer_start_wake:
	invoke	KillTimer,[hwnd],RICH_WAKE_START_TIMER_ID
	fastcall RichQueueInputWake
	xor	eax,eax
	ret
  .timer_default:
	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_contextmenu:
	mov	rax,[wparam]
	cmp	rax,[hRich]
	jne	.context_default
	cmp	qword [lparam],-1
	je	.context_keyboard
	movsx	ecx,word [lparam]
	movsx	edx,word [lparam+2]
	fastcall ShowEditContextMenu,ecx,edx
	xor	eax,eax
	ret
  .context_keyboard:
	fastcall ShowEditContextMenu,-1,-1
	xor	eax,eax
	ret
  .context_default:
	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_tray:
	fastcall HandleTrayCallback,[hwnd],[wparam],[lparam]
	test	eax,eax
	jnz	.tray_done
	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret
  .tray_done:
	xor	eax,eax
	ret

  .wm_taskbarcreated:
	fastcall SetupTrayIcon,[hwnd]
	xor	eax,eax
	ret

  .wm_close:
	invoke	ShowWindow,[hwnd],SW_HIDE
	xor	eax,eax
	ret

  .wm_destroy:
	fastcall CleanupTrayIcon
	fastcall UahMenuHook_Uninstall
	fastcall UahClose
	fastcall DeleteMenuFonts
	fastcall CleanupRichEdit
	invoke	PostQuitMessage,0
	xor	eax,eax
	ret
endp
