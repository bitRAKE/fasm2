; font_icon_demo.asm - font-icon helper routines and use cases.

include 'windows.inc'
include 'resource.h'
include 'font_icons.inc'

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
	class_name	GLOBWSTR 'Fasm2FontIconDemo',0
	toolbar_strings	dw ICON_ADD,0,ICON_SAVE,0,ICON_SEARCH,0,ICON_REFRESH,0,0

	align 8
	icc INITCOMMONCONTROLSEX dwSize: sizeof.INITCOMMONCONTROLSEX,\
		dwICC: ICC_BAR_CLASSES

	wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: WindowProc,\
		hbrBackground: COLOR_WINDOW + 1,\
		lpszClassName: class_name

	tb_add TBBUTTON iBitmap: FONTICON_I_IMAGENONE,\
		idCommand: ID_TOOL_ADD,\
		fsState: TBSTATE_ENABLED,\
		fsStyle: TBSTYLE_BUTTON,\
		dwData: ICON_ADD
	tb_save TBBUTTON iBitmap: FONTICON_I_IMAGENONE,\
		idCommand: ID_TOOL_SAVE,\
		fsStyle: TBSTYLE_BUTTON,\
		dwData: ICON_SAVE
	tb_search TBBUTTON iBitmap: FONTICON_I_IMAGENONE,\
		idCommand: ID_TOOL_SEARCH,\
		fsState: TBSTATE_ENABLED,\
		fsStyle: TBSTYLE_BUTTON,\
		dwData: ICON_SEARCH
	tb_refresh TBBUTTON iBitmap: FONTICON_I_IMAGENONE,\
		idCommand: ID_TOOL_REFRESH,\
		fsState: TBSTATE_ENABLED or TBSTATE_CHECKED,\
		fsStyle: TBSTYLE_CHECK,\
		dwData: ICON_REFRESH
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

	mov	rcx,[hIconFont]
	jrcxz	.no_icon_font
	invoke	DeleteObject,rcx
	mov	[hIconFont],0
  .no_icon_font:
	mov	rcx,[hStaticFont]
	jrcxz	.no_static_font
	invoke	DeleteObject,rcx
	mov	[hStaticFont],0
  .no_static_font:
	mov	rcx,[hCursorGlyph]
	jrcxz	.no_cursor
	invoke	DestroyCursor,rcx
	mov	[hCursorGlyph],0
  .no_cursor:

	fastcall FontIcon_CreateFontForWindow,[hMain],28,'Segoe Fluent Icons'
	mov	[hIconFont],rax
	fastcall FontIcon_CreateFontForWindow,[hMain],64,'Segoe Fluent Icons'
	mov	[hStaticFont],rax
	invoke	GetSystemMetrics,SM_CXCURSOR
	test	eax,eax
	jg	.have_cursor_size
	mov	eax,32
  .have_cursor_size:
	mov	[cursor_pixels],eax
	fastcall FontIcon_CreateGlyphCursor,ICON_POINTER,[cursor_pixels],4,4,ROLE_ACCENT,'Segoe Fluent Icons'
	mov	[hCursorGlyph],rax

	mov	rcx,[hStaticIcon]
	jrcxz	.done
	fastcall FontIcon_SetStaticGlyph,rcx,ICON_SETTINGS,[hStaticFont]
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

	invoke	CreateWindowExW,0,'STATIC',0,\
		WS_CHILD or WS_VISIBLE or SS_CENTER or SS_CENTERIMAGE,\
		28,128,104,104,[hMain],ID_STATIC_ICON,[hInstance],0
	mov	[hStaticIcon],rax
	fastcall RebuildIconResources
	ret
endp

proc ResizeDemo
	cmp	[hToolbar],0
	je	.no_toolbar
	invoke	SendMessageW,[hToolbar],TB_AUTOSIZE,0,0
  .no_toolbar:
	cmp	[hStaticIcon],0
	je	.done
	invoke	MoveWindow,[hStaticIcon],28,128,104,104,1
  .done:
	ret
endp

proc PaintLayeredGlyphs uses rbx, hdc
	mov	rbx,rcx

	mov	[panel_rect.left],168
	mov	[panel_rect.top],128
	mov	[panel_rect.right],500
	mov	[panel_rect.bottom],232
	invoke	FillRect,rbx,addr panel_rect,[hBgBrush]

	mov	[glyph_rect.left],190
	mov	[glyph_rect.top],146
	mov	[glyph_rect.right],266
	mov	[glyph_rect.bottom],216
	fastcall FontIcon_DrawGlyph,rbx,ICON_RING,addr glyph_rect,[hStaticFont],ROLE_MUTED
	fastcall FontIcon_DrawGlyph,rbx,ICON_BADGE,addr glyph_rect,[hStaticFont],ROLE_ACCENT

	mov	[glyph_rect.left],286
	mov	[glyph_rect.top],146
	mov	[glyph_rect.right],362
	mov	[glyph_rect.bottom],216
	fastcall FontIcon_DrawGlyph,rbx,ICON_REFRESH,addr glyph_rect,[hStaticFont],ROLE_ACTIVE

	mov	[glyph_rect.left],382
	mov	[glyph_rect.top],146
	mov	[glyph_rect.right],458
	mov	[glyph_rect.bottom],216
	fastcall FontIcon_DrawGlyph,rbx,ICON_SEARCH,addr glyph_rect,[hStaticFont],ROLE_TEXT
	ret
endp

proc WindowProc uses rbx, hwnd,wmsg,wparam,lparam
	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9

	iterate <message,branch>,\
		WM_CREATE,		.wm_create,\
		WM_SIZE,		.wm_size,\
		WM_DPICHANGED,		.wm_dpichanged,\
		WM_NOTIFY,		.wm_notify,\
		WM_CTLCOLORSTATIC,	.wm_ctlcolorstatic,\
		WM_SETCURSOR,		.wm_setcursor,\
		WM_PAINT,		.wm_paint,\
		WM_COMMAND,		.wm_command,\
		WM_DESTROY,		.wm_destroy

		cmp	edx,message
		je	branch
	end iterate

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
	cmp	[rbx+NMHDR.code],NM_CUSTOMDRAW
	jne	.done_zero
	fastcall FontIcon_ToolbarGlyphCustomDraw,rbx,[hIconFont],\
		ROLE_TEXT,ROLE_ACCENT,ROLE_ON_ACCENT,ROLE_DISABLED,\
		ROLE_TOOL_BG,ROLE_HOVER_BG,ROLE_PRESSED_BG,ROLE_ACTIVE_BG
	ret

  .wm_ctlcolorstatic:
	fastcall FontIcon_PrepareControlColor,[wparam],ROLE_ACCENT,[hBgBrush]
	ret

  .wm_setcursor:
	mov	rcx,[hCursorGlyph]
	test	rcx,rcx
	jz	.default
	invoke	SetCursor,rcx
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
	mov	rcx,[hCursorGlyph]
	jrcxz	.no_cursor
	invoke	DestroyCursor,rcx
  .no_cursor:
	mov	rcx,[hStaticFont]
	jrcxz	.no_static_font
	invoke	DeleteObject,rcx
  .no_static_font:
	mov	rcx,[hIconFont]
	jrcxz	.no_icon_font
	invoke	DeleteObject,rcx
  .no_icon_font:
	mov	rcx,[hBgBrush]
	jrcxz	.no_brush
	invoke	DeleteObject,rcx
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

	invoke	CreateWindowExW,0,class_name,'Font Icon Helpers',\
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
	invoke	MessageBoxW,0,'Font icon demo failed to start.','Font Icon Helpers',MB_ICONERROR
	invoke	ExitProcess,1
endp
