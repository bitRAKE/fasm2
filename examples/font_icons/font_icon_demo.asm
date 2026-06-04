; font_icon_demo.asm - font-icon helper routines and use cases.

include 'windows.inc'
include 'font_icons.inc'

ID_TOOL_ADD	= 1001
ID_TOOL_SAVE	= 1002
ID_TOOL_SEARCH	= 1003
ID_TOOL_REFRESH	= 1004
ID_STATIC_ICON	= 2001

WM_DPICHANGED	= 02E0h

ICON_ADD	= 0E710h
ICON_SETTINGS	= 0E713h
ICON_SEARCH	= 0E721h
ICON_REFRESH	= 0E72Ch
ICON_SAVE	= 0E74Eh
ICON_POINTER	= 0E8B0h
ICON_BADGE	= 0E734h
ICON_RING	= 0E73Ah

ROLE_SURFACE	= 0F7F9FCh
ROLE_TEXT	= 0423A33h
ROLE_ACCENT	= 00A0A0h
ROLE_MUTED	= 0877C70h
ROLE_ACTIVE	= 008C45h
ROLE_TOOL_BG	= 0EDF1F5h
ROLE_HOVER_BG	= 0D8F4F4h
ROLE_PRESSED_BG	= 00A0A0h
ROLE_ACTIVE_BG	= 0D7F1E3h
ROLE_DISABLED	= 0B0A8A0h
ROLE_ON_ACCENT	= 0FFFFFFh

define __GLOBAL_DATA__ demo_data
define __GLOBAL_BSS__ demo_bss

macro demo_data
	app_name	du 'Font Icon Helpers',0
	class_name	du 'Fasm2FontIconDemo',0
	static_class	du 'STATIC',0
	fluent_face	du 'Segoe Fluent Icons',0
	toolbar_strings	dw ICON_ADD,0,ICON_SAVE,0,ICON_SEARCH,0,ICON_REFRESH,0,0

	align 8
	icc INITCOMMONCONTROLSEX dwSize: sizeof.INITCOMMONCONTROLSEX,\
		dwICC: ICC_BAR_CLASSES

	wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: WindowProc,\
		cbClsExtra: 0,\
		cbWndExtra: 0,\
		hInstance: 0,\
		hIcon: 0,\
		hCursor: 0,\
		hbrBackground: COLOR_WINDOW + 1,\
		lpszMenuName: 0,\
		lpszClassName: class_name,\
		hIconSm: 0

	tb_add TBBUTTON iBitmap: FONTICON_I_IMAGENONE,\
		idCommand: ID_TOOL_ADD,\
		fsState: TBSTATE_ENABLED,\
		fsStyle: TBSTYLE_BUTTON,\
		dwData: ICON_ADD,\
		iString: 0
	tb_save TBBUTTON iBitmap: FONTICON_I_IMAGENONE,\
		idCommand: ID_TOOL_SAVE,\
		fsState: 0,\
		fsStyle: TBSTYLE_BUTTON,\
		dwData: ICON_SAVE,\
		iString: 0
	tb_search TBBUTTON iBitmap: FONTICON_I_IMAGENONE,\
		idCommand: ID_TOOL_SEARCH,\
		fsState: TBSTATE_ENABLED,\
		fsStyle: TBSTYLE_BUTTON,\
		dwData: ICON_SEARCH,\
		iString: 0
	tb_refresh TBBUTTON iBitmap: FONTICON_I_IMAGENONE,\
		idCommand: ID_TOOL_REFRESH,\
		fsState: TBSTATE_ENABLED or TBSTATE_CHECKED,\
		fsStyle: TBSTYLE_CHECK,\
		dwData: ICON_REFRESH,\
		iString: 0
purge demo_data
end macro

macro demo_bss
	align 8
	hInstance	dq ?
	hMain		dq ?
	hToolbar	dq ?
	hStaticIcon	dq ?
	hIconFont	dq ?
	hStaticFont	dq ?
	hCursorGlyph	dq ?
	hBgBrush	dq ?
	msg		MSG
	ps		PAINTSTRUCT
	client_rect	RECT
	panel_rect	RECT
	glyph_rect	RECT
purge demo_bss
end macro

proc RebuildIconResources
    locals
	cursor_pixels dd ?
    endl

	cmp	qword [hIconFont],0
	je	.no_icon_font
	invoke	DeleteObject,[hIconFont]
	mov	qword [hIconFont],0
  .no_icon_font:
	cmp	qword [hStaticFont],0
	je	.no_static_font
	invoke	DeleteObject,[hStaticFont]
	mov	qword [hStaticFont],0
  .no_static_font:
	cmp	qword [hCursorGlyph],0
	je	.no_cursor
	invoke	DestroyCursor,[hCursorGlyph]
	mov	qword [hCursorGlyph],0
  .no_cursor:

	fastcall FontIcon_CreateFontForWindow,[hMain],28,fluent_face
	mov	[hIconFont],rax
	fastcall FontIcon_CreateFontForWindow,[hMain],64,fluent_face
	mov	[hStaticFont],rax
	invoke	GetSystemMetrics,SM_CXCURSOR
	test	eax,eax
	jg	.have_cursor_size
	mov	eax,32
  .have_cursor_size:
	mov	dword [cursor_pixels],eax
	fastcall FontIcon_CreateGlyphCursor,ICON_POINTER,dword [cursor_pixels],4,4,ROLE_ACCENT,fluent_face
	mov	[hCursorGlyph],rax

	cmp	qword [hStaticIcon],0
	je	.done
	fastcall FontIcon_SetStaticGlyph,[hStaticIcon],ICON_SETTINGS,[hStaticFont]
  .done:
	ret
endp

proc DemoCreateToolbar
	invoke	CreateWindowExW,0,TOOLBAR_CLASS,0,\
		WS_CHILD or WS_VISIBLE or TBSTYLE_FLAT or TBSTYLE_TOOLTIPS or CCS_TOP or CCS_NODIVIDER,\
		0,0,0,0,[hMain],0,[hInstance],0
	mov	[hToolbar],rax
	test	rax,rax
	jz	.done

	invoke	SendMessageW,[hToolbar],TB_BUTTONSTRUCTSIZE,sizeof.TBBUTTON,0
	invoke	SendMessageW,[hToolbar],TB_SETMAXTEXTROWS,1,0
	invoke	SendMessageW,[hToolbar],TB_SETBITMAPSIZE,0,(28 shl 16) or 28
	invoke	SendMessageW,[hToolbar],TB_SETBUTTONSIZE,0,(48 shl 16) or 54
	invoke	SendMessageW,[hToolbar],TB_SETBUTTONWIDTH,0,(54 shl 16) or 54
	invoke	SendMessageW,[hToolbar],TB_ADDSTRINGW,0,addr toolbar_strings
	test	eax,eax
	js	.no_strings
	mov	[tb_add.iString],rax
	inc	rax
	mov	[tb_save.iString],rax
	inc	rax
	mov	[tb_search.iString],rax
	inc	rax
	mov	[tb_refresh.iString],rax
  .no_strings:
	invoke	SendMessageW,[hToolbar],TB_ADDBUTTONS,4,addr tb_add
	invoke	SendMessageW,[hToolbar],TB_PRESSBUTTON,ID_TOOL_SEARCH,1
	invoke	SendMessageW,[hToolbar],TB_AUTOSIZE,0,0
  .done:
	mov	rax,[hToolbar]
	ret
endp

proc CreateDemoControls
	invoke	CreateSolidBrush,ROLE_SURFACE
	mov	[hBgBrush],rax

	fastcall DemoCreateToolbar

	invoke	CreateWindowExW,0,static_class,0,\
		WS_CHILD or WS_VISIBLE or SS_CENTER or SS_CENTERIMAGE,\
		28,128,104,104,[hMain],ID_STATIC_ICON,[hInstance],0
	mov	[hStaticIcon],rax
	fastcall RebuildIconResources
	ret
endp

proc ResizeDemo
	cmp	qword [hToolbar],0
	je	.no_toolbar
	invoke	SendMessageW,[hToolbar],TB_AUTOSIZE,0,0
  .no_toolbar:
	cmp	qword [hStaticIcon],0
	je	.done
	invoke	MoveWindow,[hStaticIcon],28,128,104,104,1
  .done:
	ret
endp

proc PaintLayeredGlyphs hdc
	mov	[hdc],rcx

	mov	dword [panel_rect.left],168
	mov	dword [panel_rect.top],128
	mov	dword [panel_rect.right],500
	mov	dword [panel_rect.bottom],232
	invoke	FillRect,[hdc],addr panel_rect,[hBgBrush]

	mov	dword [glyph_rect.left],190
	mov	dword [glyph_rect.top],146
	mov	dword [glyph_rect.right],266
	mov	dword [glyph_rect.bottom],216
	fastcall FontIcon_DrawGlyph,[hdc],ICON_RING,addr glyph_rect,[hStaticFont],ROLE_MUTED
	fastcall FontIcon_DrawGlyph,[hdc],ICON_BADGE,addr glyph_rect,[hStaticFont],ROLE_ACCENT

	mov	dword [glyph_rect.left],286
	mov	dword [glyph_rect.top],146
	mov	dword [glyph_rect.right],362
	mov	dword [glyph_rect.bottom],216
	fastcall FontIcon_DrawGlyph,[hdc],ICON_REFRESH,addr glyph_rect,[hStaticFont],ROLE_ACTIVE

	mov	dword [glyph_rect.left],382
	mov	dword [glyph_rect.top],146
	mov	dword [glyph_rect.right],458
	mov	dword [glyph_rect.bottom],216
	fastcall FontIcon_DrawGlyph,[hdc],ICON_SEARCH,addr glyph_rect,[hStaticFont],ROLE_TEXT
	ret
endp

proc WindowProc uses rbx, hwnd,wmsg,wparam,lparam
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	cmp	edx,WM_CREATE
	je	.wm_create
	cmp	edx,WM_SIZE
	je	.wm_size
	cmp	edx,WM_DPICHANGED
	je	.wm_dpichanged
	cmp	edx,WM_NOTIFY
	je	.wm_notify
	cmp	edx,WM_CTLCOLORSTATIC
	je	.wm_ctlcolorstatic
	cmp	edx,WM_SETCURSOR
	je	.wm_setcursor
	cmp	edx,WM_PAINT
	je	.wm_paint
	cmp	edx,WM_COMMAND
	je	.wm_command
	cmp	edx,WM_DESTROY
	je	.wm_destroy

	invoke	DefWindowProcW,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  .wm_create:
	mov	rax,[hwnd]
	mov	[hMain],rax
	fastcall CreateDemoControls
	xor	eax,eax
	ret

  .wm_size:
	fastcall ResizeDemo
	xor	eax,eax
	ret

  .wm_dpichanged:
	fastcall RebuildIconResources
	invoke	InvalidateRect,[hwnd],0,1
	xor	eax,eax
	ret

  .wm_notify:
	mov	rbx,[lparam]
	cmp	dword [rbx+NMHDR.code],NM_CUSTOMDRAW
	jne	.done_zero
	fastcall FontIcon_ToolbarGlyphCustomDraw,rbx,[hIconFont],\
		ROLE_TEXT,ROLE_ACCENT,ROLE_ON_ACCENT,ROLE_DISABLED,\
		ROLE_TOOL_BG,ROLE_HOVER_BG,ROLE_PRESSED_BG,ROLE_ACTIVE_BG
	ret

  .wm_ctlcolorstatic:
	fastcall FontIcon_PrepareControlColor,[wparam],ROLE_ACCENT,[hBgBrush]
	ret

  .wm_setcursor:
	cmp	qword [hCursorGlyph],0
	je	.default
	invoke	SetCursor,[hCursorGlyph]
	mov	eax,1
	ret

  .wm_paint:
	invoke	BeginPaint,[hwnd],addr ps
	fastcall PaintLayeredGlyphs,rax
	invoke	EndPaint,[hwnd],addr ps
	xor	eax,eax
	ret

  .wm_command:
	invoke	InvalidateRect,[hwnd],0,1
	xor	eax,eax
	ret

  .wm_destroy:
	cmp	qword [hCursorGlyph],0
	je	.no_cursor
	invoke	DestroyCursor,[hCursorGlyph]
  .no_cursor:
	cmp	qword [hStaticFont],0
	je	.no_static_font
	invoke	DeleteObject,[hStaticFont]
  .no_static_font:
	cmp	qword [hIconFont],0
	je	.no_icon_font
	invoke	DeleteObject,[hIconFont]
  .no_icon_font:
	cmp	qword [hBgBrush],0
	je	.no_brush
	invoke	DeleteObject,[hBgBrush]
  .no_brush:
	invoke	PostQuitMessage,0
  .done_zero:
	xor	eax,eax
	ret

  .default:
	invoke	DefWindowProcW,[hwnd],dword [wmsg],[wparam],[lparam]
	ret
endp

proc start
	invoke	GetModuleHandleW,0
	mov	[hInstance],rax
	mov	[wc.hInstance],rax

	invoke	InitCommonControlsEx,addr icc

	invoke	LoadCursorW,0,IDC_ARROW
	mov	[wc.hCursor],rax
	invoke	LoadIconW,0,IDI_APPLICATION
	mov	[wc.hIcon],rax
	mov	[wc.hIconSm],rax

	invoke	RegisterClassExW,addr wc
	test	rax,rax
	jz	.fatal

	invoke	CreateWindowExW,0,class_name,app_name,\
		WS_OVERLAPPEDWINDOW,\
		CW_USEDEFAULT,CW_USEDEFAULT,640,360,\
		0,0,[hInstance],0
	mov	[hMain],rax
	test	rax,rax
	jz	.fatal

	invoke	ShowWindow,[hMain],SW_SHOWDEFAULT
	invoke	UpdateWindow,[hMain]

  .message_loop:
	invoke	GetMessageW,addr msg,0,0,0
	test	eax,eax
	jle	.exit
	invoke	TranslateMessage,addr msg
	invoke	DispatchMessageW,addr msg
	jmp	.message_loop

  .exit:
	invoke	ExitProcess,[msg.wParam]

  .fatal:
	invoke	MessageBoxW,0,'Font icon demo failed to start.',app_name,MB_ICONERROR
	invoke	ExitProcess,1
endp
