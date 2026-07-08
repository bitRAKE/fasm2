; 03_dwm_frame.asm - preserve DWM frame behavior for a CSD window.
;
; This builds on the WM_NCCALCSIZE + WM_NCHITTEST shape and adds
; DwmExtendFrameIntoClientArea with a one-pixel margin.

include 'addon/windows.inc'

CSD_TITLE_HEIGHT	= 52
CSD_CONTENT_PAD		= 26
CSD_EDGE_PX		= 4
CSD_RESIZE_PX		= 8

CSD_TITLE_BG		= 004E6FD8h	; RGB(216,111,78)
CSD_EDGE_COLOR		= 00545759h	; RGB(89,87,84)
CSD_TITLE_TEXT		= 00FFFFFFh
CSD_BODY_TEXT		= 00242120h
CSD_MUTED_TEXT		= 007A6D61h

define __GLOBAL_DATA__ csd_data
define __GLOBAL_BSS__ csd_bss

macro csd_data
	app_name	du 'CSD 03 - DWM frame',0
	class_name	du 'Fasm2CsdDwmFrame',0
	title_text	du '03  DWM frame extension',0
	body_text	du 'This version keeps the reclaimed client area and the WM_NCHITTEST native input routing, then calls DwmExtendFrameIntoClientArea.',13,10,13,10
			du 'A one-pixel DWM margin keeps the window registered with the compositor frame path while the application still draws the visible top band and edge.',0
	footer_text	du 'ESC closes. Drag, resize, snap, minimize, and restore remain native.',0

	align 4
	dwm_margins	MARGINS leftWidth: 1, rightWidth: 1, topHeight: 1, bottomHeight: 1

	align 8
	wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: WindowProc,\
		lpszClassName: class_name
purge csd_data
end macro

macro csd_bss
	align 8
	hInstance	dq ?
	hMain		dq ?
	hTitleBrush	dq ?
	hEdgeBrush	dq ?
	msg		MSG
	client_rect	RECT
	window_rect	RECT
purge csd_bss
end macro

proc CreatePaintObjects
	invoke	CreateSolidBrush,CSD_TITLE_BG
	mov	[hTitleBrush],rax
	invoke	CreateSolidBrush,CSD_EDGE_COLOR
	mov	[hEdgeBrush],rax
	ret
endp

proc DestroyPaintObjects
	mov	rcx,[hTitleBrush]
	jrcxz	destroy_edges
	invoke	DeleteObject,rcx
	mov	qword [hTitleBrush],0

  destroy_edges:
	mov	rcx,[hEdgeBrush]
	jrcxz	destroy_done
	invoke	DeleteObject,rcx
	mov	qword [hEdgeBrush],0

  destroy_done:
	ret
endp

proc PaintFrame uses rbx, hdc
    locals
	title_rect	RECT
	body_rect	RECT
	footer_rect	RECT
	edge_rect	RECT
	old_bk		dd ?
	old_color	dd ?
    endl

	mov	rbx,rcx

	invoke	GetClientRect,[hMain],addr client_rect
	invoke	GetSysColorBrush,COLOR_WINDOW
	invoke	FillRect,rbx,addr client_rect,rax

	invoke	CopyRect,addr title_rect,addr client_rect
	mov	eax,[title_rect.top]
	add	eax,CSD_TITLE_HEIGHT
	mov	[title_rect.bottom],eax
	invoke	FillRect,rbx,addr title_rect,[hTitleBrush]

	invoke	CopyRect,addr edge_rect,addr client_rect
	mov	eax,[edge_rect.left]
	add	eax,CSD_EDGE_PX
	mov	[edge_rect.right],eax
	invoke	FillRect,rbx,addr edge_rect,[hEdgeBrush]

	invoke	CopyRect,addr edge_rect,addr client_rect
	mov	eax,[edge_rect.right]
	sub	eax,CSD_EDGE_PX
	mov	[edge_rect.left],eax
	invoke	FillRect,rbx,addr edge_rect,[hEdgeBrush]

	invoke	CopyRect,addr edge_rect,addr client_rect
	mov	eax,[edge_rect.bottom]
	sub	eax,CSD_EDGE_PX
	mov	[edge_rect.top],eax
	invoke	FillRect,rbx,addr edge_rect,[hEdgeBrush]

	invoke	SetBkMode,rbx,TRANSPARENT
	mov	[old_bk],eax
	invoke	SetTextColor,rbx,CSD_TITLE_TEXT
	mov	[old_color],eax
	invoke	InflateRect,addr title_rect,-CSD_CONTENT_PAD,0
	invoke	DrawTextW,rbx,title_text,-1,addr title_rect,\
		DT_LEFT or DT_VCENTER or DT_SINGLELINE or DT_NOPREFIX

	invoke	CopyRect,addr body_rect,addr client_rect
	mov	eax,[body_rect.left]
	add	eax,CSD_CONTENT_PAD
	mov	[body_rect.left],eax
	mov	eax,[body_rect.top]
	add	eax,CSD_TITLE_HEIGHT + CSD_CONTENT_PAD
	mov	[body_rect.top],eax
	mov	eax,[body_rect.right]
	sub	eax,CSD_CONTENT_PAD
	mov	[body_rect.right],eax
	mov	eax,[body_rect.bottom]
	sub	eax,CSD_TITLE_HEIGHT
	mov	[body_rect.bottom],eax
	invoke	SetTextColor,rbx,CSD_BODY_TEXT
	invoke	DrawTextW,rbx,body_text,-1,addr body_rect,\
		DT_LEFT or DT_TOP or DT_WORDBREAK or DT_NOPREFIX

	invoke	CopyRect,addr footer_rect,addr client_rect
	mov	eax,[footer_rect.left]
	add	eax,CSD_CONTENT_PAD
	mov	[footer_rect.left],eax
	mov	eax,[footer_rect.right]
	sub	eax,CSD_CONTENT_PAD
	mov	[footer_rect.right],eax
	mov	eax,[footer_rect.bottom]
	sub	eax,CSD_TITLE_HEIGHT - 8
	mov	[footer_rect.top],eax
	mov	eax,[client_rect.bottom]
	sub	eax,CSD_EDGE_PX + 8
	mov	[footer_rect.bottom],eax
	invoke	SetTextColor,rbx,CSD_MUTED_TEXT
	invoke	DrawTextW,rbx,footer_text,-1,addr footer_rect,\
		DT_LEFT or DT_BOTTOM or DT_SINGLELINE or DT_NOPREFIX

	invoke	SetTextColor,rbx,dword [old_color]
	invoke	SetBkMode,rbx,dword [old_bk]
	ret
endp

proc HitTestCsdFrame hwnd,lparam_value
    locals
	xpos		dd ?
	ypos		dd ?
	left_hit	dd ?
	right_hit	dd ?
	top_hit		dd ?
	bottom_hit	dd ?
    endl

	mov	r8,rcx
	mov	eax,edx
	movsx	ecx,ax
	sar	eax,16
	movsx	edx,ax
	mov	[xpos],ecx
	mov	[ypos],edx
	invoke	GetWindowRect,r8,addr window_rect

	mov	dword [left_hit],0
	mov	dword [right_hit],0
	mov	dword [top_hit],0
	mov	dword [bottom_hit],0

	mov	eax,[window_rect.left]
	add	eax,CSD_RESIZE_PX
	cmp	[xpos],eax
	jg	hittest_check_right
	mov	dword [left_hit],1

  hittest_check_right:
	mov	eax,[window_rect.right]
	sub	eax,CSD_RESIZE_PX
	cmp	[xpos],eax
	jl	hittest_check_top
	mov	dword [right_hit],1

  hittest_check_top:
	mov	eax,[window_rect.top]
	add	eax,CSD_RESIZE_PX
	cmp	[ypos],eax
	jg	hittest_check_bottom
	mov	dword [top_hit],1

  hittest_check_bottom:
	mov	eax,[window_rect.bottom]
	sub	eax,CSD_RESIZE_PX
	cmp	[ypos],eax
	jl	hittest_decide
	mov	dword [bottom_hit],1

  hittest_decide:
	cmp	dword [top_hit],0
	je	hittest_not_top
	cmp	dword [left_hit],0
	jne	hittest_top_left
	cmp	dword [right_hit],0
	jne	hittest_top_right
	mov	eax,HTTOP
	ret

  hittest_not_top:
	cmp	dword [bottom_hit],0
	je	hittest_not_bottom
	cmp	dword [left_hit],0
	jne	hittest_bottom_left
	cmp	dword [right_hit],0
	jne	hittest_bottom_right
	mov	eax,HTBOTTOM
	ret

  hittest_not_bottom:
	cmp	dword [left_hit],0
	jne	hittest_left
	cmp	dword [right_hit],0
	jne	hittest_right
	mov	eax,[window_rect.top]
	add	eax,CSD_TITLE_HEIGHT
	cmp	[ypos],eax
	jge	hittest_client
	mov	eax,HTCAPTION
	ret

  hittest_client:
	mov	eax,HTCLIENT
	ret
  hittest_top_left:
	mov	eax,HTTOPLEFT
	ret
  hittest_top_right:
	mov	eax,HTTOPRIGHT
	ret
  hittest_bottom_left:
	mov	eax,HTBOTTOMLEFT
	ret
  hittest_bottom_right:
	mov	eax,HTBOTTOMRIGHT
	ret
  hittest_left:
	mov	eax,HTLEFT
	ret
  hittest_right:
	mov	eax,HTRIGHT
	ret
endp

proc WindowProc hwnd,wmsg,wparam,lparam
    locals
	ps	PAINTSTRUCT
    endl

	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	iterate <message,branch>,\
		WM_NCCALCSIZE,	wnd_nccalcsize,\
		WM_NCHITTEST,	wnd_nchittest,\
		WM_CREATE,	wnd_create,\
		WM_SIZE,	wnd_size,\
		WM_KEYDOWN,	wnd_keydown,\
		WM_ERASEBKGND,	wnd_erasebkgnd,\
		WM_PAINT,	wnd_paint,\
		WM_DESTROY,	wnd_destroy

		cmp	edx,message
		je	branch
	end iterate

  wnd_default:
	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  wnd_nccalcsize:
	cmp	qword [wparam],0
	je	wnd_nccalc_done
	mov	eax,0300h		; WVR_REDRAW
	ret

  wnd_nccalc_done:
	xor	eax,eax
	ret

  wnd_nchittest:
	fastcall HitTestCsdFrame,[hwnd],r9
	ret

  wnd_create:
	mov	rax,[hwnd]
	mov	[hMain],rax
	invoke	DwmExtendFrameIntoClientArea,[hwnd],addr dwm_margins
	fastcall CreatePaintObjects
	xor	eax,eax
	ret

  wnd_size:
	invoke	InvalidateRect,[hwnd],0,1
	xor	eax,eax
	ret

  wnd_keydown:
	cmp	r8d,VK_ESCAPE
	jne	wnd_default
	invoke	DestroyWindow,[hwnd]
	xor	eax,eax
	ret

  wnd_erasebkgnd:
	mov	eax,1
	ret

  wnd_paint:
	invoke	BeginPaint,[hwnd],addr ps
	fastcall PaintFrame,rax
	invoke	EndPaint,[hwnd],addr ps
	xor	eax,eax
	ret

  wnd_destroy:
	fastcall DestroyPaintObjects
	invoke	PostQuitMessage,0
	xor	eax,eax
	ret
endp

proc start
	invoke	GetModuleHandleW,0
	mov	[hInstance],rax
	mov	[wc.hInstance],rax

	invoke	LoadCursorW,0,IDC_ARROW
	mov	[wc.hCursor],rax
	invoke	LoadIconW,0,IDI_APPLICATION
	mov	[wc.hIcon],rax
	mov	[wc.hIconSm],rax

	invoke	RegisterClassExW,addr wc
	test	rax,rax
	jz	start_fatal_register

	invoke	CreateWindowEx,WS_EX_APPWINDOW,class_name,app_name,\
		WS_OVERLAPPEDWINDOW,\
		CW_USEDEFAULT,CW_USEDEFAULT,860,460,\
		0,0,[hInstance],0
	mov	[hMain],rax
	test	rax,rax
	jz	start_fatal_window

	invoke	ShowWindow,[hMain],SW_SHOWDEFAULT
	invoke	UpdateWindow,[hMain]

  start_message_loop:
	invoke	GetMessageW,addr msg,0,0,0
	test	eax,eax
	jle	start_exit
	invoke	TranslateMessage,addr msg
	invoke	DispatchMessageW,addr msg
	jmp	start_message_loop

  start_exit:
	invoke	ExitProcess,[msg.wParam]

  start_fatal_register:
	invoke	MessageBox,0,'RegisterClassExW failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1

  start_fatal_window:
	invoke	MessageBox,0,'CreateWindowExW failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1
endp
