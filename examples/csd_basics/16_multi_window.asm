; 16_multi_window.asm - per-HWND CSD state with an owned tool frame.
;
; This steps back to the 09 caption surface so the ownership change stays
; visible: each HWND stores its own caption state, DPI, theme objects, and child
; controls through GWLP_USERDATA.

ADDON_WINDOWS_RESOURCE equ '16_multi_window.res'

include 'addon\windows.inc'
include '..\font_icons\font_icons.inc'
include 'addon\csd\caption.inc'
include 'addon\csd\dpi.inc'
include 'addon\csd\theme.inc'
include 'addon\csd\state.inc'
include 'addon\csd\snap.inc'
include 'addon\csd\child.inc'

CSD_TITLE_HEIGHT_DIP	= 48
CSD_CONTENT_PAD_DIP	= 26
CSD_EDGE_DIP		= 4
CSD_BUTTON_WIDTH_DIP	= 44
CSD_ICON_DIP		= 18
CSD_SEARCH_WIDTH_DIP	= 240
CSD_CHILD_PAD_X_DIP	= 8
CSD_CHILD_PAD_Y_DIP	= 12
CSD_DRAG_PAD_DIP	= CSD_CONTENT_PAD_DIP / 2
CSD_INITIAL_CLIENT_W_DIP = 920
CSD_INITIAL_CLIENT_H_DIP = 500
CSD_TOOL_CLIENT_W_DIP	= 560
CSD_TOOL_CLIENT_H_DIP	= 300
CSD_STATE_BUTTON_HOVER	= 00DCEED7h
CSD_STATE_BUTTON_PRESSED = 00BBD8B4h
CSD_STATE_BUTTON_INACTIVE = 006A706Ah
CSD_STATE_CLOSE_HOVER	= 003232E8h
CSD_STATE_CLOSE_PRESSED = 001818B8h
CSD_STATE_TEXT_INACTIVE = 00AEB6AEh

ICON_MENU		= 0E700h
ICON_CLOSE		= 0E8BBh
ICON_MINIMIZE		= 0E921h
ICON_MAXIMIZE		= 0E922h
ICON_RESTORE		= 0E923h
ICON_SEARCH		= 0E721h
ICON_SETTINGS		= 0E713h

ID_CAPTION_SYSTEM	= 4201
ID_CAPTION_SEARCH	= 4202
ID_CAPTION_SETTINGS	= 4203
ID_CAPTION_CLOSE	= 4204
ID_CAPTION_MAX		= 4205
ID_CAPTION_MIN		= 4206
CSD_CAPTION_SEARCH_INDEX = 1
CSD_CAPTION_MAX_INDEX	= 4
CSD_WINDOW_MAX		= 4
CSD_WINDOW_MAIN		= 0001h
CSD_WINDOW_TOOL		= 0002h

struct CSD_WINDOW
  hwnd		    dq ?
  owner		    dq ?
  flags		    dd ?
  current_dpi	    dd ?
  resize_border_px  dd ?
		    dd ?
  hBodyBrush	    dq ?
  hTitleBrush	    dq ?
  hButtonBrush	    dq ?
  hButtonHoverBrush dq ?
  hButtonPressedBrush dq ?
  hButtonInactiveBrush dq ?
  hCloseBrush	    dq ?
  hCloseHoverBrush  dq ?
  hClosePressedBrush dq ?
  hEdgeBrush	    dq ?
  hCaptionFont	    dq ?
  hTextFont	    dq ?
  client_rect	    RECT
  window_rect	    RECT
  caption_drag_rect RECT
  current_theme	    CSD_THEME
  caption_input	    CSD_CAPTION_INPUT
  snap_policy	    CSD_SNAP_POLICY
  hSearchEdit	    dq ?
  hUiFont	    dq ?
  status_buffer	    rw 160
  caption_geometry rb sizeof.CSD_CAPTION_GEOMETRY * CSD_CAPTION_CONTROL_COUNT
  caption_state    rb sizeof.CSD_CAPTION_STATE * CSD_CAPTION_CONTROL_COUNT
ends

hMain			equ active_window.hwnd
hBodyBrush		equ active_window.hBodyBrush
hTitleBrush		equ active_window.hTitleBrush
hButtonBrush		equ active_window.hButtonBrush
hButtonHoverBrush	equ active_window.hButtonHoverBrush
hButtonPressedBrush	equ active_window.hButtonPressedBrush
hButtonInactiveBrush	equ active_window.hButtonInactiveBrush
hCloseBrush		equ active_window.hCloseBrush
hCloseHoverBrush	equ active_window.hCloseHoverBrush
hClosePressedBrush	equ active_window.hClosePressedBrush
hEdgeBrush		equ active_window.hEdgeBrush
hCaptionFont		equ active_window.hCaptionFont
hTextFont		equ active_window.hTextFont
client_rect		equ active_window.client_rect
window_rect		equ active_window.window_rect
caption_drag_rect	equ active_window.caption_drag_rect
current_dpi		equ active_window.current_dpi
resize_border_px	equ active_window.resize_border_px
current_theme		equ active_window.current_theme
caption_input		equ active_window.caption_input
snap_policy		equ active_window.snap_policy
hSearchEdit		equ active_window.hSearchEdit
hUiFont			equ active_window.hUiFont
status_buffer		equ active_window.status_buffer
caption_geometry	equ active_window.caption_geometry
caption_state		equ active_window.caption_state

define __GLOBAL_DATA__ csd_data
define __GLOBAL_BSS__ csd_bss

macro csd_data
	app_name	du 'CSD 16 - multi-window context',0
	tool_name	du 'CSD 16 - owned tool frame',0
	class_name	du 'Fasm2CsdMultiWindow',0
	title_text	du '16  Per-HWND CSD context',0
	body_text	du 'This version keeps the 09 caption/edit surface but moves window state into a CSD_WINDOW record stored in GWLP_USERDATA.',13,10,13,10
			du 'The owned tool frame has its own DPI, theme brushes, caption state, Search child HWND, and system-menu path. Closing the tool does not quit the app.',0
	status_format	du '%s ctx %p | owner %p | edit HWND %p | snap %s',0
	status_edit	du 'Type in the caption edit. The child HWND owns focus, text input, and IME.',0
	status_settings	du 'Settings caption button clicked.',0
	role_main	du 'main',0
	role_tool	du 'tool',0
	theme_dark	du 'dark',0
	theme_light	du 'light',0
	snap_on		du 'HTMAXBUTTON',0
	snap_off	du 'HTCLIENT',0

	align 4
	dwm_margins	MARGINS leftWidth: 1, rightWidth: 1, topHeight: 1, bottomHeight: 1

	caption_descriptors:
		CSD_CAPTION_DESCRIPTOR id: ID_CAPTION_SYSTEM,\
			glyph: ICON_MENU, hitCode: HTCLIENT,\
			flags: CSD_CAPTION_ALIGN_LEADING or CSD_CAPTION_SYSTEM,\
			widthDip: CSD_BUTTON_WIDTH_DIP
		CSD_CAPTION_DESCRIPTOR id: ID_CAPTION_SEARCH,\
			glyph: ICON_SEARCH, command: 0,\
			hitCode: HTCLIENT,\
			flags: CSD_CAPTION_ALIGN_LEADING or CSD_CAPTION_CHILD,\
			widthDip: CSD_SEARCH_WIDTH_DIP
		CSD_CAPTION_DESCRIPTOR id: ID_CAPTION_SETTINGS,\
			glyph: ICON_SETTINGS, command: ID_CAPTION_SETTINGS,\
			hitCode: HTCLIENT,\
			flags: CSD_CAPTION_ALIGN_LEADING or CSD_CAPTION_APP,\
			widthDip: CSD_BUTTON_WIDTH_DIP
		CSD_CAPTION_DESCRIPTOR id: ID_CAPTION_CLOSE,\
			glyph: ICON_CLOSE, command: SC_CLOSE,\
			hitCode: HTCLIENT,\
			flags: CSD_CAPTION_ALIGN_TRAILING or CSD_CAPTION_DANGER or CSD_CAPTION_SYSCMD,\
			widthDip: CSD_BUTTON_WIDTH_DIP
		CSD_CAPTION_DESCRIPTOR id: ID_CAPTION_MAX,\
			glyph: ICON_MAXIMIZE, command: SC_MAXIMIZE,\
			hitCode: HTCLIENT,\
			flags: CSD_CAPTION_ALIGN_TRAILING or CSD_CAPTION_SYSCMD,\
			widthDip: CSD_BUTTON_WIDTH_DIP
		CSD_CAPTION_DESCRIPTOR id: ID_CAPTION_MIN,\
			glyph: ICON_MINIMIZE, command: SC_MINIMIZE,\
			hitCode: HTCLIENT,\
			flags: CSD_CAPTION_ALIGN_TRAILING or CSD_CAPTION_SYSCMD,\
			widthDip: CSD_BUTTON_WIDTH_DIP
	caption_descriptors_end:
	CSD_CAPTION_CONTROL_COUNT = (caption_descriptors_end - caption_descriptors) / sizeof.CSD_CAPTION_DESCRIPTOR

	wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: WindowProc,\
		lpszClassName: class_name
purge csd_data
end macro

macro csd_bss
	align 8
	hInstance	dq ?
	active_windowp dq ?
	active_window	CSD_WINDOW
	csd_windows	dq CSD_WINDOW_MAX dup ?
	msg		MSG
	initial_rect	RECT
purge csd_bss
end macro

proc SetDpiState dpi
	test	ecx,ecx
	jnz	dpi_ready
	mov	ecx,CSD_DPI_BASE
  dpi_ready:
	mov	[current_dpi],ecx
	fastcall CsdGetResizeBorderForDpi,ecx
	mov	[resize_border_px],eax
	ret
endp

proc RefreshDpiState hwnd
	fastcall CsdGetWindowDpi,rcx
	fastcall SetDpiState,eax
	ret
endp

proc RebuildCaptionFont
	mov	rcx,[hCaptionFont]
	jrcxz	rebuild_create
	invoke	DeleteObject,rcx
	mov	qword [hCaptionFont],0

  rebuild_create:
	fastcall FontIcon_CreateFontForWindow,[hMain],CSD_ICON_DIP,'Segoe Fluent Icons'
	mov	[hCaptionFont],rax
	ret
endp

proc RebuildTextFont
	mov	rcx,[hTextFont]
	jrcxz	rebuild_text_create
	invoke	DeleteObject,rcx
	mov	qword [hTextFont],0

  rebuild_text_create:
	fastcall CsdCreateTextFontForDpi,dword [current_dpi]
	mov	[hTextFont],rax
	mov	rcx,[hSearchEdit]
	jrcxz	rebuild_text_done
	invoke	SendMessageW,rcx,WM_SETFONT,[hTextFont],1

  rebuild_text_done:
	ret
endp

proc DestroyThemeBrushes
	iterate handle, hBodyBrush,hTitleBrush,hButtonBrush,hButtonHoverBrush,\
		hButtonPressedBrush,hButtonInactiveBrush,hCloseBrush,\
		hCloseHoverBrush,hClosePressedBrush,hEdgeBrush
		mov	rcx,[handle]
		jrcxz	destroy_#handle
		invoke	DeleteObject,rcx
		mov	qword [handle],0
	  destroy_#handle:
	end iterate
	ret
endp

proc RebuildThemeBrushes
	fastcall DestroyThemeBrushes
	iterate <handle,color>,\
		hBodyBrush,		dword [active_window.current_theme.bodyBackColor],\
		hTitleBrush,		dword [active_window.current_theme.captionColor],\
		hButtonBrush,		dword [active_window.current_theme.buttonColor],\
		hButtonHoverBrush,	CSD_STATE_BUTTON_HOVER,\
		hButtonPressedBrush,	CSD_STATE_BUTTON_PRESSED,\
		hButtonInactiveBrush,	CSD_STATE_BUTTON_INACTIVE,\
		hCloseBrush,		dword [active_window.current_theme.closeColor],\
		hCloseHoverBrush,	CSD_STATE_CLOSE_HOVER,\
		hClosePressedBrush,	CSD_STATE_CLOSE_PRESSED,\
		hEdgeBrush,		dword [active_window.current_theme.borderColor]

		invoke	CreateSolidBrush,color
		mov	[handle],rax
	end iterate
	ret
endp

proc WindowRoleName
	test	dword [active_window.flags],CSD_WINDOW_TOOL
	jnz	role_name_tool
	mov	rax,role_main
	ret

  role_name_tool:
	mov	rax,role_tool
	ret
endp

proc UpdateThemeStatus uses rbx rsi
	fastcall WindowRoleName
	mov	rsi,rax
	mov	rbx,snap_off
	cmp	dword [active_window.snap_policy.snapCapable],0
	je	status_snap_ready
	mov	rbx,snap_on
  status_snap_ready:
	invoke	wsprintfW,addr status_buffer,status_format,\
		rsi,[active_windowp],qword [active_window.owner],[hSearchEdit],rbx
	ret
endp

proc RefreshThemeAndFrame hwnd
	mov	[hwnd],rcx
	fastcall CsdThemeRefresh,addr current_theme
	fastcall CsdThemeApplyDwm,[hwnd],addr current_theme
	fastcall RebuildThemeBrushes
	fastcall UpdateThemeStatus
	mov	rcx,[hSearchEdit]
	jrcxz	theme_invalidate_parent
	invoke	InvalidateRect,rcx,0,1

  theme_invalidate_parent:
	invoke	InvalidateRect,[hwnd],0,1
	ret
endp

proc CreatePaintObjects
	fastcall RebuildCaptionFont
	fastcall RebuildTextFont
	fastcall RefreshThemeAndFrame,[hMain]
	ret
endp

proc DestroyPaintObjects
	iterate handle, hCaptionFont,hTextFont
		mov	rcx,[handle]
		jrcxz	destroy_#handle
		invoke	DeleteObject,rcx
		mov	qword [handle],0
	  destroy_#handle:
	end iterate
	fastcall DestroyThemeBrushes
	ret
endp

proc CsdWindowAlloc uses rbx rsi, role,owner
	mov	ebx,ecx
	mov	rsi,rdx
	invoke	GetProcessHeap
	invoke	HeapAlloc,rax,HEAP_ZERO_MEMORY,sizeof.CSD_WINDOW
	test	rax,rax
	jz	window_alloc_done
	mov	[rax+CSD_WINDOW.flags],ebx
	mov	[rax+CSD_WINDOW.owner],rsi

  window_alloc_done:
	ret
endp

proc CsdWindowFree uses rbx, windowp
	mov	rbx,rcx
	test	rbx,rbx
	jz	window_free_done
	invoke	GetProcessHeap
	invoke	HeapFree,rax,0,rbx

  window_free_done:
	ret
endp

proc CsdWindowRegister windowp
	mov	rdx,csd_windows
	mov	r8d,CSD_WINDOW_MAX

  register_loop:
	cmp	qword [rdx],0
	je	register_store
	add	rdx,8
	dec	r8d
	jnz	register_loop
	xor	eax,eax
	ret

  register_store:
	mov	[rdx],rcx
	mov	eax,1
	ret
endp

proc CsdWindowUnregister windowp
	mov	rdx,csd_windows
	mov	r8d,CSD_WINDOW_MAX

  unregister_loop:
	cmp	[rdx],rcx
	je	unregister_clear
	add	rdx,8
	dec	r8d
	jnz	unregister_loop
	ret

  unregister_clear:
	mov	qword [rdx],0
	ret
endp

proc CsdWindowAnyLive
	mov	rcx,csd_windows
	mov	edx,CSD_WINDOW_MAX

  any_live_loop:
	cmp	qword [rcx],0
	jne	any_live_yes
	add	rcx,8
	dec	edx
	jnz	any_live_loop
	xor	eax,eax
	ret

  any_live_yes:
	mov	eax,1
	ret
endp

proc CsdWindowLoad windowp
	mov	[active_windowp],rcx
	invoke	RtlMoveMemory,addr active_window,rcx,sizeof.CSD_WINDOW
	ret
endp

proc CsdWindowSave windowp
	test	rcx,rcx
	jz	window_save_done
	invoke	RtlMoveMemory,rcx,addr active_window,sizeof.CSD_WINDOW

  window_save_done:
	ret
endp

proc CsdWindowRefreshAll uses rbx rsi
	mov	rsi,csd_windows
	mov	ebx,CSD_WINDOW_MAX

  refresh_all_loop:
	mov	rcx,[rsi]
	test	rcx,rcx
	jz	refresh_all_next
	fastcall CsdWindowLoad,rcx
	fastcall RefreshThemeAndFrame,[hMain]
	fastcall CsdWindowSave,[active_windowp]

  refresh_all_next:
	add	rsi,8
	dec	ebx
	jnz	refresh_all_loop
	ret
endp

proc CsdWindowDestroyOwned uses rbx rsi rdi, ownerHwnd
	mov	rdi,rcx
	mov	rsi,csd_windows
	mov	ebx,CSD_WINDOW_MAX

  destroy_owned_loop:
	mov	rax,[rsi]
	test	rax,rax
	jz	destroy_owned_next
	cmp	[rax+CSD_WINDOW.owner],rdi
	jne	destroy_owned_next
	invoke	DestroyWindow,[rax+CSD_WINDOW.hwnd]

  destroy_owned_next:
	add	rsi,8
	dec	ebx
	jnz	destroy_owned_loop
	ret
endp

proc LayoutCaptionControls
	invoke	GetClientRect,[hMain],addr client_rect
	fastcall CsdCaptionLayout,caption_descriptors,caption_geometry,\
		CSD_CAPTION_CONTROL_COUNT,addr client_rect,dword [current_dpi],\
		CSD_TITLE_HEIGHT_DIP,CSD_EDGE_DIP,CSD_DRAG_PAD_DIP,\
		addr caption_drag_rect
	ret
endp

proc CreateCaptionChildren hwnd
	mov	[hwnd],rcx
	cmp	qword [hSearchEdit],0
	jne	create_children_done
	invoke	CreateWindowExW,WS_EX_CLIENTEDGE,'EDIT','Search',\
		WS_CHILD or WS_VISIBLE or WS_TABSTOP or ES_AUTOHSCROLL or ES_NOHIDESEL,\
		0,0,0,0,[hwnd],ID_CAPTION_SEARCH,[hInstance],0
	mov	[hSearchEdit],rax
	test	rax,rax
	jz	create_children_done
	mov	rax,[hTextFont]
	mov	[hUiFont],rax
	invoke	SendMessageW,[hSearchEdit],WM_SETFONT,rax,1
	invoke	SendMessageW,[hSearchEdit],EM_SETMARGINS,\
		EC_LEFTMARGIN or EC_RIGHTMARGIN,8 or (8 shl 16)

  create_children_done:
	ret
endp

proc LayoutCaptionChildren
	mov	rcx,[hSearchEdit]
	jrcxz	layout_children_done
	fastcall CsdCaptionMoveChild,[hSearchEdit],caption_geometry,CSD_CAPTION_SEARCH_INDEX,\
		dword [current_dpi],CSD_CHILD_PAD_X_DIP,CSD_CHILD_PAD_Y_DIP

  layout_children_done:
	ret
endp

proc DestroyCaptionChildren
	mov	rcx,[hSearchEdit]
	jrcxz	destroy_children_done
	invoke	DestroyWindow,rcx
	mov	qword [hSearchEdit],0
	mov	qword [hUiFont],0

  destroy_children_done:
	ret
endp

proc CaptionIndexFromClientPoint x,y
	mov	r8d,ecx
	mov	r9d,edx
	fastcall CsdCaptionIndexFromPoint,caption_geometry,\
		CSD_CAPTION_CONTROL_COUNT,r8d,r9d
	ret
endp

proc CaptionControlFromClientPoint x,y
	mov	r8d,ecx
	mov	r9d,edx
	fastcall CsdCaptionIndexFromPoint,caption_geometry,\
		CSD_CAPTION_CONTROL_COUNT,r8d,r9d
	cmp	eax,CSD_CAPTION_NO_INDEX
	je	find_none
	imul	eax,sizeof.CSD_CAPTION_DESCRIPTOR
	lea	rax,[caption_descriptors+rax]
	ret

  find_none:
	xor	eax,eax
	ret
endp

proc DrawCaptionControls uses rbx rsi rdi r12 r13 r14 r15, hdc
    locals
	button_brush	dq ?
	glyph_color	dd ?
	state_value	dd ?
    endl

	mov	rbx,rcx
	mov	rsi,caption_descriptors
	mov	rdi,caption_geometry
	mov	r15,caption_state
	mov	r14d,CSD_CAPTION_CONTROL_COUNT

  draw_loop:
	lea	r12,[rdi+CSD_CAPTION_GEOMETRY.rect]
	test	[rsi+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_CHILD
	jnz	draw_next
	movzx	eax,byte [r15+CSD_CAPTION_STATE.value]
	mov	[state_value],eax
	mov	eax,[active_window.current_theme.titleTextColor]
	mov	[glyph_color],eax
	test	[rsi+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_DANGER
	jz	draw_pick_normal

	mov	rax,[hCloseBrush]
	mov	[button_brush],rax
	cmp	dword [state_value],CSD_STATE_HOVER
	jne	draw_close_not_hover
	mov	rax,[hCloseHoverBrush]
	mov	[button_brush],rax
	jmp	draw_fill

  draw_close_not_hover:
	cmp	dword [state_value],CSD_STATE_PRESSED
	jne	draw_close_not_pressed
	mov	rax,[hClosePressedBrush]
	mov	[button_brush],rax
	jmp	draw_fill

  draw_close_not_pressed:
	cmp	dword [state_value],CSD_STATE_INACTIVE
	jne	draw_fill
	mov	rax,[hButtonInactiveBrush]
	mov	[button_brush],rax
	mov	dword [glyph_color],CSD_STATE_TEXT_INACTIVE
	jmp	draw_fill

  draw_pick_normal:
	mov	rax,[hButtonBrush]
	mov	[button_brush],rax
	cmp	dword [state_value],CSD_STATE_HOVER
	jne	draw_normal_not_hover
	mov	rax,[hButtonHoverBrush]
	mov	[button_brush],rax
	jmp	draw_fill

  draw_normal_not_hover:
	cmp	dword [state_value],CSD_STATE_PRESSED
	jne	draw_normal_not_pressed
	mov	rax,[hButtonPressedBrush]
	mov	[button_brush],rax
	jmp	draw_fill

  draw_normal_not_pressed:
	cmp	dword [state_value],CSD_STATE_INACTIVE
	jne	draw_fill
	mov	rax,[hButtonInactiveBrush]
	mov	[button_brush],rax
	mov	dword [glyph_color],CSD_STATE_TEXT_INACTIVE

  draw_fill:
	invoke	FillRect,rbx,r12,[button_brush]

  draw_glyph:
	mov	r13d,[rsi+CSD_CAPTION_DESCRIPTOR.glyph]
	cmp	[rsi+CSD_CAPTION_DESCRIPTOR.id],ID_CAPTION_MAX
	jne	draw_glyph_ready
	invoke	IsZoomed,[hMain]
	test	eax,eax
	jz	draw_glyph_ready
	mov	r13d,ICON_RESTORE

  draw_glyph_ready:
	fastcall FontIcon_DrawGlyph,rbx,r13d,r12,[hCaptionFont],\
		dword [glyph_color]

  draw_next:
	add	rsi,sizeof.CSD_CAPTION_DESCRIPTOR
	add	rdi,sizeof.CSD_CAPTION_GEOMETRY
	add	r15,sizeof.CSD_CAPTION_STATE
	dec	r14d
	jnz	draw_loop
	ret
endp

proc PaintFrame uses rbx, hdc
    locals
	title_rect	RECT
	body_rect	RECT
	status_rect	RECT
	edge_rect	RECT
	paint_dpi	dd ?
	title_h		dd ?
	content_pad	dd ?
	edge_px		dd ?
	old_bk		dd ?
	old_color	dd ?
	old_font	dq ?
    endl

	mov	rbx,rcx
	mov	eax,[current_dpi]
	test	eax,eax
	jnz	paint_dpi_ready
	mov	eax,CSD_DPI_BASE
  paint_dpi_ready:
	mov	[paint_dpi],eax
	fastcall CsdDipToPx,CSD_TITLE_HEIGHT_DIP,dword [paint_dpi]
	mov	[title_h],eax
	fastcall CsdDipToPx,CSD_CONTENT_PAD_DIP,dword [paint_dpi]
	mov	[content_pad],eax
	fastcall CsdDipToPx,CSD_EDGE_DIP,dword [paint_dpi]
	mov	[edge_px],eax

	invoke	GetClientRect,[hMain],addr client_rect
	invoke	FillRect,rbx,addr client_rect,[hBodyBrush]

	invoke	CopyRect,addr title_rect,addr client_rect
	mov	eax,[title_rect.top]
	add	eax,[title_h]
	mov	[title_rect.bottom],eax
	invoke	FillRect,rbx,addr title_rect,[hTitleBrush]

	invoke	CopyRect,addr edge_rect,addr client_rect
	mov	eax,[edge_rect.left]
	add	eax,[edge_px]
	mov	[edge_rect.right],eax
	invoke	FillRect,rbx,addr edge_rect,[hEdgeBrush]

	invoke	CopyRect,addr edge_rect,addr client_rect
	mov	eax,[edge_rect.right]
	sub	eax,[edge_px]
	mov	[edge_rect.left],eax
	invoke	FillRect,rbx,addr edge_rect,[hEdgeBrush]

	invoke	CopyRect,addr edge_rect,addr client_rect
	mov	eax,[edge_rect.bottom]
	sub	eax,[edge_px]
	mov	[edge_rect.top],eax
	invoke	FillRect,rbx,addr edge_rect,[hEdgeBrush]

	fastcall DrawCaptionControls,rbx

	invoke	SetBkMode,rbx,TRANSPARENT
	mov	[old_bk],eax
	invoke	SelectObject,rbx,[hTextFont]
	mov	[old_font],rax
	invoke	SetTextColor,rbx,dword [active_window.current_theme.titleTextColor]
	mov	[old_color],eax
	invoke	DrawTextW,rbx,title_text,-1,addr caption_drag_rect,\
		DT_LEFT or DT_VCENTER or DT_SINGLELINE or DT_NOPREFIX

	invoke	CopyRect,addr body_rect,addr client_rect
	mov	eax,[body_rect.left]
	add	eax,[content_pad]
	mov	[body_rect.left],eax
	mov	eax,[body_rect.top]
	add	eax,[title_h]
	add	eax,[content_pad]
	mov	[body_rect.top],eax
	mov	eax,[body_rect.right]
	sub	eax,[content_pad]
	mov	[body_rect.right],eax
	mov	eax,[body_rect.bottom]
	sub	eax,[title_h]
	mov	[body_rect.bottom],eax
	invoke	SetTextColor,rbx,dword [active_window.current_theme.bodyTextColor]
	invoke	DrawTextW,rbx,body_text,-1,addr body_rect,\
		DT_LEFT or DT_TOP or DT_WORDBREAK or DT_NOPREFIX

	invoke	CopyRect,addr status_rect,addr client_rect
	mov	eax,[status_rect.left]
	add	eax,[content_pad]
	mov	[status_rect.left],eax
	mov	eax,[status_rect.right]
	sub	eax,[content_pad]
	mov	[status_rect.right],eax
	mov	eax,[status_rect.bottom]
	sub	eax,[title_h]
	add	eax,8
	mov	[status_rect.top],eax
	mov	eax,[active_window.client_rect.bottom]
	sub	eax,[edge_px]
	sub	eax,8
	mov	[status_rect.bottom],eax
	invoke	SetTextColor,rbx,dword [active_window.current_theme.statusTextColor]
	invoke	DrawTextW,rbx,addr status_buffer,-1,addr status_rect,\
		DT_LEFT or DT_BOTTOM or DT_SINGLELINE or DT_NOPREFIX

	invoke	SelectObject,rbx,[old_font]
	invoke	SetTextColor,rbx,dword [old_color]
	invoke	SetBkMode,rbx,dword [old_bk]
	ret
endp

proc ShowSystemMenu uses rbx, hwnd,lparam_value
    locals
	xpos		dd ?
	ypos		dd ?
	title_h		dd ?
	menu_command	dd ?
    endl

	mov	[hwnd],rcx
	cmp	edx,-1
	jne	menu_from_lparam
	fastcall CsdDipToPx,CSD_TITLE_HEIGHT_DIP,dword [current_dpi]
	mov	[title_h],eax
	invoke	GetWindowRect,[hwnd],addr window_rect
	mov	eax,[active_window.window_rect.left]
	mov	[xpos],eax
	mov	eax,[active_window.window_rect.top]
	add	eax,[title_h]
	mov	[ypos],eax
	jmp	menu_position_ready

  menu_from_lparam:
	mov	eax,edx
	movsx	ecx,ax
	sar	eax,16
	movsx	edx,ax
	mov	[xpos],ecx
	mov	[ypos],edx

  menu_position_ready:
	invoke	GetSystemMenu,[hwnd],0
	test	rax,rax
	jz	menu_done
	mov	rbx,rax
	fastcall CsdUpdateSystemMenuState,rbx,[hwnd]
	invoke	SetForegroundWindow,[hwnd]
	invoke	TrackPopupMenu,rbx,TPM_RETURNCMD or TPM_RIGHTBUTTON,\
		dword [xpos],dword [ypos],0,[hwnd],0
	mov	[menu_command],eax
	test	eax,eax
	jz	menu_done
	fastcall CsdDispatchSystemCommand,[hwnd],dword [menu_command]

  menu_done:
	ret
endp

proc HitTestCsdFrame hwnd,lparam_value
    locals
	xpos		dd ?
	ypos		dd ?
	client_x	dd ?
	client_y	dd ?
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

	mov	eax,[active_window.window_rect.left]
	add	eax,[resize_border_px]
	cmp	[xpos],eax
	jg	hittest_check_right
	mov	dword [left_hit],1

  hittest_check_right:
	mov	eax,[active_window.window_rect.right]
	sub	eax,[resize_border_px]
	cmp	[xpos],eax
	jl	hittest_check_top
	mov	dword [right_hit],1

  hittest_check_top:
	mov	eax,[active_window.window_rect.top]
	add	eax,[resize_border_px]
	cmp	[ypos],eax
	jg	hittest_check_bottom
	mov	dword [top_hit],1

  hittest_check_bottom:
	mov	eax,[active_window.window_rect.bottom]
	sub	eax,[resize_border_px]
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

	mov	eax,[xpos]
	sub	eax,[active_window.window_rect.left]
	mov	[client_x],eax
	mov	eax,[ypos]
	sub	eax,[active_window.window_rect.top]
	mov	[client_y],eax
	fastcall CaptionControlFromClientPoint,dword [client_x],dword [client_y]
	test	rax,rax
	jz	hittest_drag
	cmp	dword [rax+CSD_CAPTION_DESCRIPTOR.id],ID_CAPTION_MAX
	jne	hittest_control_default
	mov	eax,[active_window.snap_policy.maxHitCode]
	ret

  hittest_control_default:
	mov	eax,[rax+CSD_CAPTION_DESCRIPTOR.hitCode]
	ret

  hittest_drag:
	mov	eax,[client_x]
	cmp	eax,[active_window.caption_drag_rect.left]
	jl	hittest_client
	cmp	eax,[active_window.caption_drag_rect.right]
	jge	hittest_client
	mov	eax,[client_y]
	cmp	eax,[active_window.caption_drag_rect.top]
	jl	hittest_client
	cmp	eax,[active_window.caption_drag_rect.bottom]
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

proc HandleCaptionClientClick uses rbx, hwnd,lparam_value
    locals
	xpos		dd ?
	ypos		dd ?
	menu_point	POINT
	menu_lparam	dd ?
	sys_command	dd ?
    endl

	mov	[hwnd],rcx
	mov	eax,edx
	movsx	ecx,ax
	sar	eax,16
	movsx	edx,ax
	mov	[xpos],ecx
	mov	[ypos],edx
	fastcall CaptionControlFromClientPoint,dword [xpos],dword [ypos]
	test	rax,rax
	jz	caption_click_no
	mov	rbx,rax
	test	[rbx+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_SYSTEM
	jz	caption_click_not_system

	mov	eax,[xpos]
	mov	[menu_point.x],eax
	mov	eax,[ypos]
	mov	[menu_point.y],eax
	invoke	ClientToScreen,[hwnd],addr menu_point
	mov	eax,[menu_point.y]
	shl	eax,16
	movzx	edx,word [menu_point.x]
	or	edx,eax
	mov	[menu_lparam],edx
	fastcall ShowSystemMenu,[hwnd],dword [menu_lparam]
	mov	eax,1
	ret

  caption_click_not_system:
	test	[rbx+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_SYSCMD
	jz	caption_click_not_syscmd

	mov	eax,[rbx+CSD_CAPTION_DESCRIPTOR.command]
	mov	[sys_command],eax
	cmp	[rbx+CSD_CAPTION_DESCRIPTOR.id],ID_CAPTION_MAX
	jne	caption_click_post_syscmd
	invoke	IsZoomed,[hwnd]
	test	eax,eax
	jz	caption_click_post_syscmd
	mov	dword [sys_command],SC_RESTORE

  caption_click_post_syscmd:
	fastcall CsdDispatchSystemCommand,[hwnd],dword [sys_command]
	mov	eax,1
	ret

  caption_click_not_syscmd:
	test	[rbx+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_APP
	jz	caption_click_no

	mov	edx,[rbx+CSD_CAPTION_DESCRIPTOR.command]
	cmp	edx,ID_CAPTION_SEARCH
	je	caption_click_search
	cmp	edx,ID_CAPTION_SETTINGS
	je	caption_click_settings
	jmp	caption_click_no

  caption_click_search:
	invoke	lstrcpyW,addr status_buffer,status_edit
	jmp	caption_click_redraw

  caption_click_settings:
	invoke	lstrcpyW,addr status_buffer,status_settings

  caption_click_redraw:
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret

  caption_click_no:
	xor	eax,eax
	ret
endp

proc HandleCaptionMouseMove hwnd,wparam,lparam
    locals
	xpos	dd ?
	ypos	dd ?
	index	dd ?
    endl

	mov	[hwnd],rcx
	mov	eax,r8d
	movsx	ecx,ax
	sar	eax,16
	movsx	edx,ax
	mov	[xpos],ecx
	mov	[ypos],edx

	fastcall CaptionIndexFromClientPoint,dword [xpos],dword [ypos]
	mov	[index],eax
	fastcall CsdCaptionTrackMouseLeave,[hwnd],addr caption_input
	fastcall CsdCaptionSetHot,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [index]
	test	eax,eax
	jz	mousemove_done
	invoke	InvalidateRect,[hwnd],0,1

  mousemove_done:
	ret
endp

proc HandleCaptionMouseLeave hwnd
	mov	[hwnd],rcx
	fastcall CsdCaptionMouseLeave,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT
	test	eax,eax
	jz	mouseleave_done
	invoke	InvalidateRect,[hwnd],0,1

  mouseleave_done:
	ret
endp

proc HandleCaptionLButtonDown hwnd,wparam,lparam
    locals
	xpos	dd ?
	ypos	dd ?
	index	dd ?
    endl

	mov	[hwnd],rcx
	mov	eax,r8d
	movsx	ecx,ax
	sar	eax,16
	movsx	edx,ax
	mov	[xpos],ecx
	mov	[ypos],edx

	fastcall CaptionIndexFromClientPoint,dword [xpos],dword [ypos]
	mov	[index],eax
	cmp	eax,CSD_CAPTION_NO_INDEX
	je	lbuttondown_done
	invoke	SetCapture,[hwnd]
	fastcall CsdCaptionTrackMouseLeave,[hwnd],addr caption_input
	fastcall CsdCaptionSetHot,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [index]
	fastcall CsdCaptionSetPressed,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [index]
	invoke	InvalidateRect,[hwnd],0,1

  lbuttondown_done:
	ret
endp

proc HandleCaptionLButtonUp hwnd,wparam,lparam
    locals
	xpos		dd ?
	ypos		dd ?
	index		dd ?
	pressed_index	dd ?
	changed		dd ?
	lparam_value	dd ?
    endl

	mov	[hwnd],rcx
	mov	[lparam_value],r8d
	mov	eax,r8d
	movsx	ecx,ax
	sar	eax,16
	movsx	edx,ax
	mov	[xpos],ecx
	mov	[ypos],edx

	fastcall CaptionIndexFromClientPoint,dword [xpos],dword [ypos]
	mov	[index],eax
	mov	eax,[active_window.caption_input.pressedIndex]
	mov	[pressed_index],eax
	mov	dword [changed],0
	invoke	ReleaseCapture
	fastcall CsdCaptionSetHot,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [index]
	or	dword [changed],eax
	fastcall CsdCaptionSetPressed,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,CSD_CAPTION_NO_INDEX
	or	dword [changed],eax
	cmp	dword [changed],0
	je	lbuttonup_check_command
	invoke	InvalidateRect,[hwnd],0,1

  lbuttonup_check_command:
	cmp	dword [pressed_index],CSD_CAPTION_NO_INDEX
	je	lbuttonup_done
	mov	eax,[pressed_index]
	cmp	eax,[index]
	jne	lbuttonup_done
	fastcall HandleCaptionClientClick,[hwnd],dword [lparam_value]

  lbuttonup_done:
	ret
endp

proc HandleCaptionActivate hwnd,wparam
    locals
	active	dd ?
    endl

	mov	[hwnd],rcx
	movzx	eax,dx
	mov	dword [active],1
	cmp	eax,WA_INACTIVE
	jne	activate_apply
	mov	dword [active],0
	invoke	ReleaseCapture

  activate_apply:
	fastcall CsdCaptionSetActive,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [active]
	test	eax,eax
	jz	activate_done
	invoke	InvalidateRect,[hwnd],0,1

  activate_done:
	ret
endp

proc HandleCommand hwnd,wparam,lparam
	mov	[hwnd],rcx
	mov	eax,edx
	mov	ecx,eax
	and	ecx,0FFFFh
	shr	eax,16
	cmp	ecx,ID_CAPTION_SEARCH
	jne	command_no
	cmp	eax,EN_CHANGE
	jne	command_no
	cmp	r8,[hSearchEdit]
	jne	command_no
	invoke	lstrcpyW,addr status_buffer,status_edit
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret

  command_no:
	xor	eax,eax
	ret
endp

proc HandleCtlColorEdit hdc,controlHwnd
	mov	[hdc],rcx
	cmp	rdx,[hSearchEdit]
	jne	ctlcolor_no
	invoke	SetTextColor,[hdc],dword [active_window.current_theme.bodyTextColor]
	invoke	SetBkColor,[hdc],dword [active_window.current_theme.bodyBackColor]
	mov	rax,[hBodyBrush]
	ret

  ctlcolor_no:
	xor	eax,eax
	ret
endp

proc ClientLParamFromScreenLParam hwnd,lparam_value
    locals
	pt POINT
    endl

	mov	[hwnd],rcx
	mov	eax,edx
	movsx	ecx,ax
	sar	eax,16
	movsx	edx,ax
	mov	[pt.x],ecx
	mov	[pt.y],edx
	invoke	ScreenToClient,[hwnd],addr pt
	mov	eax,[pt.y]
	shl	eax,16
	movzx	edx,word [pt.x]
	or	eax,edx
	ret
endp

proc HandleSnapNcMouseMove hwnd,wparam,lparam
    locals
	client_lparam	dd ?
	xpos		dd ?
	ypos		dd ?
	index		dd ?
    endl

	mov	[hwnd],rcx
	cmp	edx,HTMAXBUTTON
	jne	ncmousemove_no
	fastcall ClientLParamFromScreenLParam,[hwnd],r8d
	mov	[client_lparam],eax
	movsx	ecx,ax
	sar	eax,16
	movsx	edx,ax
	mov	[xpos],ecx
	mov	[ypos],edx
	fastcall CaptionIndexFromClientPoint,dword [xpos],dword [ypos]
	mov	[index],eax
	cmp	eax,CSD_CAPTION_MAX_INDEX
	jne	ncmousemove_no

	fastcall CsdCaptionTrackMouseLeaveWithFlags,[hwnd],addr caption_input,\
		TME_LEAVE or TME_NONCLIENT
	fastcall CsdCaptionSetHot,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [index]
	test	eax,eax
	jz	ncmousemove_handled
	invoke	InvalidateRect,[hwnd],0,1

  ncmousemove_handled:
	mov	eax,1
	ret

  ncmousemove_no:
	xor	eax,eax
	ret
endp

proc HandleSnapNcLButtonDown hwnd,wparam,lparam
    locals
	client_lparam dd ?
    endl

	mov	[hwnd],rcx
	cmp	edx,HTMAXBUTTON
	jne	nclbuttondown_no
	fastcall ClientLParamFromScreenLParam,[hwnd],r8d
	mov	[client_lparam],eax
	fastcall HandleCaptionLButtonDown,[hwnd],MK_LBUTTON,dword [client_lparam]
	mov	eax,1
	ret

  nclbuttondown_no:
	xor	eax,eax
	ret
endp

proc HandleSnapNcLButtonUp hwnd,wparam,lparam
    locals
	client_lparam dd ?
    endl

	mov	[hwnd],rcx
	cmp	edx,HTMAXBUTTON
	jne	nclbuttonup_no
	fastcall ClientLParamFromScreenLParam,[hwnd],r8d
	mov	[client_lparam],eax
	fastcall HandleCaptionLButtonUp,[hwnd],0,dword [client_lparam]
	mov	eax,1
	ret

  nclbuttonup_no:
	xor	eax,eax
	ret
endp

proc HandleDpiChanged uses rbx, hwnd,wparam,lparam
	mov	[hwnd],rcx
	mov	rbx,r8
	movzx	ecx,dx
	fastcall SetDpiState,ecx
	fastcall CsdApplySuggestedDpiRect,[hwnd],rbx
	fastcall RebuildCaptionFont
	fastcall RebuildTextFont
	fastcall LayoutCaptionControls
	fastcall LayoutCaptionChildren
	invoke	InvalidateRect,[hwnd],0,1
	ret
endp

proc PrepareWindowRect widthDip,heightDip
    locals
	dpi	dd ?
	width_value dd ?
	height_value dd ?
    endl

	mov	[width_value],ecx
	mov	[height_value],edx
	invoke	GetDesktopWindow
	fastcall CsdGetWindowDpi,rax
	mov	[dpi],eax

	mov	dword [initial_rect.left],0
	mov	dword [initial_rect.top],0
	fastcall CsdDipToPx,dword [width_value],dword [dpi]
	mov	[initial_rect.right],eax
	fastcall CsdDipToPx,dword [height_value],dword [dpi]
	mov	[initial_rect.bottom],eax
	fastcall CsdAdjustWindowRectForDpi,addr initial_rect,\
		WS_OVERLAPPEDWINDOW or WS_CLIPCHILDREN,WS_EX_APPWINDOW,dword [dpi]
	ret
endp

proc PrepareInitialWindowRect
	fastcall PrepareWindowRect,CSD_INITIAL_CLIENT_W_DIP,CSD_INITIAL_CLIENT_H_DIP
	ret
endp

proc CreateOwnedToolFrame uses rbx
    locals
	window_w dd ?
	window_h dd ?
	tool_hwnd dq ?
    endl

	mov	rbx,[active_windowp]
	fastcall CsdWindowSave,rbx
	fastcall PrepareWindowRect,CSD_TOOL_CLIENT_W_DIP,CSD_TOOL_CLIENT_H_DIP
	mov	eax,[initial_rect.right]
	sub	eax,[initial_rect.left]
	mov	[window_w],eax
	mov	eax,[initial_rect.bottom]
	sub	eax,[initial_rect.top]
	mov	[window_h],eax

	invoke	CreateWindowEx,WS_EX_TOOLWINDOW,class_name,tool_name,\
		WS_OVERLAPPEDWINDOW or WS_CLIPCHILDREN,\
		CW_USEDEFAULT,CW_USEDEFAULT,dword [window_w],dword [window_h],\
		[hMain],0,[hInstance],CSD_WINDOW_TOOL
	mov	[tool_hwnd],rax
	test	rax,rax
	jz	create_tool_restore
	invoke	ShowWindow,[tool_hwnd],SW_SHOWNORMAL
	invoke	UpdateWindow,[tool_hwnd]

  create_tool_restore:
	fastcall CsdWindowLoad,rbx
	ret
endp

proc WindowProc uses rbx, hwnd,wmsg,wparam,lparam
    locals
	ps	PAINTSTRUCT
	contextp dq ?
	quit_after_destroy dd ?
    endl

	mov	[hwnd],rcx
	mov	dword [wmsg],edx
	mov	[wparam],r8
	mov	[lparam],r9
	mov	qword [contextp],0
	cmp	edx,WM_NCCREATE
	je	wnd_dispatch
	invoke	GetWindowLongPtrW,[hwnd],GWLP_USERDATA
	mov	[contextp],rax
	test	rax,rax
	jz	wnd_dispatch
	fastcall CsdWindowLoad,rax

  wnd_dispatch:
	iterate <message,branch>,\
		WM_NCCREATE,			wnd_nccreate,\
		WM_NCCALCSIZE,			wnd_nccalcsize,\
		WM_NCPAINT,			wnd_ncpaint,\
		WM_NCACTIVATE,			wnd_ncactivate,\
		WM_ACTIVATE,			wnd_activate,\
		WM_NCHITTEST,			wnd_nchittest,\
		WM_CREATE,			wnd_create,\
		WM_SIZE,			wnd_size,\
		WM_DPICHANGED,			wnd_dpichanged,\
		WM_SETTINGCHANGE,		wnd_theme_changed,\
		WM_DWMCOLORIZATIONCOLORCHANGED,	wnd_theme_changed,\
		WM_MOUSEMOVE,			wnd_mousemove,\
		WM_MOUSELEAVE,			wnd_mouseleave,\
		WM_NCMOUSEMOVE,			wnd_ncmousemove,\
		WM_NCMOUSELEAVE,		wnd_ncmouseleave,\
		WM_LBUTTONDOWN,			wnd_lbuttondown,\
		WM_LBUTTONUP,			wnd_lbuttonup,\
		WM_NCLBUTTONDOWN,		wnd_nclbuttondown,\
		WM_NCLBUTTONUP,			wnd_nclbuttonup,\
		WM_NCRBUTTONUP,			wnd_ncrbuttonup,\
		WM_CONTEXTMENU,			wnd_contextmenu,\
		WM_INITMENU,			wnd_initmenu,\
		WM_INITMENUPOPUP,		wnd_initmenupopup,\
		WM_COMMAND,			wnd_command,\
		WM_CTLCOLOREDIT,		wnd_ctlcoloredit,\
		WM_KEYDOWN,			wnd_keydown,\
		WM_ERASEBKGND,			wnd_erasebkgnd,\
		WM_PAINT,			wnd_paint,\
		WM_DESTROY,			wnd_destroy,\
		WM_NCDESTROY,			wnd_ncdestroy

		cmp	dword [wmsg],message
		je	branch
	end iterate

  wnd_default:
	invoke	DefWindowProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret

  wnd_nccreate:
	mov	rbx,[lparam]
	mov	ecx,dword [rbx+CREATESTRUCT.lpCreateParams]
	mov	rdx,[rbx+CREATESTRUCT.hwndParent]
	fastcall CsdWindowAlloc,ecx,rdx
	test	rax,rax
	jz	wnd_nccreate_fail
	mov	[contextp],rax
	mov	rcx,[hwnd]
	mov	[rax+CSD_WINDOW.hwnd],rcx
	fastcall CsdWindowRegister,rax
	test	eax,eax
	jz	wnd_nccreate_free
	invoke	SetWindowLongPtr,[hwnd],GWLP_USERDATA,[contextp]
	fastcall CsdWindowLoad,[contextp]
	mov	eax,1
	ret

  wnd_nccreate_free:
	fastcall CsdWindowFree,[contextp]

  wnd_nccreate_fail:
	xor	eax,eax
	ret

  wnd_nccalcsize:
	cmp	qword [wparam],0
	je	wnd_nccalc_done
	mov	eax,0300h		; WVR_REDRAW
	ret

  wnd_nccalc_done:
	xor	eax,eax
	ret

  wnd_ncpaint:
	xor	eax,eax
	ret

  wnd_ncactivate:
	mov	eax,1
	ret

  wnd_activate:
	fastcall HandleCaptionActivate,[hwnd],r8
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_nchittest:
	fastcall HitTestCsdFrame,[hwnd],r9
	ret

  wnd_create:
	invoke	DwmExtendFrameIntoClientArea,[hwnd],addr dwm_margins
	fastcall RefreshDpiState,[hwnd]
	fastcall CsdSnapProbePolicy,addr snap_policy
	fastcall CreatePaintObjects
	fastcall LayoutCaptionControls
	fastcall CreateCaptionChildren,[hwnd]
	fastcall LayoutCaptionChildren
	fastcall CsdCaptionInputInit,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT
	fastcall UpdateThemeStatus
	fastcall CsdRefreshSystemMenuState,[hwnd]
	test	dword [active_window.flags],CSD_WINDOW_MAIN
	jz	wnd_create_save
	fastcall CreateOwnedToolFrame

  wnd_create_save:
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_size:
	fastcall LayoutCaptionControls
	fastcall LayoutCaptionChildren
	fastcall CsdRefreshSystemMenuState,[hwnd]
	invoke	InvalidateRect,[hwnd],0,1
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_dpichanged:
	fastcall HandleDpiChanged,[hwnd],r8,r9
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_theme_changed:
	fastcall CsdWindowRefreshAll
	xor	eax,eax
	ret

  wnd_mousemove:
	fastcall HandleCaptionMouseMove,[hwnd],r8,r9
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_mouseleave:
	fastcall HandleCaptionMouseLeave,[hwnd]
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_ncmousemove:
	fastcall HandleSnapNcMouseMove,[hwnd],r8,r9
	test	eax,eax
	jz	wnd_default
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_ncmouseleave:
	fastcall HandleCaptionMouseLeave,[hwnd]
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_lbuttondown:
	fastcall HandleCaptionLButtonDown,[hwnd],r8,r9
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_lbuttonup:
	fastcall HandleCaptionLButtonUp,[hwnd],r8,r9
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_nclbuttondown:
	fastcall HandleSnapNcLButtonDown,[hwnd],r8,r9
	test	eax,eax
	jz	wnd_default
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_nclbuttonup:
	fastcall HandleSnapNcLButtonUp,[hwnd],r8,r9
	test	eax,eax
	jz	wnd_default
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_ncrbuttonup:
	cmp	r8d,HTCAPTION
	je	wnd_show_system_menu
	cmp	r8d,HTSYSMENU
	je	wnd_show_system_menu
	jmp	wnd_default

  wnd_contextmenu:
	cmp	r9d,-1
	jne	wnd_default

  wnd_show_system_menu:
	fastcall ShowSystemMenu,[hwnd],r9
	xor	eax,eax
	ret

  wnd_initmenu:
	fastcall CsdUpdateSystemMenuState,r8,[hwnd]
	xor	eax,eax
	ret

  wnd_initmenupopup:
	test	r9d,0FFFF0000h
	jz	wnd_default
	fastcall CsdUpdateSystemMenuState,r8,[hwnd]
	xor	eax,eax
	ret

  wnd_command:
	fastcall HandleCommand,[hwnd],r8,r9
	fastcall CsdWindowSave,[contextp]
	xor	eax,eax
	ret

  wnd_ctlcoloredit:
	fastcall HandleCtlColorEdit,r8,r9
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
	test	dword [active_window.flags],CSD_WINDOW_MAIN
	jz	wnd_destroy_no_quit
	fastcall CsdWindowDestroyOwned,[hwnd]
	invoke	PostQuitMessage,0

  wnd_destroy_no_quit:
	xor	eax,eax
	ret

  wnd_ncdestroy:
	mov	rbx,[contextp]
	test	rbx,rbx
	jz	wnd_ncdestroy_default
	mov	dword [quit_after_destroy],0
	test	dword [rbx+CSD_WINDOW.flags],CSD_WINDOW_MAIN
	jz	wnd_ncdestroy_release
	mov	dword [quit_after_destroy],1
	fastcall CsdWindowDestroyOwned,[hwnd]
	fastcall CsdWindowLoad,rbx

  wnd_ncdestroy_release:
	fastcall DestroyCaptionChildren
	fastcall DestroyPaintObjects
	fastcall CsdWindowUnregister,rbx
	invoke	SetWindowLongPtr,[hwnd],GWLP_USERDATA,0
	fastcall CsdWindowFree,rbx
	mov	qword [contextp],0
	cmp	dword [quit_after_destroy],0
	je	wnd_ncdestroy_default
	invoke	PostQuitMessage,0
	jmp	wnd_default

  wnd_ncdestroy_default:
	jmp	wnd_default
endp

proc start
    locals
	window_w dd ?
	window_h dd ?
	main_hwnd dq ?
    endl

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

	fastcall PrepareInitialWindowRect
	mov	eax,[initial_rect.right]
	sub	eax,[initial_rect.left]
	mov	[window_w],eax
	mov	eax,[initial_rect.bottom]
	sub	eax,[initial_rect.top]
	mov	[window_h],eax

	invoke	CreateWindowEx,WS_EX_APPWINDOW,class_name,app_name,\
		WS_OVERLAPPEDWINDOW or WS_CLIPCHILDREN,\
		CW_USEDEFAULT,CW_USEDEFAULT,dword [window_w],dword [window_h],\
		0,0,[hInstance],CSD_WINDOW_MAIN
	mov	[main_hwnd],rax
	test	rax,rax
	jz	start_fatal_window

	invoke	ShowWindow,[main_hwnd],SW_SHOWDEFAULT
	invoke	UpdateWindow,[main_hwnd]

  start_message_loop:
	fastcall CsdWindowAnyLive
	test	eax,eax
	jz	start_exit_empty
	invoke	GetMessageW,addr msg,0,0,0
	test	eax,eax
	jle	start_exit
	invoke	TranslateMessage,addr msg
	invoke	DispatchMessageW,addr msg
	jmp	start_message_loop

  start_exit:
	invoke	ExitProcess,[msg.wParam]

  start_exit_empty:
	invoke	ExitProcess,0

  start_fatal_register:
	invoke	MessageBox,0,'RegisterClassExW failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1

  start_fatal_window:
	invoke	MessageBox,0,'CreateWindowExW failed.',app_name,MB_ICONERROR
	invoke	ExitProcess,1
endp
