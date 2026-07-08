; 08_snap_layouts.asm - opt into Win11 snap-layout max-button hit-testing.
;
; This keeps 07 dynamic caption state and version-gates the maximize glyph so
; Windows 11 can see HTMAXBUTTON for the snap flyout while down-level builds keep
; the fully client-owned command path.

ADDON_WINDOWS_RESOURCE equ '08_snap_layouts.res'

include 'addon\windows.inc'
include '..\font_icons\font_icons.inc'
include 'addon\csd\caption.inc'
include 'addon\csd\dpi.inc'
include 'addon\csd\theme.inc'
include 'addon\csd\state.inc'
include 'addon\csd\snap.inc'

CSD_TITLE_HEIGHT_DIP	= 56
CSD_CONTENT_PAD_DIP	= 26
CSD_EDGE_DIP		= 4
CSD_BUTTON_WIDTH_DIP	= 44
CSD_ICON_DIP		= 18
CSD_DRAG_PAD_DIP	= CSD_CONTENT_PAD_DIP / 2
CSD_INITIAL_CLIENT_W_DIP = 920
CSD_INITIAL_CLIENT_H_DIP = 500
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
CSD_CAPTION_MAX_INDEX	= 4

define __GLOBAL_DATA__ csd_data
define __GLOBAL_BSS__ csd_bss

macro csd_data
	app_name	du 'CSD 08 - snap layouts',0
	class_name	du 'Fasm2CsdSnapLayouts',0
	title_text	du '08  Snap-layout max button policy',0
	body_text	du 'This version keeps 07 dynamic state and adds a version-gated HTMAXBUTTON policy for the maximize glyph.',13,10,13,10
			du 'On Windows 11, WM_NCHITTEST returns HTMAXBUTTON over the owner-drawn maximize button so the shell can show snap layouts. Down-level builds keep HTCLIENT. WM_NCLBUTTONDOWN is handled by this example, not DefWindowProc.',0
	status_format	du 'Snap policy %s | OS build %u',0
	status_search	du 'Search caption button clicked.',0
	status_settings	du 'Settings caption button clicked.',0
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
			glyph: ICON_SEARCH, command: ID_CAPTION_SEARCH,\
			hitCode: HTCLIENT,\
			flags: CSD_CAPTION_ALIGN_LEADING or CSD_CAPTION_APP,\
			widthDip: CSD_BUTTON_WIDTH_DIP
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

	align 8
	status_message	dq status_buffer
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
	hBodyBrush	dq ?
	hTitleBrush	dq ?
	hButtonBrush	dq ?
	hButtonHoverBrush dq ?
	hButtonPressedBrush dq ?
	hButtonInactiveBrush dq ?
	hCloseBrush	dq ?
	hCloseHoverBrush dq ?
	hClosePressedBrush dq ?
	hEdgeBrush	dq ?
	hCaptionFont	dq ?
	hTextFont	dq ?
	msg		MSG
	client_rect	RECT
	window_rect	RECT
	initial_rect	RECT
	caption_drag_rect RECT
	current_dpi	dd ?
	resize_border_px dd ?
	current_theme	CSD_THEME
	caption_input	CSD_CAPTION_INPUT
	snap_policy	CSD_SNAP_POLICY
	status_buffer	rw 128
	caption_geometry rb sizeof.CSD_CAPTION_GEOMETRY * CSD_CAPTION_CONTROL_COUNT
	caption_state	rb sizeof.CSD_CAPTION_STATE * CSD_CAPTION_CONTROL_COUNT
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
		hBodyBrush,		dword [current_theme.bodyBackColor],\
		hTitleBrush,		dword [current_theme.captionColor],\
		hButtonBrush,		dword [current_theme.buttonColor],\
		hButtonHoverBrush,	CSD_STATE_BUTTON_HOVER,\
		hButtonPressedBrush,	CSD_STATE_BUTTON_PRESSED,\
		hButtonInactiveBrush,	CSD_STATE_BUTTON_INACTIVE,\
		hCloseBrush,		dword [current_theme.closeColor],\
		hCloseHoverBrush,	CSD_STATE_CLOSE_HOVER,\
		hClosePressedBrush,	CSD_STATE_CLOSE_PRESSED,\
		hEdgeBrush,		dword [current_theme.borderColor]

		invoke	CreateSolidBrush,color
		mov	[handle],rax
	end iterate
	ret
endp

proc UpdateThemeStatus uses rbx
	mov	rbx,snap_off
	cmp	dword [snap_policy.snapCapable],0
	je	status_snap_ready
	mov	rbx,snap_on
  status_snap_ready:
	invoke	wsprintfW,addr status_buffer,status_format,\
		rbx,\
		dword [snap_policy.osBuild]
	mov	rax,status_buffer
	mov	[status_message],rax
	ret
endp

proc RefreshThemeAndFrame hwnd
	mov	[hwnd],rcx
	fastcall CsdThemeRefresh,addr current_theme
	fastcall CsdThemeApplyDwm,[hwnd],addr current_theme
	fastcall RebuildThemeBrushes
	fastcall UpdateThemeStatus
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

proc LayoutCaptionControls
	invoke	GetClientRect,[hMain],addr client_rect
	fastcall CsdCaptionLayout,caption_descriptors,caption_geometry,\
		CSD_CAPTION_CONTROL_COUNT,addr client_rect,dword [current_dpi],\
		CSD_TITLE_HEIGHT_DIP,CSD_EDGE_DIP,CSD_DRAG_PAD_DIP,\
		addr caption_drag_rect
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
	movzx	eax,byte [r15+CSD_CAPTION_STATE.value]
	mov	[state_value],eax
	mov	eax,[current_theme.titleTextColor]
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
	invoke	SetTextColor,rbx,dword [current_theme.titleTextColor]
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
	invoke	SetTextColor,rbx,dword [current_theme.bodyTextColor]
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
	mov	eax,[client_rect.bottom]
	sub	eax,[edge_px]
	sub	eax,8
	mov	[status_rect.bottom],eax
	invoke	SetTextColor,rbx,dword [current_theme.statusTextColor]
	invoke	DrawTextW,rbx,[status_message],-1,addr status_rect,\
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
	mov	eax,[window_rect.left]
	mov	[xpos],eax
	mov	eax,[window_rect.top]
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

	mov	eax,[window_rect.left]
	add	eax,[resize_border_px]
	cmp	[xpos],eax
	jg	hittest_check_right
	mov	dword [left_hit],1

  hittest_check_right:
	mov	eax,[window_rect.right]
	sub	eax,[resize_border_px]
	cmp	[xpos],eax
	jl	hittest_check_top
	mov	dword [right_hit],1

  hittest_check_top:
	mov	eax,[window_rect.top]
	add	eax,[resize_border_px]
	cmp	[ypos],eax
	jg	hittest_check_bottom
	mov	dword [top_hit],1

  hittest_check_bottom:
	mov	eax,[window_rect.bottom]
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
	sub	eax,[window_rect.left]
	mov	[client_x],eax
	mov	eax,[ypos]
	sub	eax,[window_rect.top]
	mov	[client_y],eax
	fastcall CaptionControlFromClientPoint,dword [client_x],dword [client_y]
	test	rax,rax
	jz	hittest_drag
	cmp	dword [rax+CSD_CAPTION_DESCRIPTOR.id],ID_CAPTION_MAX
	jne	hittest_control_default
	mov	eax,[snap_policy.maxHitCode]
	ret

  hittest_control_default:
	mov	eax,[rax+CSD_CAPTION_DESCRIPTOR.hitCode]
	ret

  hittest_drag:
	mov	eax,[client_x]
	cmp	eax,[caption_drag_rect.left]
	jl	hittest_client
	cmp	eax,[caption_drag_rect.right]
	jge	hittest_client
	mov	eax,[client_y]
	cmp	eax,[caption_drag_rect.top]
	jl	hittest_client
	cmp	eax,[caption_drag_rect.bottom]
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
	mov	rax,status_search
	mov	[status_message],rax
	jmp	caption_click_redraw

  caption_click_settings:
	mov	rax,status_settings
	mov	[status_message],rax

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
	mov	eax,[caption_input.pressedIndex]
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
	invoke	InvalidateRect,[hwnd],0,1
	ret
endp

proc PrepareInitialWindowRect
    locals
	dpi	dd ?
    endl

	invoke	GetDesktopWindow
	fastcall CsdGetWindowDpi,rax
	mov	[dpi],eax

	mov	dword [initial_rect.left],0
	mov	dword [initial_rect.top],0
	fastcall CsdDipToPx,CSD_INITIAL_CLIENT_W_DIP,dword [dpi]
	mov	[initial_rect.right],eax
	fastcall CsdDipToPx,CSD_INITIAL_CLIENT_H_DIP,dword [dpi]
	mov	[initial_rect.bottom],eax
	fastcall CsdAdjustWindowRectForDpi,addr initial_rect,\
		WS_OVERLAPPEDWINDOW,WS_EX_APPWINDOW,dword [dpi]
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
		WM_KEYDOWN,			wnd_keydown,\
		WM_ERASEBKGND,			wnd_erasebkgnd,\
		WM_PAINT,			wnd_paint,\
		WM_DESTROY,			wnd_destroy

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

  wnd_ncpaint:
	xor	eax,eax
	ret

  wnd_ncactivate:
	mov	eax,1
	ret

  wnd_activate:
	fastcall HandleCaptionActivate,[hwnd],r8
	xor	eax,eax
	ret

  wnd_nchittest:
	fastcall HitTestCsdFrame,[hwnd],r9
	ret

  wnd_create:
	mov	rax,[hwnd]
	mov	[hMain],rax
	invoke	DwmExtendFrameIntoClientArea,[hwnd],addr dwm_margins
	fastcall RefreshDpiState,[hwnd]
	fastcall CsdSnapProbePolicy,addr snap_policy
	fastcall CreatePaintObjects
	fastcall LayoutCaptionControls
	fastcall CsdCaptionInputInit,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT
	fastcall CsdRefreshSystemMenuState,[hwnd]
	xor	eax,eax
	ret

  wnd_size:
	fastcall LayoutCaptionControls
	fastcall CsdRefreshSystemMenuState,[hwnd]
	invoke	InvalidateRect,[hwnd],0,1
	xor	eax,eax
	ret

  wnd_dpichanged:
	fastcall HandleDpiChanged,[hwnd],r8,r9
	xor	eax,eax
	ret

  wnd_theme_changed:
	fastcall RefreshThemeAndFrame,[hwnd]
	xor	eax,eax
	ret

  wnd_mousemove:
	fastcall HandleCaptionMouseMove,[hwnd],r8,r9
	xor	eax,eax
	ret

  wnd_mouseleave:
	fastcall HandleCaptionMouseLeave,[hwnd]
	xor	eax,eax
	ret

  wnd_ncmousemove:
	fastcall HandleSnapNcMouseMove,[hwnd],r8,r9
	test	eax,eax
	jz	wnd_default
	xor	eax,eax
	ret

  wnd_ncmouseleave:
	fastcall HandleCaptionMouseLeave,[hwnd]
	xor	eax,eax
	ret

  wnd_lbuttondown:
	fastcall HandleCaptionLButtonDown,[hwnd],r8,r9
	xor	eax,eax
	ret

  wnd_lbuttonup:
	fastcall HandleCaptionLButtonUp,[hwnd],r8,r9
	xor	eax,eax
	ret

  wnd_nclbuttondown:
	fastcall HandleSnapNcLButtonDown,[hwnd],r8,r9
	test	eax,eax
	jz	wnd_default
	xor	eax,eax
	ret

  wnd_nclbuttonup:
	fastcall HandleSnapNcLButtonUp,[hwnd],r8,r9
	test	eax,eax
	jz	wnd_default
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
    locals
	window_w dd ?
	window_h dd ?
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
		WS_OVERLAPPEDWINDOW,\
		CW_USEDEFAULT,CW_USEDEFAULT,dword [window_w],dword [window_h],\
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
