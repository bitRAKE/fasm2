; ui_main.inc - window procedure, edit subclass, and message-loop routing.

proc HandleCommandChar hwnd,ch
	cmp	edx,'v'
	je	.toggle
	cmp	edx,'V'
	je	.toggle
	cmp	edx,'q'
	je	.quit
	cmp	edx,'Q'
	je	.quit
	xor	eax,eax
	ret
  .toggle:
	fastcall ToggleNoiseFilter
	fastcall UpdateFilterMenuChecks
	mov	eax,1
	ret
  .quit:
	invoke	DestroyWindow,rcx
	mov	eax,1
	ret
endp

proc EditProc hwnd,wmsg,wparam,lparam
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	cmp	edx,WM_CHAR
	je	.wm_char
	cmp	edx,WM_PASTE
	je	.handled
	cmp	edx,WM_CUT
	je	.handled
	cmp	edx,WM_CLEAR
	je	.handled
	cmp	edx,WM_UNDO
	je	.handled
	cmp	edx,WM_KEYDOWN
	jne	.default
	cmp	r8d,VK_DELETE
	je	.handled
	cmp	r8d,VK_BACK
	je	.handled
	jmp	.default

  .wm_char:
	fastcall HandleCommandChar,[hMain],dword [wparam]
	test	eax,eax
	jnz	.handled
	jmp	.handled

  .default:
	invoke	CallWindowProc,[oldEditProc],[hwnd],dword [wmsg],[wparam],[lparam]
	ret
  .handled:
	xor	eax,eax
	ret
endp

proc ResizeEditor
	cmp	qword [hEdit],0
	je	.done
	invoke	GetClientRect,[hMain],addr client_rect
	invoke	MoveWindow,[hEdit],0,0,\
		dword [client_rect.right],dword [client_rect.bottom],1
  .done:
	ret
endp

proc CreateLogEdit hwnd
	mov	[hwnd],rcx
	invoke	CreateWindowEx,WS_EX_CLIENTEDGE,'EDIT','',\
		WS_CHILD or WS_VISIBLE or WS_VSCROLL or WS_HSCROLL or WS_TABSTOP or\
		ES_MULTILINE or ES_AUTOVSCROLL or ES_AUTOHSCROLL or ES_NOHIDESEL or\
		ES_WANTRETURN,\
		0,0,0,0,[hwnd],ID_EDIT_CONTROL,[hInstance],0
	mov	[hEdit],rax
	test	rax,rax
	jz	.done
	fastcall InitLogEdit
	invoke	SetWindowLongPtr,[hEdit],GWLP_WNDPROC,addr EditProc
	mov	[oldEditProc],rax
	fastcall ResizeEditor
  .done:
	ret
endp

proc WindowProc hwnd,wmsg,wparam,lparam
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	cmp	edx,WM_CREATE
	je	.wm_create

	fastcall LogMessage,dword [wmsg],[wparam],[lparam]

	mov	eax,dword [msgTaskbar]
	test	eax,eax
	jz	.route_known_messages
	cmp	dword [wmsg],eax
	je	.wm_taskbarcreated

  .route_known_messages:
	cmp	dword [wmsg],WM_SIZE
	je	.wm_size
	cmp	dword [wmsg],WM_SETFOCUS
	je	.wm_setfocus
	cmp	dword [wmsg],WM_MOUSEMOVE
	je	.wm_mousemove
	cmp	dword [wmsg],WM_INPUT
	je	.wm_input
	cmp	dword [wmsg],WM_CHAR
	je	.wm_char
	cmp	dword [wmsg],WM_COMMAND
	je	.wm_command
	cmp	dword [wmsg],WM_HOTKEY
	je	.wm_hotkey
	cmp	dword [wmsg],WM_DROPFILES
	je	.wm_dropfiles
	cmp	dword [wmsg],WM_DESTROY
	je	.wm_destroy

	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_create:
	mov	rax,[hwnd]
	mov	[hMain],rax
	fastcall CreateLogEdit,[hwnd]
	fastcall BuildMenus,[hwnd]
	fastcall ResizeEditor
	fastcall WireSources,[hwnd]
	fastcall LogMessage,dword [wmsg],[wparam],[lparam]
	fastcall LogText,<'msgflood running. Newest messages are inserted at the top.',13,10,\
		'Try moving, resizing, dragging a file, copying text, Ctrl+Alt+M, or toggling the noise filter with V.',13,10>
	xor	eax,eax
	ret

  .wm_size:
	fastcall ResizeEditor
	xor	eax,eax
	ret

  .wm_setfocus:
	cmp	qword [hEdit],0
	je	.focus_done
	invoke	SetFocus,[hEdit]
  .focus_done:
	xor	eax,eax
	ret

  .wm_mousemove:
	fastcall RearmMouseTracking,[hwnd]
	xor	eax,eax
	ret

  .wm_taskbarcreated:
	fastcall SetupTrayIcon,[hwnd]
	xor	eax,eax
	ret

  .wm_input:
	fastcall DrainRawInput,[lparam]
	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_char:
	fastcall HandleCommandChar,[hwnd],dword [wparam]
	xor	eax,eax
	ret

  .wm_command:
	mov	eax,dword [wparam]
	movzx	edx,ax
	fastcall HandleMenuCommand,[hwnd],edx
	test	eax,eax
	jnz	.command_done
	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret
  .command_done:
	xor	eax,eax
	ret

  .wm_hotkey:
	cmp	dword [wparam],ID_HOTKEY
	jne	.hotkey_done
	fastcall LogText,<'    -> Ctrl+Alt+M hotkey received',13,10>
  .hotkey_done:
	xor	eax,eax
	ret

  .wm_dropfiles:
	invoke	DragQueryFile,[wparam],-1,0,0
	fastcall LogDropCount,eax
	invoke	DragFinish,[wparam]
	xor	eax,eax
	ret

  .wm_destroy:
	fastcall CleanupSources,[hwnd]
	invoke	PostQuitMessage,0
	xor	eax,eax
	ret
endp
