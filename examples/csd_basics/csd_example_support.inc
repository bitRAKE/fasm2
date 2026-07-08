; csd_example_support.inc - local support shared by the tabbed CSD examples.
;
; This file is intentionally scoped to examples/csd_basics. It binds to the
; demo globals used by 13_caption_tabs, 14_responsive_caption, and
; 15_rtl_caption, so it is not part of the reusable include/addon/csd package.
; The examples still own their caption descriptors, tab layout policy, UIA tree,
; overflow policy, RTL policy, rendering shape, and command routing.

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

proc BackdropNameForType requestedType
	cmp	ecx,DWMSBT_MAINWINDOW
	je	backdrop_name_mica
	cmp	ecx,DWMSBT_TRANSIENTWINDOW
	je	backdrop_name_acrylic
	cmp	ecx,DWMSBT_TABBEDWINDOW
	je	backdrop_name_mica_alt
	mov	rax,backdrop_none
	ret

  backdrop_name_mica:
	mov	rax,backdrop_mica
	ret

  backdrop_name_acrylic:
	mov	rax,backdrop_acrylic
	ret

  backdrop_name_mica_alt:
	mov	rax,backdrop_mica_alt
	ret
endp

proc BackdropCapabilityLabel
	cmp	dword [backdrop_policy.capable],0
	je	backdrop_capability_no
	mov	rax,backdrop_available
	ret

  backdrop_capability_no:
	mov	rax,backdrop_unavailable
	ret
endp

proc RefreshThemeAndFrame hwnd
	mov	[hwnd],rcx
	fastcall CsdThemeRefresh,addr current_theme
	fastcall CsdThemeApplyDwm,[hwnd],addr current_theme
	fastcall CsdBackdropApply,[hwnd],addr backdrop_policy
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
	fastcall DestroyThemeBrushes
	iterate handle, hCaptionFont,hTextFont
		mov	rcx,[handle]
		jrcxz	destroy_paint_#handle
		invoke	DeleteObject,rcx
		mov	qword [handle],0
	  destroy_paint_#handle:
	end iterate
	ret
endp

proc AddCaptionDragRect left,top,right,bottom
	cmp	ecx,r8d
	jge	add_drag_done
	cmp	edx,r9d
	jge	add_drag_done
	cmp	dword [caption_drag_rect_count],CSD_TAB_DRAG_RECT_MAX
	jae	add_drag_done
	mov	eax,[caption_drag_rect_count]
	imul	eax,sizeof.RECT
	lea	rax,[caption_drag_rects+rax]
	mov	[rax+RECT.left],ecx
	mov	[rax+RECT.top],edx
	mov	[rax+RECT.right],r8d
	mov	[rax+RECT.bottom],r9d
	inc	dword [caption_drag_rect_count]

  add_drag_done:
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

proc CsdBlendColorref fromColor,toColor,phase
	test	r8d,r8d
	jnz	blend_not_from
	mov	eax,ecx
	ret

  blend_not_from:
	cmp	r8d,255
	jb	blend_mix
	mov	eax,edx
	ret

  blend_mix:
	mov	eax,ecx
	and	eax,0FFh
	mov	r9d,edx
	and	r9d,0FFh
	sub	r9d,eax
	imul	r9d,r8d
	sar	r9d,8
	add	eax,r9d
	and	eax,0FFh
	mov	r10d,eax

	mov	eax,ecx
	shr	eax,8
	and	eax,0FFh
	mov	r9d,edx
	shr	r9d,8
	and	r9d,0FFh
	sub	r9d,eax
	imul	r9d,r8d
	sar	r9d,8
	add	eax,r9d
	and	eax,0FFh
	shl	eax,8
	or	r10d,eax

	mov	eax,ecx
	shr	eax,16
	and	eax,0FFh
	mov	r9d,edx
	shr	r9d,16
	and	r9d,0FFh
	sub	r9d,eax
	imul	r9d,r8d
	sar	r9d,8
	add	eax,r9d
	and	eax,0FFh
	shl	eax,16
	or	eax,r10d
	ret
endp

proc CaptionFillColorForState descp,stateValue
	test	dword [rcx+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_DANGER
	jnz	fill_danger
	cmp	edx,CSD_STATE_HOVER
	je	fill_button_hover
	cmp	edx,CSD_STATE_PRESSED
	je	fill_button_pressed
	cmp	edx,CSD_STATE_DISABLED
	je	fill_button_inactive
	cmp	edx,CSD_STATE_INACTIVE
	je	fill_button_inactive
	mov	eax,[current_theme.buttonColor]
	ret

  fill_button_hover:
	mov	eax,CSD_STATE_BUTTON_HOVER
	ret

  fill_button_pressed:
	mov	eax,CSD_STATE_BUTTON_PRESSED
	ret

  fill_button_inactive:
	mov	eax,CSD_STATE_BUTTON_INACTIVE
	ret

  fill_danger:
	cmp	edx,CSD_STATE_HOVER
	je	fill_close_hover
	cmp	edx,CSD_STATE_PRESSED
	je	fill_close_pressed
	cmp	edx,CSD_STATE_DISABLED
	je	fill_close_inactive
	cmp	edx,CSD_STATE_INACTIVE
	je	fill_close_inactive
	mov	eax,[current_theme.closeColor]
	ret

  fill_close_hover:
	mov	eax,CSD_STATE_CLOSE_HOVER
	ret

  fill_close_pressed:
	mov	eax,CSD_STATE_CLOSE_PRESSED
	ret

  fill_close_inactive:
	mov	eax,CSD_STATE_BUTTON_INACTIVE
	ret
endp

proc CaptionGlyphColorForState stateValue
	cmp	ecx,CSD_STATE_DISABLED
	je	glyph_inactive
	cmp	ecx,CSD_STATE_INACTIVE
	je	glyph_inactive
	mov	eax,[current_theme.titleTextColor]
	ret

  glyph_inactive:
	mov	eax,CSD_STATE_TEXT_INACTIVE
	ret
endp

proc TabFillColorForState stateValue
	cmp	ecx,CSD_TAB_VALUE_ACTIVE
	je	tab_fill_active
	cmp	ecx,CSD_STATE_HOVER
	je	tab_fill_hover
	cmp	ecx,CSD_STATE_PRESSED
	je	tab_fill_pressed
	cmp	ecx,CSD_STATE_INACTIVE
	je	tab_fill_inactive
	mov	eax,[current_theme.buttonColor]
	ret

  tab_fill_active:
	mov	eax,[current_theme.bodyBackColor]
	ret

  tab_fill_hover:
	mov	eax,CSD_STATE_BUTTON_HOVER
	ret

  tab_fill_pressed:
	mov	eax,CSD_STATE_BUTTON_PRESSED
	ret

  tab_fill_inactive:
	mov	eax,CSD_STATE_BUTTON_INACTIVE
	ret
endp

proc TabTextColorForState stateValue
	cmp	ecx,CSD_STATE_INACTIVE
	je	tab_text_inactive
	mov	eax,[current_theme.bodyTextColor]
	ret

  tab_text_inactive:
	mov	eax,CSD_STATE_TEXT_INACTIVE
	ret
endp

proc CycleBackdropMode hwnd
	mov	[hwnd],rcx
	fastcall CsdBackdropCycle,addr backdrop_policy
	test	eax,eax
	jz	backdrop_cycle_status
	fastcall CsdBackdropApply,[hwnd],addr backdrop_policy

  backdrop_cycle_status:
	fastcall UpdateThemeStatus
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret
endp

proc SetTabHot index,part
	cmp	[tabstrip.hotIndex],ecx
	jne	set_tab_hot_assign
	cmp	[tabstrip.hotPart],edx
	jne	set_tab_hot_assign
	xor	eax,eax
	ret

  set_tab_hot_assign:
	mov	[tabstrip.hotIndex],ecx
	mov	[tabstrip.hotPart],edx
	fastcall ApplyTabStates
	mov	eax,1
	ret
endp

proc SetTabPressed index,part
	cmp	[tabstrip.pressedIndex],ecx
	jne	set_tab_pressed_assign
	cmp	[tabstrip.pressedPart],edx
	jne	set_tab_pressed_assign
	xor	eax,eax
	ret

  set_tab_pressed_assign:
	mov	[tabstrip.pressedIndex],ecx
	mov	[tabstrip.pressedPart],edx
	fastcall ApplyTabStates
	mov	eax,1
	ret
endp

proc ClearTabInput
	cmp	dword [tabstrip.hotIndex],CSD_TAB_NO_INDEX
	jne	clear_tab_input_apply
	cmp	dword [tabstrip.pressedIndex],CSD_TAB_NO_INDEX
	jne	clear_tab_input_apply
	xor	eax,eax
	ret

  clear_tab_input_apply:
	fastcall CsdTabStripClearInput,addr tabstrip
	fastcall ApplyTabStates
	mov	eax,1
	ret
endp

proc SetActiveTab index
	cmp	ecx,0
	jl	set_active_tab_no
	cmp	ecx,[tabstrip.count]
	jge	set_active_tab_no
	cmp	[tabstrip.activeIndex],ecx
	jne	set_active_tab_assign
	xor	eax,eax
	ret

  set_active_tab_assign:
	mov	[tabstrip.activeIndex],ecx
	fastcall ApplyTabStates
	mov	eax,1
	ret

  set_active_tab_no:
	xor	eax,eax
	ret
endp

proc StartCaptionFadeTimer hwnd
	mov	[hwnd],rcx
	cmp	dword [caption_fade_timer],0
	jne	start_timer_done
	fastcall CsdCaptionAnimAnyActive,caption_state,CSD_CAPTION_CONTROL_COUNT
	test	eax,eax
	jz	start_timer_done
	invoke	SetTimer,[hwnd],CSD_FADE_TIMER_ID,CSD_FADE_TIMER_MS,0
	test	rax,rax
	jz	start_timer_done
	mov	dword [caption_fade_timer],1

  start_timer_done:
	ret
endp

proc StopCaptionFadeTimer hwnd
	mov	[hwnd],rcx
	cmp	dword [caption_fade_timer],0
	je	stop_timer_done
	invoke	KillTimer,[hwnd],CSD_FADE_TIMER_ID
	mov	dword [caption_fade_timer],0

  stop_timer_done:
	ret
endp

proc HandleCaptionFadeTimer hwnd
	mov	[hwnd],rcx
	fastcall CsdCaptionAnimAdvance,caption_state,CSD_CAPTION_CONTROL_COUNT,\
		CSD_FADE_PHASE_STEP
	test	eax,eax
	jz	fade_timer_check_stop
	invoke	InvalidateRect,[hwnd],0,1

  fade_timer_check_stop:
	fastcall CsdCaptionAnimAnyActive,caption_state,CSD_CAPTION_CONTROL_COUNT
	test	eax,eax
	jnz	fade_timer_done
	fastcall StopCaptionFadeTimer,[hwnd]

  fade_timer_done:
	ret
endp

proc HandleCaptionActivate hwnd,wparam
    locals
	active	dd ?
	changed dd ?
    endl

	mov	[hwnd],rcx
	movzx	eax,dx
	mov	dword [active],1
	cmp	eax,WA_INACTIVE
	jne	activate_apply
	mov	dword [active],0
	invoke	ReleaseCapture

  activate_apply:
	mov	dword [changed],0
	fastcall CsdCaptionSetActive,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [active]
	or	[changed],eax
	fastcall ClearTabInput
	or	[changed],eax
	fastcall ApplyTabStates
	cmp	dword [changed],0
	jz	activate_done
	invoke	InvalidateRect,[hwnd],0,1
	fastcall StartCaptionFadeTimer,[hwnd]

  activate_done:
	ret
endp

proc HandleCtlColorEdit hdc,controlHwnd
	mov	[hdc],rcx
	cmp	rdx,[hSearchEdit]
	jne	ctlcolor_no
	invoke	SetTextColor,[hdc],dword [current_theme.bodyTextColor]
	invoke	SetBkColor,[hdc],dword [current_theme.bodyBackColor]
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
	fastcall StartCaptionFadeTimer,[hwnd]

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
		WS_OVERLAPPEDWINDOW or WS_CLIPCHILDREN,WS_EX_APPWINDOW,dword [dpi]
	ret
endp
