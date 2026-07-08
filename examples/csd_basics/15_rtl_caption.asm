; 15_rtl_caption.asm - semantic leading/trailing caption layout.
;
; This keeps 14's responsive overflow and mirrors caption geometry without
; changing descriptor order or physical resize-edge hit-testing.

ADDON_WINDOWS_RESOURCE equ '15_rtl_caption.res'

include 'addon\windows.inc'
include '..\font_icons\font_icons.inc'
include 'addon\csd\caption.inc'
include 'addon\csd\dpi.inc'
include 'addon\csd\theme.inc'
include 'addon\csd\state.inc'
include 'addon\csd\snap.inc'
include 'addon\csd\backdrop.inc'
include 'addon\csd\tabstrip.inc'
include 'addon\csd\child.inc'
include 'addon\csd\uia.inc'
include 'addon\csd\verify.inc'

CSD_TITLE_HEIGHT_DIP	= 48
CSD_CONTENT_PAD_DIP	= 26
CSD_EDGE_DIP		= 4
CSD_BUTTON_WIDTH_DIP	= 44
CSD_ICON_DIP		= 18
CSD_SEARCH_WIDTH_DIP	= 240
CSD_CHILD_PAD_X_DIP	= 8
CSD_CHILD_PAD_Y_DIP	= 12
CSD_DRAG_PAD_DIP	= CSD_CONTENT_PAD_DIP / 2
CSD_TAB_CAPACITY	= 6
CSD_TAB_INITIAL_COUNT	= 3
CSD_TAB_MIN_WIDTH_DIP	= 96
CSD_TAB_PREF_WIDTH_DIP	= 150
CSD_TAB_MAX_WIDTH_DIP	= 220
CSD_TAB_CLOSE_DIP	= 28
CSD_TAB_PAD_DIP		= 10
CSD_TAB_GAP_DIP		= 4
CSD_TAB_DRAG_RECT_MAX	= 8
CSD_MIN_BODY_W_DIP	= 520
CSD_MIN_BODY_H_DIP	= 320
CSD_INITIAL_CLIENT_W_DIP = 920
CSD_INITIAL_CLIENT_H_DIP = 500
CSD_STATE_BUTTON_HOVER	= 00DCEED7h
CSD_STATE_BUTTON_PRESSED = 00BBD8B4h
CSD_STATE_BUTTON_INACTIVE = 006A706Ah
CSD_STATE_CLOSE_HOVER	= 003232E8h
CSD_STATE_CLOSE_PRESSED = 001818B8h
CSD_STATE_TEXT_INACTIVE = 00AEB6AEh
CSD_FADE_TIMER_ID	= 11
CSD_FADE_TIMER_MS	= 16
CSD_FADE_PHASE_STEP	= 34
CSD_LAYOUT_RTL		= 0001h
DC_BRUSH		= 18
WS_EX_NOINHERITLAYOUT	= 00100000h
WS_EX_LAYOUTRTL		= 00400000h


ICON_MENU		= 0E700h
ICON_CLOSE		= 0E8BBh
ICON_MINIMIZE		= 0E921h
ICON_MAXIMIZE		= 0E922h
ICON_RESTORE		= 0E923h
ICON_SEARCH		= 0E721h
ICON_SETTINGS		= 0E713h
ICON_ADD		= 0E710h
ICON_MORE		= 0E712h

ID_CAPTION_SYSTEM	= 4201
ID_CAPTION_SEARCH	= 4202
ID_CAPTION_SETTINGS	= 4203
ID_CAPTION_CLOSE	= 4204
ID_CAPTION_MAX		= 4205
ID_CAPTION_MIN		= 4206
ID_CAPTION_NEWTAB	= 4207
ID_CAPTION_OVERFLOW	= 4208
ID_OVERFLOW_SEARCH	= 4301
ID_OVERFLOW_NEWTAB	= 4302
ID_OVERFLOW_SETTINGS	= 4303
CSD_CAPTION_SYSTEM_INDEX = 0
CSD_CAPTION_SEARCH_INDEX = 1
CSD_CAPTION_NEWTAB_INDEX = 2
CSD_CAPTION_OVERFLOW_INDEX = 3
CSD_CAPTION_SETTINGS_INDEX = 4
CSD_CAPTION_MAX_INDEX	= 6
CSD_TAB_KEYBOARD_OVERFLOW_INDEX = -2
CSD_TAB_PART_OVERFLOW	= 4
CSD_UIA_TAB_SLOT_ROOT	= -1
CSD_UIA_OVERFLOW_SLOT	= CSD_TAB_CAPACITY * 2
CSD_UIA_TAB_SLOT_COUNT	= CSD_UIA_OVERFLOW_SLOT + 1

CSD_RESP_REQUIRED	= 0001h
CSD_RESP_TO_OVERFLOW	= 0002h
CSD_RESP_CHILD_HWND	= 0004h

struct CSD_TAB_KEYBOARD
  active	dd ?
  index		dd ?
  part		dd ?
  reserved	dd ?
ends

struct CSD_CAPTION_RESPONSIVE
  priority	  dd ?
  minWidthDip	  dd ?
  overflowCommand dd ?
  flags		  dd ?
ends

define __GLOBAL_DATA__ csd_data
define __GLOBAL_BSS__ csd_bss

macro csd_data
	app_name	du 'CSD 15 - RTL caption',0
	class_name	du 'Fasm2CsdRtlCaption',0
	title_text	du '15  RTL mirrored caption',0
	body_text	du 'This version keeps responsive overflow, tabs, Search child HWND, backdrop policy, snap hover, and the real system menu while proving leading/trailing are semantic.',13,10,13,10
			du 'Press F2 to toggle LTR/RTL. Caption geometry, tab flow, overflow, and the system-menu popup mirror; physical resize edges still return physical HTLEFT/HTRIGHT.',0
	status_format	du 'Layout %s | Tabs %u/%u active %u | hidden %u | overflow %u | Backdrop %s | build %u | %s | edit HWND %p | snap %s',0
	status_edit	du 'Type in the caption edit. The child HWND owns focus, text input, and IME.',0
	status_settings	du 'Settings cycles the requested DWM system backdrop.',0
	status_new_tab	du 'New tab added.',0
	status_close_tab du 'Tab closed.',0
	status_select_tab du 'Tab selected.',0
	status_overflow du 'Overflow routes hidden caption commands without replacing the real system menu.',0
	status_search_hidden du 'Search is currently collapsed into overflow.',0
	status_layout_toggle du 'Layout direction toggled.',0
	layout_ltr	du 'LTR',0
	layout_rtl	du 'RTL',0
	theme_dark	du 'dark',0
	theme_light	du 'light',0
	snap_on		du 'HTMAXBUTTON',0
	snap_off	du 'HTCLIENT',0
	backdrop_none	du 'None',0
	backdrop_mica	du 'Mica',0
	backdrop_acrylic du 'Acrylic',0
	backdrop_mica_alt du 'Mica Alt',0
	backdrop_available du 'available',0
	backdrop_unavailable du 'requires 22621+',0
	tab_title_1	du 'Overview',0
	tab_title_2	du 'Messages',0
	tab_title_3	du 'Settings',0
	tab_title_4	du 'Notes',0
	tab_title_5	du 'Build Log',0
	tab_title_6	du 'Preview',0
	tab_titles	dq tab_title_1,tab_title_2,tab_title_3,tab_title_4,tab_title_5,tab_title_6
	overflow_search_text du 'Search',0
	overflow_newtab_text du 'New tab',0
	overflow_settings_text du 'Settings',0
	uia_name_root	du 'CSD 15 - RTL caption',0
	uia_name_close_tab du 'Close tab',0
	uia_name_overflow du 'Overflow hidden caption commands',0
	uia_id_root	du 'rtl.root',0
	uia_id_overflow du 'caption.overflow',0
	uia_id_tab_0	du 'tab.0',0
	uia_id_tab_1	du 'tab.1',0
	uia_id_tab_2	du 'tab.2',0
	uia_id_tab_3	du 'tab.3',0
	uia_id_tab_4	du 'tab.4',0
	uia_id_tab_5	du 'tab.5',0
	uia_id_close_0	du 'tab.0.close',0
	uia_id_close_1	du 'tab.1.close',0
	uia_id_close_2	du 'tab.2.close',0
	uia_id_close_3	du 'tab.3.close',0
	uia_id_close_4	du 'tab.4.close',0
	uia_id_close_5	du 'tab.5.close',0
	uia_tab_ids	dq uia_id_tab_0,uia_id_tab_1,uia_id_tab_2,\
			uia_id_tab_3,uia_id_tab_4,uia_id_tab_5
	uia_close_ids	dq uia_id_close_0,uia_id_close_1,uia_id_close_2,\
			uia_id_close_3,uia_id_close_4,uia_id_close_5
	uia_status_overflow_search du 'Search hidden',0
	uia_status_overflow_newtab du 'New tab hidden',0
	uia_status_overflow_settings du 'Settings hidden',0
	uia_status_overflow_search_newtab du 'Search and New tab hidden',0
	uia_status_overflow_search_settings du 'Search and Settings hidden',0
	uia_status_overflow_newtab_settings du 'New tab and Settings hidden',0
	uia_status_overflow_all du 'Search, New tab, and Settings hidden',0

	align 8
	uia_simple_vtbl dq UiaProvider_QueryInterface,CsdUiaProvider_AddRef,\
		CsdUiaProvider_Release,CsdUiaSimple_ProviderOptions,\
		UiaSimple_GetPatternProvider,UiaSimple_GetPropertyValue,\
		UiaSimple_HostRawElementProvider
	uia_fragment_vtbl dq UiaProvider_QueryInterface,CsdUiaProvider_AddRef,\
		CsdUiaProvider_Release,UiaFragment_Navigate,\
		UiaFragment_GetRuntimeId,UiaFragment_BoundingRectangle,\
		UiaFragment_EmbeddedFragmentRoots,UiaFragment_SetFocus,\
		UiaFragment_FragmentRoot
	uia_root_vtbl dq UiaProvider_QueryInterface,CsdUiaProvider_AddRef,\
		CsdUiaProvider_Release,UiaRoot_ElementProviderFromPoint,\
		UiaRoot_GetFocus
	uia_invoke_vtbl dq UiaProvider_QueryInterface,CsdUiaProvider_AddRef,\
		CsdUiaProvider_Release,UiaInvoke_Invoke
	uia_selection_vtbl dq UiaProvider_QueryInterface,CsdUiaProvider_AddRef,\
		CsdUiaProvider_Release,UiaSelection_Select,\
		UiaSelection_AddToSelection,UiaSelection_RemoveFromSelection,\
		UiaSelection_IsSelected,UiaSelection_SelectionContainer

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
		CSD_CAPTION_DESCRIPTOR id: ID_CAPTION_NEWTAB,\
			glyph: ICON_ADD, command: ID_CAPTION_NEWTAB,\
			hitCode: HTCLIENT,\
			flags: CSD_CAPTION_ALIGN_LEADING or CSD_CAPTION_APP,\
			widthDip: CSD_BUTTON_WIDTH_DIP
		CSD_CAPTION_DESCRIPTOR id: ID_CAPTION_OVERFLOW,\
			glyph: ICON_MORE, command: ID_CAPTION_OVERFLOW,\
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

	caption_responsive:
		CSD_CAPTION_RESPONSIVE priority: 100, minWidthDip: CSD_BUTTON_WIDTH_DIP,\
			overflowCommand: 0, flags: CSD_RESP_REQUIRED
		CSD_CAPTION_RESPONSIVE priority: 10, minWidthDip: CSD_SEARCH_WIDTH_DIP,\
			overflowCommand: ID_OVERFLOW_SEARCH,\
			flags: CSD_RESP_TO_OVERFLOW or CSD_RESP_CHILD_HWND
		CSD_CAPTION_RESPONSIVE priority: 30, minWidthDip: CSD_BUTTON_WIDTH_DIP,\
			overflowCommand: ID_OVERFLOW_NEWTAB, flags: CSD_RESP_TO_OVERFLOW
		CSD_CAPTION_RESPONSIVE priority: 40, minWidthDip: CSD_BUTTON_WIDTH_DIP,\
			overflowCommand: 0, flags: 0
		CSD_CAPTION_RESPONSIVE priority: 20, minWidthDip: CSD_BUTTON_WIDTH_DIP,\
			overflowCommand: ID_OVERFLOW_SETTINGS, flags: CSD_RESP_TO_OVERFLOW
		CSD_CAPTION_RESPONSIVE priority: 100, minWidthDip: CSD_BUTTON_WIDTH_DIP,\
			overflowCommand: 0, flags: CSD_RESP_REQUIRED
		CSD_CAPTION_RESPONSIVE priority: 100, minWidthDip: CSD_BUTTON_WIDTH_DIP,\
			overflowCommand: 0, flags: CSD_RESP_REQUIRED
		CSD_CAPTION_RESPONSIVE priority: 100, minWidthDip: CSD_BUTTON_WIDTH_DIP,\
			overflowCommand: 0, flags: CSD_RESP_REQUIRED

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
	backdrop_policy CSD_BACKDROP_POLICY
	tabstrip	CSD_TABSTRIP
	tab_keyboard	CSD_TAB_KEYBOARD
	hSearchEdit	dq ?
	hUiFont		dq ?
	hOverflowMenu	dq ?
	caption_fade_timer dd ?
	tab_next_id	dd ?
	caption_drag_rect_count dd ?
	caption_hidden_count dd ?
	caption_overflow_count dd ?
	layout_flags	dd ?
	min_track_client_w dd ?
	min_track_client_h dd ?
	status_buffer	rw 192
	uia_initialized dd ?
	caption_drag_rects rb sizeof.RECT * CSD_TAB_DRAG_RECT_MAX
	caption_geometry rb sizeof.CSD_CAPTION_GEOMETRY * CSD_CAPTION_CONTROL_COUNT
	caption_state	rb sizeof.CSD_CAPTION_STATE * CSD_CAPTION_CONTROL_COUNT
	caption_visible rd CSD_CAPTION_CONTROL_COUNT
	align 8
	uia_root	rb sizeof.CSD_UIA_PROVIDER
	uia_children	rb sizeof.CSD_UIA_PROVIDER * CSD_UIA_TAB_SLOT_COUNT
	tab_items	rb sizeof.CSD_TAB_ITEM * CSD_TAB_CAPACITY
	tab_geometry	rb sizeof.CSD_TAB_GEOMETRY * CSD_TAB_CAPACITY
	tab_state	rb sizeof.CSD_TAB_STATE * CSD_TAB_CAPACITY
purge csd_bss
end macro

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

include 'csd_example_support.inc'

proc LayoutDirectionName
	test	dword [layout_flags],CSD_LAYOUT_RTL
	jnz	layout_name_rtl
	mov	rax,layout_ltr
	ret

  layout_name_rtl:
	mov	rax,layout_rtl
	ret
endp

proc RtlPopupMenuFlags baseFlags
	mov	eax,ecx
	test	dword [layout_flags],CSD_LAYOUT_RTL
	jz	popup_flags_done
	or	eax,TPM_LAYOUTRTL

  popup_flags_done:
	ret
endp

proc TabTitleForIndex index
	mov	eax,ecx
	cmp	eax,CSD_TAB_CAPACITY
	jb	tab_title_in_range
	mov	eax,CSD_TAB_CAPACITY - 1

  tab_title_in_range:
	mov	rax,[tab_titles+rax*8]
	ret
endp

proc InitOneTab index,titlep
	mov	eax,ecx
	imul	eax,sizeof.CSD_TAB_ITEM
	lea	rcx,[tab_items+rax]
	mov	eax,dword [tab_next_id]
	mov	[rcx+CSD_TAB_ITEM.id],eax
	inc	dword [tab_next_id]
	mov	dword [rcx+CSD_TAB_ITEM.flags],0
	mov	[rcx+CSD_TAB_ITEM.titlep],rdx
	mov	dword [rcx+CSD_TAB_ITEM.iconGlyph],0
	mov	dword [rcx+CSD_TAB_ITEM.minWidthDip],CSD_TAB_MIN_WIDTH_DIP
	mov	dword [rcx+CSD_TAB_ITEM.preferredDip],CSD_TAB_PREF_WIDTH_DIP
	mov	dword [rcx+CSD_TAB_ITEM.maxWidthDip],CSD_TAB_MAX_WIDTH_DIP
	ret
endp

proc ApplyTabStates
	mov	ecx,[tabstrip.count]
	test	ecx,ecx
	jz	tab_states_done
	mov	rdx,tab_state
	xor	r8d,r8d

  tab_states_loop:
	mov	al,CSD_STATE_NORMAL
	cmp	dword [caption_input.active],0
	je	tab_state_inactive
	cmp	r8d,[tabstrip.activeIndex]
	je	tab_state_active
	cmp	r8d,[tabstrip.hotIndex]
	jne	tab_state_store
	mov	al,CSD_STATE_HOVER
	cmp	r8d,[tabstrip.pressedIndex]
	jne	tab_state_store
	cmp	dword [tabstrip.pressedPart],CSD_TAB_PART_BODY
	jne	tab_state_store
	mov	al,CSD_STATE_PRESSED
	jmp	tab_state_store

  tab_state_active:
	mov	al,CSD_TAB_VALUE_ACTIVE
	cmp	r8d,[tabstrip.pressedIndex]
	jne	tab_state_store
	cmp	dword [tabstrip.pressedPart],CSD_TAB_PART_BODY
	jne	tab_state_store
	mov	al,CSD_STATE_PRESSED
	jmp	tab_state_store

  tab_state_inactive:
	mov	al,CSD_STATE_INACTIVE

  tab_state_store:
	mov	[rdx+CSD_TAB_STATE.value],al
	mov	byte [rdx+CSD_TAB_STATE.closeValue],CSD_STATE_NORMAL
	cmp	r8d,[tabstrip.hotIndex]
	jne	tab_state_next
	cmp	dword [tabstrip.hotPart],CSD_TAB_PART_CLOSE
	jne	tab_state_next
	mov	byte [rdx+CSD_TAB_STATE.closeValue],CSD_STATE_HOVER
	cmp	r8d,[tabstrip.pressedIndex]
	jne	tab_state_next
	cmp	dword [tabstrip.pressedPart],CSD_TAB_PART_CLOSE
	jne	tab_state_next
	mov	byte [rdx+CSD_TAB_STATE.closeValue],CSD_STATE_PRESSED

  tab_state_next:
	add	rdx,sizeof.CSD_TAB_STATE
	inc	r8d
	dec	ecx
	jnz	tab_states_loop

  tab_states_done:
	ret
endp

proc InitTabKeyboard
	mov	dword [tab_keyboard.active],0
	mov	dword [tab_keyboard.index],CSD_TAB_NO_INDEX
	mov	dword [tab_keyboard.part],CSD_TAB_PART_NONE
	ret
endp

proc IsTabKeyboardTargetVisible index,part
	cmp	ecx,CSD_TAB_KEYBOARD_OVERFLOW_INDEX
	jne	tab_key_target_tab
	cmp	edx,CSD_TAB_PART_OVERFLOW
	jne	tab_key_target_no
	cmp	dword [caption_visible+CSD_CAPTION_OVERFLOW_INDEX*4],0
	setne	al
	movzx	eax,al
	ret

  tab_key_target_tab:
	cmp	ecx,0
	jl	tab_key_target_no
	cmp	ecx,[tabstrip.count]
	jge	tab_key_target_no
	cmp	edx,CSD_TAB_PART_BODY
	je	tab_key_target_check_visible
	cmp	edx,CSD_TAB_PART_CLOSE
	jne	tab_key_target_no
	cmp	dword [tabstrip.count],1
	jle	tab_key_target_no

  tab_key_target_check_visible:
	mov	eax,ecx
	imul	eax,sizeof.CSD_TAB_ITEM
	test	dword [tab_items+rax+CSD_TAB_ITEM.flags],CSD_TAB_FLAG_VISIBLE
	jz	tab_key_target_no
	mov	eax,1
	ret

  tab_key_target_no:
	xor	eax,eax
	ret
endp

proc ClearTabKeyboard hwnd
	mov	[hwnd],rcx
	cmp	dword [tab_keyboard.active],0
	je	clear_tab_keyboard_no
	mov	dword [tab_keyboard.active],0
	mov	dword [tab_keyboard.index],CSD_TAB_NO_INDEX
	mov	dword [tab_keyboard.part],CSD_TAB_PART_NONE
	mov	rcx,[hwnd]
	jrcxz	clear_tab_keyboard_done
	invoke	InvalidateRect,rcx,0,1

  clear_tab_keyboard_done:
	mov	eax,1
	ret

  clear_tab_keyboard_no:
	xor	eax,eax
	ret
endp

proc NormalizeTabKeyboard
	cmp	dword [tab_keyboard.active],0
	je	normalize_tab_keyboard_done
	fastcall IsTabKeyboardTargetVisible,dword [tab_keyboard.index],\
		dword [tab_keyboard.part]
	test	eax,eax
	jnz	normalize_tab_keyboard_done
	fastcall IsTabKeyboardTargetVisible,dword [tabstrip.activeIndex],\
		CSD_TAB_PART_BODY
	test	eax,eax
	jz	normalize_tab_keyboard_clear
	mov	eax,[tabstrip.activeIndex]
	mov	[tab_keyboard.index],eax
	mov	dword [tab_keyboard.part],CSD_TAB_PART_BODY
	ret

  normalize_tab_keyboard_clear:
	fastcall InitTabKeyboard

  normalize_tab_keyboard_done:
	ret
endp

proc SetTabKeyboardFocus hwnd,index,part
	mov	[hwnd],rcx
	mov	dword [index],edx
	mov	dword [part],r8d
	fastcall IsTabKeyboardTargetVisible,dword [index],dword [part]
	test	eax,eax
	jz	set_tab_keyboard_clear
	cmp	dword [tab_keyboard.active],0
	je	set_tab_keyboard_apply
	mov	eax,[tab_keyboard.index]
	cmp	eax,dword [index]
	jne	set_tab_keyboard_apply
	mov	eax,[tab_keyboard.part]
	cmp	eax,dword [part]
	jne	set_tab_keyboard_apply
	xor	eax,eax
	ret

  set_tab_keyboard_apply:
	mov	dword [tab_keyboard.active],1
	mov	eax,dword [index]
	mov	[tab_keyboard.index],eax
	mov	eax,dword [part]
	mov	[tab_keyboard.part],eax
	invoke	SetFocus,[hwnd]
	invoke	ReleaseCapture
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret

  set_tab_keyboard_clear:
	fastcall ClearTabKeyboard,[hwnd]
	ret
endp

proc EnterTabKeyboard hwnd
    locals
	index dd ?
    endl

	mov	[hwnd],rcx
	mov	ecx,[tabstrip.activeIndex]
	fastcall IsTabKeyboardTargetVisible,ecx,CSD_TAB_PART_BODY
	test	eax,eax
	jz	enter_tab_keyboard_find
	mov	edx,[tabstrip.activeIndex]
	fastcall SetTabKeyboardFocus,[hwnd],edx,CSD_TAB_PART_BODY
	ret

  enter_tab_keyboard_find:
	mov	dword [index],0

  enter_tab_keyboard_loop:
	mov	eax,[index]
	cmp	eax,[tabstrip.count]
	jge	enter_tab_keyboard_no
	fastcall IsTabKeyboardTargetVisible,dword [index],CSD_TAB_PART_BODY
	test	eax,eax
	jnz	enter_tab_keyboard_apply
	inc	dword [index]
	jmp	enter_tab_keyboard_loop

  enter_tab_keyboard_apply:
	fastcall SetTabKeyboardFocus,[hwnd],dword [index],CSD_TAB_PART_BODY
	ret

  enter_tab_keyboard_no:
	fastcall ClearTabKeyboard,[hwnd]
	ret
endp

proc MoveTabKeyboardFocus hwnd,delta
    locals
	slot	dd ?
	total	dd ?
	attempts dd ?
	index	dd ?
	part	dd ?
    endl

	mov	[hwnd],rcx
	mov	dword [delta],edx
	cmp	dword [tab_keyboard.active],0
	jne	move_tab_keyboard_active
	fastcall EnterTabKeyboard,[hwnd]
	ret

  move_tab_keyboard_active:
	mov	dword [total],CSD_UIA_TAB_SLOT_COUNT
	mov	dword [attempts],CSD_UIA_TAB_SLOT_COUNT
	fastcall UiaSlotFromIndexPart,dword [tab_keyboard.index],\
		dword [tab_keyboard.part]
	cmp	eax,CSD_UIA_TAB_SLOT_ROOT
	jne	move_tab_keyboard_slot_ready
	xor	eax,eax

  move_tab_keyboard_slot_ready:
	mov	[slot],eax

  move_tab_keyboard_loop:
	mov	eax,[slot]
	add	eax,dword [delta]
	cmp	eax,0
	jge	move_tab_keyboard_check_high
	mov	eax,[total]
	dec	eax
	jmp	move_tab_keyboard_store_slot

  move_tab_keyboard_check_high:
	cmp	eax,[total]
	jl	move_tab_keyboard_store_slot
	xor	eax,eax

  move_tab_keyboard_store_slot:
	mov	[slot],eax
	mov	ecx,eax
	fastcall UiaSlotToIndexPart,ecx
	mov	[index],eax
	mov	[part],edx
	fastcall IsTabKeyboardTargetVisible,dword [index],dword [part]
	test	eax,eax
	jnz	move_tab_keyboard_apply
	dec	dword [attempts]
	jnz	move_tab_keyboard_loop

  move_tab_keyboard_none:
	fastcall ClearTabKeyboard,[hwnd]
	ret

  move_tab_keyboard_apply:
	fastcall SetTabKeyboardFocus,[hwnd],dword [index],dword [part]
	ret
endp

proc MoveTabKeyboardTabFocus hwnd,delta
    locals
	index	dd ?
	attempts dd ?
    endl

	mov	[hwnd],rcx
	mov	dword [delta],edx
	cmp	dword [tab_keyboard.active],0
	jne	move_tab_body_active
	fastcall EnterTabKeyboard,[hwnd]
	ret

  move_tab_body_active:
	mov	eax,[tabstrip.count]
	mov	[attempts],eax
	test	eax,eax
	jz	move_tab_body_none
	mov	eax,[tab_keyboard.index]
	cmp	eax,0
	jl	move_tab_body_use_active
	cmp	eax,[tabstrip.count]
	jl	move_tab_body_store_current

  move_tab_body_use_active:
	mov	eax,[tabstrip.activeIndex]
	cmp	eax,0
	jge	move_tab_body_check_active_high
	xor	eax,eax
	jmp	move_tab_body_store_current

  move_tab_body_check_active_high:
	cmp	eax,[tabstrip.count]
	jl	move_tab_body_store_current
	xor	eax,eax

  move_tab_body_store_current:
	mov	[index],eax

  move_tab_body_loop:
	mov	eax,[index]
	add	eax,dword [delta]
	cmp	eax,0
	jge	move_tab_body_check_high
	mov	eax,[tabstrip.count]
	dec	eax
	jmp	move_tab_body_store

  move_tab_body_check_high:
	cmp	eax,[tabstrip.count]
	jl	move_tab_body_store
	xor	eax,eax

  move_tab_body_store:
	mov	[index],eax
	fastcall IsTabKeyboardTargetVisible,dword [index],CSD_TAB_PART_BODY
	test	eax,eax
	jnz	move_tab_body_apply
	dec	dword [attempts]
	jnz	move_tab_body_loop

  move_tab_body_none:
	fastcall ClearTabKeyboard,[hwnd]
	ret

  move_tab_body_apply:
	fastcall SetTabKeyboardFocus,[hwnd],dword [index],CSD_TAB_PART_BODY
	ret
endp

proc CloseTabKeyboardIndex hwnd,index
	mov	[hwnd],rcx
	mov	dword [index],edx
	fastcall CloseDemoTab,dword [index]
	test	eax,eax
	jz	close_tab_keyboard_no
	fastcall LayoutCaptionControls
	fastcall LayoutCaptionChildren
	fastcall UpdateThemeStatus
	mov	edx,[tabstrip.activeIndex]
	fastcall SetTabKeyboardFocus,[hwnd],edx,CSD_TAB_PART_BODY
	mov	eax,1
	ret

  close_tab_keyboard_no:
	xor	eax,eax
	ret
endp

proc InvokeTabKeyboardFocus hwnd
	mov	[hwnd],rcx
	cmp	dword [tab_keyboard.active],0
	je	invoke_tab_keyboard_no
	fastcall IsTabKeyboardTargetVisible,dword [tab_keyboard.index],\
		dword [tab_keyboard.part]
	test	eax,eax
	jz	invoke_tab_keyboard_no
	cmp	dword [tab_keyboard.part],CSD_TAB_PART_OVERFLOW
	je	invoke_tab_keyboard_overflow
	cmp	dword [tab_keyboard.part],CSD_TAB_PART_CLOSE
	je	invoke_tab_keyboard_close
	cmp	dword [tab_keyboard.part],CSD_TAB_PART_BODY
	jne	invoke_tab_keyboard_no
	fastcall SetActiveTab,dword [tab_keyboard.index]
	mov	rax,status_select_tab
	mov	[status_message],rax
	fastcall UpdateThemeStatus
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret

  invoke_tab_keyboard_close:
	fastcall CloseTabKeyboardIndex,[hwnd],dword [tab_keyboard.index]
	ret

  invoke_tab_keyboard_overflow:
	lea	r10,[caption_geometry+CSD_CAPTION_OVERFLOW_INDEX*sizeof.CSD_CAPTION_GEOMETRY]
	mov	eax,[r10+CSD_CAPTION_GEOMETRY.rect.bottom]
	shl	eax,16
	test	dword [layout_flags],CSD_LAYOUT_RTL
	jz	invoke_tab_keyboard_overflow_ltr
	movzx	edx,word [r10+CSD_CAPTION_GEOMETRY.rect.right]
	jmp	invoke_tab_keyboard_overflow_x_ready

  invoke_tab_keyboard_overflow_ltr:
	movzx	edx,word [r10+CSD_CAPTION_GEOMETRY.rect.left]

  invoke_tab_keyboard_overflow_x_ready:
	or	eax,edx
	fastcall ShowOverflowMenu,[hwnd],eax
	ret

  invoke_tab_keyboard_no:
	xor	eax,eax
	ret
endp

proc HandleTabKeyDown hwnd,wparam,lparam
    locals
	delta dd ?
    endl

	mov	[hwnd],rcx
	mov	dword [wparam],edx
	cmp	edx,VK_F6
	je	tab_key_enter
	cmp	edx,VK_TAB
	je	tab_key_tab
	cmp	edx,VK_LEFT
	je	tab_key_left
	cmp	edx,VK_RIGHT
	je	tab_key_right
	cmp	edx,VK_RETURN
	je	tab_key_invoke
	cmp	edx,VK_SPACE
	je	tab_key_invoke
	cmp	edx,VK_DELETE
	je	tab_key_delete
	cmp	edx,VK_ESCAPE
	je	tab_key_escape
	xor	eax,eax
	ret

  tab_key_enter:
	fastcall EnterTabKeyboard,[hwnd]
	ret

  tab_key_tab:
	invoke	GetKeyState,VK_CONTROL
	test	eax,8000h
	jz	tab_key_no
	mov	dword [delta],1
	invoke	GetKeyState,VK_SHIFT
	test	eax,8000h
	jz	tab_key_tab_ready
	mov	dword [delta],-1

  tab_key_tab_ready:
	fastcall MoveTabKeyboardTabFocus,[hwnd],dword [delta]
	ret

  tab_key_left:
	cmp	dword [tab_keyboard.active],0
	je	tab_key_no
	mov	dword [delta],-1
	test	dword [layout_flags],CSD_LAYOUT_RTL
	jz	tab_key_left_ready
	mov	dword [delta],1

  tab_key_left_ready:
	fastcall MoveTabKeyboardFocus,[hwnd],dword [delta]
	ret

  tab_key_right:
	cmp	dword [tab_keyboard.active],0
	je	tab_key_no
	mov	dword [delta],1
	test	dword [layout_flags],CSD_LAYOUT_RTL
	jz	tab_key_right_ready
	mov	dword [delta],-1

  tab_key_right_ready:
	fastcall MoveTabKeyboardFocus,[hwnd],dword [delta]
	ret

  tab_key_invoke:
	fastcall InvokeTabKeyboardFocus,[hwnd]
	ret

  tab_key_delete:
	cmp	dword [tab_keyboard.active],0
	je	tab_key_no
	cmp	dword [tab_keyboard.part],CSD_TAB_PART_BODY
	je	tab_key_delete_close
	cmp	dword [tab_keyboard.part],CSD_TAB_PART_CLOSE
	jne	tab_key_no

  tab_key_delete_close:
	fastcall CloseTabKeyboardIndex,[hwnd],dword [tab_keyboard.index]
	mov	eax,1
	ret

  tab_key_escape:
	cmp	dword [tab_keyboard.active],0
	je	tab_key_no
	fastcall ClearTabKeyboard,[hwnd]
	ret

  tab_key_no:
	xor	eax,eax
	ret
endp

proc HandleTabSysKeyDown hwnd,wparam,lparam
	mov	[hwnd],rcx
	cmp	edx,VK_SPACE
	je	tab_syskey_system_menu
	xor	eax,eax
	ret

  tab_syskey_system_menu:
	fastcall ShowSystemMenu,[hwnd],-1
	mov	eax,1
	ret
endp

proc InitUiaProviders uses rbx rsi
	fastcall CsdUiaLoadApis
	test	eax,eax
	jz	init_uia_done
	fastcall CsdUiaInitProvider,uia_root,CSD_UIA_TAB_SLOT_ROOT,\
		uia_simple_vtbl,uia_fragment_vtbl,uia_root_vtbl,\
		uia_invoke_vtbl,uia_selection_vtbl

	xor	esi,esi
	mov	rbx,uia_children
  init_uia_loop:
	fastcall CsdUiaInitProvider,rbx,esi,uia_simple_vtbl,\
		uia_fragment_vtbl,uia_root_vtbl,uia_invoke_vtbl,\
		uia_selection_vtbl
	add	rbx,sizeof.CSD_UIA_PROVIDER
	inc	esi
	cmp	esi,CSD_UIA_TAB_SLOT_COUNT
	jb	init_uia_loop
	mov	dword [uia_initialized],1

  init_uia_done:
	ret
endp

proc DisconnectUiaProvider
	cmp	dword [uia_initialized],0
	je	disconnect_uia_done
	fastcall CsdUiaDisconnectProvider,uia_root+CSD_UIA_SIMPLE_OFFSET
	mov	dword [uia_initialized],0

  disconnect_uia_done:
	ret
endp

proc UiaSlotToIndexPart slot
	cmp	ecx,0
	jl	uia_slot_to_part_no
	cmp	ecx,CSD_UIA_OVERFLOW_SLOT
	je	uia_slot_to_part_overflow
	ja	uia_slot_to_part_no
	mov	eax,ecx
	shr	eax,1
	mov	edx,CSD_TAB_PART_BODY
	test	ecx,1
	jz	uia_slot_to_part_done
	mov	edx,CSD_TAB_PART_CLOSE

  uia_slot_to_part_done:
	ret

  uia_slot_to_part_overflow:
	mov	eax,CSD_TAB_KEYBOARD_OVERFLOW_INDEX
	mov	edx,CSD_TAB_PART_OVERFLOW
	ret

  uia_slot_to_part_no:
	mov	eax,CSD_TAB_NO_INDEX
	mov	edx,CSD_TAB_PART_NONE
	ret
endp

proc UiaSlotFromIndexPart index,part
	cmp	ecx,CSD_TAB_KEYBOARD_OVERFLOW_INDEX
	jne	uia_slot_from_part_tab
	cmp	edx,CSD_TAB_PART_OVERFLOW
	jne	uia_slot_from_part_no
	mov	eax,CSD_UIA_OVERFLOW_SLOT
	ret

  uia_slot_from_part_tab:
	cmp	ecx,0
	jl	uia_slot_from_part_no
	cmp	ecx,[tabstrip.count]
	jge	uia_slot_from_part_no
	cmp	edx,CSD_TAB_PART_BODY
	je	uia_slot_from_body
	cmp	edx,CSD_TAB_PART_CLOSE
	jne	uia_slot_from_part_no
	mov	eax,ecx
	shl	eax,1
	inc	eax
	ret

  uia_slot_from_body:
	mov	eax,ecx
	shl	eax,1
	ret

  uia_slot_from_part_no:
	mov	eax,CSD_UIA_TAB_SLOT_ROOT
	ret
endp

proc UiaProviderFromSlot slot
	cmp	ecx,CSD_UIA_TAB_SLOT_COUNT
	jae	uia_provider_from_slot_no
	imul	ecx,sizeof.CSD_UIA_PROVIDER
	lea	rax,[uia_children+ecx]
	ret

  uia_provider_from_slot_no:
	xor	eax,eax
	ret
endp

proc UiaProviderVisible providerp
	test	rcx,rcx
	jz	uia_provider_visible_no
	mov	ecx,[rcx+CSD_UIA_PROVIDER.slot]
	cmp	ecx,CSD_UIA_TAB_SLOT_ROOT
	je	uia_provider_visible_yes
	fastcall UiaSlotToIndexPart,ecx
	cmp	eax,CSD_TAB_NO_INDEX
	je	uia_provider_visible_no
	fastcall IsTabKeyboardTargetVisible,eax,edx
	ret

  uia_provider_visible_yes:
	mov	eax,1
	ret

  uia_provider_visible_no:
	xor	eax,eax
	ret
endp

proc UiaProviderIsBody providerp
	mov	ecx,[rcx+CSD_UIA_PROVIDER.slot]
	fastcall UiaSlotToIndexPart,ecx
	cmp	edx,CSD_TAB_PART_BODY
	sete	al
	movzx	eax,al
	ret
endp

proc UiaFirstVisibleProvider
    locals
	slot	dd ?
	providerp dq ?
    endl

	mov	dword [slot],0

  uia_first_visible_loop:
	cmp	dword [slot],CSD_UIA_TAB_SLOT_COUNT
	jae	uia_first_visible_no
	fastcall UiaProviderFromSlot,dword [slot]
	mov	[providerp],rax
	fastcall UiaProviderVisible,rax
	test	eax,eax
	jnz	uia_first_visible_found
	inc	dword [slot]
	jmp	uia_first_visible_loop

  uia_first_visible_found:
	mov	rax,[providerp]
	ret

  uia_first_visible_no:
	xor	eax,eax
	ret
endp

proc UiaLastVisibleProvider
    locals
	slot	dd ?
	providerp dq ?
    endl

	mov	dword [slot],CSD_UIA_TAB_SLOT_COUNT - 1

  uia_last_visible_loop:
	fastcall UiaProviderFromSlot,dword [slot]
	mov	[providerp],rax
	fastcall UiaProviderVisible,rax
	test	eax,eax
	jnz	uia_last_visible_found
	cmp	dword [slot],0
	je	uia_last_visible_no
	dec	dword [slot]
	jmp	uia_last_visible_loop

  uia_last_visible_found:
	mov	rax,[providerp]
	ret

  uia_last_visible_no:
	xor	eax,eax
	ret
endp

proc UiaNextVisibleProvider providerp
    locals
	slot	dd ?
	providerp_next dq ?
    endl

	mov	eax,[rcx+CSD_UIA_PROVIDER.slot]
	inc	eax
	mov	[slot],eax

  uia_next_visible_loop:
	cmp	dword [slot],CSD_UIA_TAB_SLOT_COUNT
	jae	uia_next_visible_no
	fastcall UiaProviderFromSlot,dword [slot]
	mov	[providerp_next],rax
	fastcall UiaProviderVisible,rax
	test	eax,eax
	jnz	uia_next_visible_found
	inc	dword [slot]
	jmp	uia_next_visible_loop

  uia_next_visible_found:
	mov	rax,[providerp_next]
	ret

  uia_next_visible_no:
	xor	eax,eax
	ret
endp

proc UiaPreviousVisibleProvider providerp
    locals
	slot	dd ?
	providerp_prev dq ?
    endl

	mov	eax,[rcx+CSD_UIA_PROVIDER.slot]
	test	eax,eax
	jz	uia_previous_visible_no
	dec	eax
	mov	[slot],eax

  uia_previous_visible_loop:
	fastcall UiaProviderFromSlot,dword [slot]
	mov	[providerp_prev],rax
	fastcall UiaProviderVisible,rax
	test	eax,eax
	jnz	uia_previous_visible_found
	cmp	dword [slot],0
	je	uia_previous_visible_no
	dec	dword [slot]
	jmp	uia_previous_visible_loop

  uia_previous_visible_found:
	mov	rax,[providerp_prev]
	ret

  uia_previous_visible_no:
	xor	eax,eax
	ret
endp

proc UiaTabNameForProvider providerp
	mov	ecx,[rcx+CSD_UIA_PROVIDER.slot]
	cmp	ecx,CSD_UIA_TAB_SLOT_ROOT
	je	uia_name_root_ready
	fastcall UiaSlotToIndexPart,ecx
	cmp	edx,CSD_TAB_PART_OVERFLOW
	je	uia_name_overflow_ready
	cmp	edx,CSD_TAB_PART_CLOSE
	je	uia_name_close_ready
	cmp	edx,CSD_TAB_PART_BODY
	jne	uia_name_none
	imul	eax,sizeof.CSD_TAB_ITEM
	cdqe
	lea	rax,[tab_items+rax]
	mov	rax,[rax+CSD_TAB_ITEM.titlep]
	ret

  uia_name_root_ready:
	mov	rax,uia_name_root
	ret

  uia_name_close_ready:
	mov	rax,uia_name_close_tab
	ret

  uia_name_overflow_ready:
	mov	rax,uia_name_overflow
	ret

  uia_name_none:
	xor	eax,eax
	ret
endp

proc UiaTabAutomationIdForProvider providerp
	mov	ecx,[rcx+CSD_UIA_PROVIDER.slot]
	cmp	ecx,CSD_UIA_TAB_SLOT_ROOT
	je	uia_id_root_ready
	fastcall UiaSlotToIndexPart,ecx
	cmp	edx,CSD_TAB_PART_OVERFLOW
	je	uia_id_overflow_ready
	cmp	edx,CSD_TAB_PART_CLOSE
	je	uia_id_close_ready
	cmp	edx,CSD_TAB_PART_BODY
	jne	uia_id_none
	mov	rax,[uia_tab_ids+rax*8]
	ret

  uia_id_close_ready:
	mov	rax,[uia_close_ids+rax*8]
	ret

  uia_id_root_ready:
	mov	rax,uia_id_root
	ret

  uia_id_overflow_ready:
	mov	rax,uia_id_overflow
	ret

  uia_id_none:
	xor	eax,eax
	ret
endp

proc UiaOverflowStatusString
	xor	eax,eax
	cmp	dword [caption_visible+CSD_CAPTION_SEARCH_INDEX*4],0
	jne	uia_overflow_status_newtab
	or	eax,1

  uia_overflow_status_newtab:
	cmp	dword [caption_visible+CSD_CAPTION_NEWTAB_INDEX*4],0
	jne	uia_overflow_status_settings
	or	eax,2

  uia_overflow_status_settings:
	cmp	dword [caption_visible+CSD_CAPTION_SETTINGS_INDEX*4],0
	jne	uia_overflow_status_select
	or	eax,4

  uia_overflow_status_select:
	cmp	eax,1
	je	uia_overflow_status_search_ready
	cmp	eax,2
	je	uia_overflow_status_newtab_ready
	cmp	eax,3
	je	uia_overflow_status_search_newtab_ready
	cmp	eax,4
	je	uia_overflow_status_settings_ready
	cmp	eax,5
	je	uia_overflow_status_search_settings_ready
	cmp	eax,6
	je	uia_overflow_status_newtab_settings_ready
	cmp	eax,7
	je	uia_overflow_status_all_ready
	xor	eax,eax
	ret

  uia_overflow_status_search_ready:
	mov	rax,uia_status_overflow_search
	ret

  uia_overflow_status_newtab_ready:
	mov	rax,uia_status_overflow_newtab
	ret

  uia_overflow_status_settings_ready:
	mov	rax,uia_status_overflow_settings
	ret

  uia_overflow_status_search_newtab_ready:
	mov	rax,uia_status_overflow_search_newtab
	ret

  uia_overflow_status_search_settings_ready:
	mov	rax,uia_status_overflow_search_settings
	ret

  uia_overflow_status_newtab_settings_ready:
	mov	rax,uia_status_overflow_newtab_settings
	ret

  uia_overflow_status_all_ready:
	mov	rax,uia_status_overflow_all
	ret
endp

proc UiaProvider_QueryInterface uses rbx rsi rdi, thisp,riidp,ppvp
	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	rsi,rdx
	mov	rdi,r8
	test	rdi,rdi
	jz	query_interface_bad_pointer
	mov	qword [rdi],0

	fastcall CsdUiaGuidEquals,rsi,CSD_IID_IUnknown
	test	eax,eax
	jnz	query_interface_simple
	fastcall CsdUiaGuidEquals,rsi,CSD_IID_IRawElementProviderSimple
	test	eax,eax
	jnz	query_interface_simple
	fastcall CsdUiaGuidEquals,rsi,CSD_IID_IRawElementProviderFragment
	test	eax,eax
	jnz	query_interface_fragment
	fastcall CsdUiaGuidEquals,rsi,CSD_IID_IRawElementProviderFragmentRoot
	test	eax,eax
	jnz	query_interface_root
	fastcall CsdUiaGuidEquals,rsi,CSD_IID_IInvokeProvider
	test	eax,eax
	jnz	query_interface_invoke
	fastcall CsdUiaGuidEquals,rsi,CSD_IID_ISelectionItemProvider
	test	eax,eax
	jnz	query_interface_selection
	mov	eax,E_NOINTERFACE
	ret

  query_interface_simple:
	lea	rax,[rbx+CSD_UIA_SIMPLE_OFFSET]
	jmp	query_interface_return

  query_interface_fragment:
	lea	rax,[rbx+CSD_UIA_FRAGMENT_OFFSET]
	jmp	query_interface_return

  query_interface_root:
	cmp	dword [rbx+CSD_UIA_PROVIDER.slot],CSD_UIA_TAB_SLOT_ROOT
	jne	query_interface_no_interface
	lea	rax,[rbx+CSD_UIA_ROOT_OFFSET]
	jmp	query_interface_return

  query_interface_invoke:
	cmp	dword [rbx+CSD_UIA_PROVIDER.slot],CSD_UIA_TAB_SLOT_ROOT
	je	query_interface_no_interface
	fastcall UiaProviderVisible,rbx
	test	eax,eax
	jz	query_interface_no_interface
	lea	rax,[rbx+CSD_UIA_INVOKE_OFFSET]
	jmp	query_interface_return

  query_interface_selection:
	fastcall UiaProviderVisible,rbx
	test	eax,eax
	jz	query_interface_no_interface
	fastcall UiaProviderIsBody,rbx
	test	eax,eax
	jz	query_interface_no_interface
	lea	rax,[rbx+CSD_UIA_SELECTION_OFFSET]

  query_interface_return:
	mov	[rdi],rax
	inc	dword [rbx+CSD_UIA_PROVIDER.refCount]
	xor	eax,eax
	ret

  query_interface_no_interface:
	mov	eax,E_NOINTERFACE
	ret

  query_interface_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaSimple_GetPatternProvider uses rbx rsi, thisp,patternId,pRetVal
	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	esi,edx
	test	r8,r8
	jz	simple_pattern_bad_pointer
	mov	qword [r8],0
	cmp	dword [rbx+CSD_UIA_PROVIDER.slot],CSD_UIA_TAB_SLOT_ROOT
	je	simple_pattern_done
	fastcall UiaProviderVisible,rbx
	test	eax,eax
	jz	simple_pattern_done
	cmp	esi,UIA_InvokePatternId
	je	simple_pattern_invoke
	cmp	esi,UIA_SelectionItemPatternId
	jne	simple_pattern_done
	fastcall UiaProviderIsBody,rbx
	test	eax,eax
	jz	simple_pattern_done
	lea	rdx,[rbx+CSD_UIA_SELECTION_OFFSET]
	jmp	simple_pattern_return

  simple_pattern_invoke:
	lea	rdx,[rbx+CSD_UIA_INVOKE_OFFSET]

  simple_pattern_return:
	mov	[r8],rdx
	inc	dword [rbx+CSD_UIA_PROVIDER.refCount]

  simple_pattern_done:
	xor	eax,eax
	ret

  simple_pattern_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaSimple_GetPropertyValue uses rbx rsi rdi, thisp,propertyId,pRetVal
	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	esi,edx
	mov	rdi,r8
	test	rdi,rdi
	jz	simple_property_bad_pointer
	fastcall CsdUiaVariantEmpty,rdi

	cmp	esi,UIA_NamePropertyId
	je	simple_property_name
	cmp	esi,UIA_AutomationIdPropertyId
	je	simple_property_automation_id
	cmp	esi,UIA_ControlTypePropertyId
	je	simple_property_control_type
	cmp	esi,UIA_IsEnabledPropertyId
	je	simple_property_enabled
	cmp	esi,UIA_IsKeyboardFocusablePropertyId
	je	simple_property_focusable
	cmp	esi,UIA_HasKeyboardFocusPropertyId
	je	simple_property_has_focus
	cmp	esi,UIA_SelectionItemIsSelectedPropertyId
	je	simple_property_is_selected
	cmp	esi,UIA_ItemStatusPropertyId
	je	simple_property_item_status
	xor	eax,eax
	ret

  simple_property_name:
	fastcall UiaTabNameForProvider,rbx
	test	rax,rax
	jz	simple_property_done_empty
	fastcall CsdUiaVariantBstr,rdi,rax
	ret

  simple_property_automation_id:
	fastcall UiaTabAutomationIdForProvider,rbx
	test	rax,rax
	jz	simple_property_done_empty
	fastcall CsdUiaVariantBstr,rdi,rax
	ret

  simple_property_control_type:
	cmp	dword [rbx+CSD_UIA_PROVIDER.slot],CSD_UIA_TAB_SLOT_ROOT
	je	simple_property_root_control
	fastcall UiaProviderIsBody,rbx
	test	eax,eax
	jz	simple_property_button_control
	fastcall CsdUiaVariantI4,rdi,UIA_TabItemControlTypeId
	ret

  simple_property_root_control:
	fastcall CsdUiaVariantI4,rdi,UIA_TabControlTypeId
	ret

  simple_property_button_control:
	fastcall CsdUiaVariantI4,rdi,UIA_ButtonControlTypeId
	ret

  simple_property_enabled:
	mov	edx,1
	cmp	dword [rbx+CSD_UIA_PROVIDER.slot],CSD_UIA_TAB_SLOT_ROOT
	je	simple_property_enabled_ready
	fastcall UiaProviderVisible,rbx
	mov	edx,eax
  simple_property_enabled_ready:
	fastcall CsdUiaVariantBool,rdi,edx
	ret

  simple_property_focusable:
	xor	edx,edx
	cmp	dword [rbx+CSD_UIA_PROVIDER.slot],CSD_UIA_TAB_SLOT_ROOT
	je	simple_property_focusable_ready
	fastcall UiaProviderVisible,rbx
	mov	edx,eax
  simple_property_focusable_ready:
	fastcall CsdUiaVariantBool,rdi,edx
	ret

  simple_property_has_focus:
	xor	r10d,r10d
	cmp	dword [tab_keyboard.active],0
	je	simple_property_has_focus_ready
	mov	ecx,[rbx+CSD_UIA_PROVIDER.slot]
	fastcall UiaSlotToIndexPart,ecx
	cmp	eax,[tab_keyboard.index]
	jne	simple_property_has_focus_ready
	cmp	edx,[tab_keyboard.part]
	sete	r10b
  simple_property_has_focus_ready:
	mov	edx,r10d
	fastcall CsdUiaVariantBool,rdi,edx
	ret

  simple_property_is_selected:
	fastcall UiaProviderIsBody,rbx
	test	eax,eax
	jz	simple_property_done_empty
	mov	ecx,[rbx+CSD_UIA_PROVIDER.slot]
	fastcall UiaSlotToIndexPart,ecx
	xor	edx,edx
	cmp	eax,[tabstrip.activeIndex]
	sete	dl
	fastcall CsdUiaVariantBool,rdi,edx
	ret

  simple_property_item_status:
	cmp	dword [rbx+CSD_UIA_PROVIDER.slot],CSD_UIA_OVERFLOW_SLOT
	jne	simple_property_done_empty
	fastcall UiaOverflowStatusString
	test	rax,rax
	jz	simple_property_done_empty
	fastcall CsdUiaVariantBstr,rdi,rax
	ret

  simple_property_done_empty:
	xor	eax,eax
	ret

  simple_property_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaSimple_HostRawElementProvider thisp,pRetVal
	mov	rax,[rcx+CSD_UIA_INTERFACE_OWNER]
	test	rdx,rdx
	jz	simple_host_bad_pointer
	mov	qword [rdx],0
	cmp	dword [rax+CSD_UIA_PROVIDER.slot],CSD_UIA_TAB_SLOT_ROOT
	jne	simple_host_done
	fastcall [pCsdUiaHostProviderFromHwnd],[hMain],rdx
	ret

  simple_host_done:
	xor	eax,eax
	ret

  simple_host_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaRoot_ElementProviderFromPoint uses rbx, thisp
    locals
	pt	POINT
	outp	dq ?
	tab_index dd ?
	tab_part dd ?
	slot	dd ?
    endl

	mov	[outp],r9
	test	r9,r9
	jz	root_point_bad_pointer
	mov	qword [r9],0
	cvttsd2si eax,xmm1
	mov	[pt.x],eax
	cvttsd2si eax,xmm2
	mov	[pt.y],eax
	invoke	ScreenToClient,[hMain],addr pt
	fastcall CaptionIndexFromClientPoint,dword [pt.x],dword [pt.y]
	cmp	eax,CSD_CAPTION_OVERFLOW_INDEX
	jne	root_point_check_tab
	cmp	dword [caption_visible+CSD_CAPTION_OVERFLOW_INDEX*4],0
	je	root_point_check_tab
	fastcall UiaProviderFromSlot,CSD_UIA_OVERFLOW_SLOT
	jmp	root_point_have_provider

  root_point_check_tab:
	fastcall TabPartFromClientPoint,dword [pt.x],dword [pt.y]
	cmp	eax,CSD_TAB_NO_INDEX
	je	root_point_done
	mov	[tab_index],eax
	mov	[tab_part],edx
	fastcall UiaSlotFromIndexPart,dword [tab_index],dword [tab_part]
	mov	[slot],eax
	cmp	eax,CSD_UIA_TAB_SLOT_ROOT
	je	root_point_done
	fastcall UiaProviderFromSlot,dword [slot]

  root_point_have_provider:
	test	rax,rax
	jz	root_point_done
	mov	rbx,rax
	fastcall UiaProviderVisible,rbx
	test	eax,eax
	jz	root_point_done
	fastcall CsdUiaReturnFragmentInterface,rbx,[outp]

  root_point_done:
	xor	eax,eax
	ret

  root_point_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaRoot_GetFocus thisp,pRetVal
    locals
	outp dq ?
	slot dd ?
    endl

	test	rdx,rdx
	jz	root_focus_bad_pointer
	mov	[outp],rdx
	mov	qword [rdx],0
	cmp	dword [tab_keyboard.active],0
	je	root_focus_done
	fastcall UiaSlotFromIndexPart,dword [tab_keyboard.index],\
		dword [tab_keyboard.part]
	mov	[slot],eax
	cmp	eax,CSD_UIA_TAB_SLOT_ROOT
	je	root_focus_done
	fastcall UiaProviderFromSlot,dword [slot]
	test	rax,rax
	jz	root_focus_done
	fastcall CsdUiaReturnFragmentInterface,rax,[outp]

  root_focus_done:
	xor	eax,eax
	ret

  root_focus_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaFragment_Navigate uses rbx rsi, thisp,direction,pRetVal
    locals
	outp dq ?
    endl

	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	esi,edx
	test	r8,r8
	jz	fragment_navigate_bad_pointer
	mov	[outp],r8
	mov	qword [r8],0

	cmp	dword [rbx+CSD_UIA_PROVIDER.slot],CSD_UIA_TAB_SLOT_ROOT
	jne	fragment_navigate_child
	cmp	esi,NavigateDirection_FirstChild
	je	fragment_navigate_root_first
	cmp	esi,NavigateDirection_LastChild
	je	fragment_navigate_root_last
	jmp	fragment_navigate_done

  fragment_navigate_root_first:
	fastcall UiaFirstVisibleProvider
	fastcall CsdUiaReturnFragmentInterface,rax,[outp]
	jmp	fragment_navigate_done

  fragment_navigate_root_last:
	fastcall UiaLastVisibleProvider
	fastcall CsdUiaReturnFragmentInterface,rax,[outp]
	jmp	fragment_navigate_done

  fragment_navigate_child:
	cmp	esi,NavigateDirection_Parent
	je	fragment_navigate_parent
	cmp	esi,NavigateDirection_NextSibling
	je	fragment_navigate_next
	cmp	esi,NavigateDirection_PreviousSibling
	je	fragment_navigate_previous
	jmp	fragment_navigate_done

  fragment_navigate_parent:
	fastcall CsdUiaReturnFragmentInterface,uia_root,[outp]
	jmp	fragment_navigate_done

  fragment_navigate_next:
	fastcall UiaNextVisibleProvider,rbx
	fastcall CsdUiaReturnFragmentInterface,rax,[outp]
	jmp	fragment_navigate_done

  fragment_navigate_previous:
	fastcall UiaPreviousVisibleProvider,rbx
	fastcall CsdUiaReturnFragmentInterface,rax,[outp]

  fragment_navigate_done:
	xor	eax,eax
	ret

  fragment_navigate_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaFragment_GetRuntimeId thisp,pRetVal
	mov	r8,rdx
	mov	rdx,[hMain]
	fastcall CsdUiaFragment_GetRuntimeId,rcx,rdx,r8
	ret
endp

proc UiaFragment_BoundingRectangle uses rbx rsi rdi, thisp,pRetVal
    locals
	pt0	POINT
	pt1	POINT
    endl

	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	rdi,rdx
	test	rdi,rdi
	jz	fragment_bounds_bad_pointer
	cmp	dword [rbx+CSD_UIA_PROVIDER.slot],CSD_UIA_TAB_SLOT_ROOT
	je	fragment_bounds_root

	mov	ecx,[rbx+CSD_UIA_PROVIDER.slot]
	fastcall UiaSlotToIndexPart,ecx
	cmp	edx,CSD_TAB_PART_OVERFLOW
	je	fragment_bounds_overflow
	cmp	eax,CSD_TAB_NO_INDEX
	je	fragment_bounds_empty
	imul	eax,sizeof.CSD_TAB_GEOMETRY
	lea	rsi,[tab_geometry+eax]
	cmp	edx,CSD_TAB_PART_CLOSE
	jne	fragment_bounds_body
	lea	rsi,[rsi+CSD_TAB_GEOMETRY.closeRect]
	jmp	fragment_bounds_copy

  fragment_bounds_overflow:
	lea	rsi,[caption_geometry+CSD_CAPTION_OVERFLOW_INDEX*sizeof.CSD_CAPTION_GEOMETRY]
	jmp	fragment_bounds_copy

  fragment_bounds_body:
	lea	rsi,[rsi+CSD_TAB_GEOMETRY.tabRect]

  fragment_bounds_copy:
	mov	eax,[rsi+RECT.left]
	mov	[pt0.x],eax
	mov	eax,[rsi+RECT.top]
	mov	[pt0.y],eax
	mov	eax,[rsi+RECT.right]
	mov	[pt1.x],eax
	mov	eax,[rsi+RECT.bottom]
	mov	[pt1.y],eax
	jmp	fragment_bounds_screen

  fragment_bounds_empty:
	xor	eax,eax
	mov	[pt0.x],eax
	mov	[pt0.y],eax
	mov	[pt1.x],eax
	mov	[pt1.y],eax
	jmp	fragment_bounds_screen

  fragment_bounds_root:
	invoke	GetClientRect,[hMain],addr client_rect
	mov	eax,[client_rect.left]
	mov	[pt0.x],eax
	mov	eax,[client_rect.top]
	mov	[pt0.y],eax
	mov	eax,[client_rect.right]
	mov	[pt1.x],eax
	mov	eax,[client_rect.bottom]
	mov	[pt1.y],eax

  fragment_bounds_screen:
	invoke	ClientToScreen,[hMain],addr pt0
	invoke	ClientToScreen,[hMain],addr pt1
	cvtsi2sd xmm0,dword [pt0.x]
	movsd	[rdi+UIA_RECT.left],xmm0
	cvtsi2sd xmm0,dword [pt0.y]
	movsd	[rdi+UIA_RECT.top],xmm0
	mov	eax,[pt1.x]
	sub	eax,[pt0.x]
	cvtsi2sd xmm0,eax
	movsd	[rdi+UIA_RECT.width],xmm0
	mov	eax,[pt1.y]
	sub	eax,[pt0.y]
	cvtsi2sd xmm0,eax
	movsd	[rdi+UIA_RECT.height],xmm0
	xor	eax,eax
	ret

  fragment_bounds_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaFragment_EmbeddedFragmentRoots thisp,pRetVal
	test	rdx,rdx
	jz	fragment_embedded_bad_pointer
	mov	qword [rdx],0
	xor	eax,eax
	ret

  fragment_embedded_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaFragment_SetFocus uses rbx, thisp
    locals
	tab_index dd ?
	tab_part dd ?
    endl

	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	cmp	dword [rbx+CSD_UIA_PROVIDER.slot],CSD_UIA_TAB_SLOT_ROOT
	je	fragment_set_focus_done
	fastcall UiaProviderVisible,rbx
	test	eax,eax
	jz	fragment_set_focus_done
	mov	ecx,[rbx+CSD_UIA_PROVIDER.slot]
	fastcall UiaSlotToIndexPart,ecx
	mov	[tab_index],eax
	mov	[tab_part],edx
	fastcall SetTabKeyboardFocus,[hMain],dword [tab_index],dword [tab_part]

  fragment_set_focus_done:
	xor	eax,eax
	ret
endp

proc UiaFragment_FragmentRoot thisp,pRetVal
	test	rdx,rdx
	jz	fragment_root_bad_pointer
	lea	rax,[uia_root+CSD_UIA_ROOT_OFFSET]
	mov	[rdx],rax
	inc	dword [uia_root+CSD_UIA_PROVIDER.refCount]
	xor	eax,eax
	ret

  fragment_root_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaInvoke_Invoke uses rbx, thisp
    locals
	tab_index dd ?
	tab_part dd ?
    endl

	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	fastcall UiaProviderVisible,rbx
	test	eax,eax
	jz	uia_invoke_fail
	mov	ecx,[rbx+CSD_UIA_PROVIDER.slot]
	fastcall UiaSlotToIndexPart,rcx
	mov	[tab_index],eax
	mov	[tab_part],edx
	fastcall SetTabKeyboardFocus,[hMain],dword [tab_index],dword [tab_part]
	fastcall InvokeTabKeyboardFocus,[hMain]
	test	eax,eax
	jz	uia_invoke_fail
	xor	eax,eax
	ret

  uia_invoke_fail:
	mov	eax,E_FAIL
	ret
endp

proc UiaSelection_Select uses rbx, thisp
    locals
	tab_index dd ?
    endl

	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	fastcall UiaProviderVisible,rbx
	test	eax,eax
	jz	uia_selection_select_fail
	mov	ecx,[rbx+CSD_UIA_PROVIDER.slot]
	fastcall UiaSlotToIndexPart,rcx
	cmp	edx,CSD_TAB_PART_BODY
	jne	uia_selection_select_fail
	mov	[tab_index],eax
	fastcall SetTabKeyboardFocus,[hMain],dword [tab_index],CSD_TAB_PART_BODY
	fastcall InvokeTabKeyboardFocus,[hMain]
	test	eax,eax
	jz	uia_selection_select_fail
	xor	eax,eax
	ret

  uia_selection_select_fail:
	mov	eax,E_FAIL
	ret
endp

proc UiaSelection_AddToSelection thisp
	mov	eax,E_NOTIMPL
	ret
endp

proc UiaSelection_RemoveFromSelection thisp
	mov	eax,E_NOTIMPL
	ret
endp

proc UiaSelection_IsSelected uses rbx rsi, thisp,pRetVal
	test	rdx,rdx
	jz	uia_selection_selected_bad_pointer
	mov	rsi,rdx
	mov	dword [rdx],0
	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	ecx,[rbx+CSD_UIA_PROVIDER.slot]
	fastcall UiaSlotToIndexPart,ecx
	cmp	edx,CSD_TAB_PART_BODY
	jne	uia_selection_selected_done
	cmp	eax,[tabstrip.activeIndex]
	sete	cl
	movzx	ecx,cl
	mov	[rsi],ecx

  uia_selection_selected_done:
	xor	eax,eax
	ret

  uia_selection_selected_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaSelection_SelectionContainer thisp,pRetVal
	fastcall CsdUiaSelection_SelectionContainer,uia_root,rdx
	ret
endp

proc HandleGetObject hwnd,wparam,lparam
	cmp	dword [uia_initialized],0
	je	getobject_no
	fastcall CsdUiaHandleGetObject,rcx,rdx,r8,uia_root+CSD_UIA_SIMPLE_OFFSET
	ret

  getobject_no:
	xor	eax,eax
	ret
endp


proc UpdateThemeStatus uses rbx rsi rdi r12
	fastcall LayoutDirectionName
	mov	r12,rax
	mov	rbx,snap_off
	cmp	dword [snap_policy.snapCapable],0
	je	status_snap_ready
	mov	rbx,snap_on
  status_snap_ready:
	mov	ecx,[backdrop_policy.requestedType]
	fastcall BackdropNameForType,ecx
	mov	rsi,rax
	fastcall BackdropCapabilityLabel
	mov	rdi,rax
	invoke	wsprintfW,addr status_buffer,status_format,\
		r12,\
		dword [tabstrip.count],CSD_TAB_CAPACITY,\
		dword [tabstrip.activeIndex],dword [caption_hidden_count],\
		dword [caption_overflow_count],rsi,dword [backdrop_policy.osBuild],\
		rdi,[hSearchEdit],rbx
	mov	rax,status_buffer
	mov	[status_message],rax
	ret
endp

proc InitTabs
	fastcall CsdTabStripBind,addr tabstrip,tab_items,tab_geometry,tab_state,\
		CSD_TAB_CAPACITY
	mov	dword [tab_next_id],1
	fastcall TabTitleForIndex,0
	fastcall InitOneTab,0,rax
	fastcall TabTitleForIndex,1
	fastcall InitOneTab,1,rax
	fastcall TabTitleForIndex,2
	fastcall InitOneTab,2,rax
	mov	dword [tabstrip.count],CSD_TAB_INITIAL_COUNT
	mov	dword [tabstrip.activeIndex],0
	fastcall CsdTabStripClearInput,addr tabstrip
	fastcall ApplyTabStates
	ret
endp

proc LayoutTabs uses rbx rsi rdi r12 r13 r14 r15, bandp
    locals
	min_w		dd ?
	pref_w		dd ?
	close_w		dd ?
	pad_w		dd ?
	gap_w		dd ?
	avail_w		dd ?
	visible_count	dd ?
	start_index	dd ?
	tab_w		dd ?
	xpos		dd ?
	tab_index	dd ?
	top		dd ?
	bottom		dd ?
	right_edge	dd ?
    endl

	mov	rbx,rcx
	mov	dword [caption_drag_rect_count],0
	mov	rsi,tab_items
	mov	rdi,tab_geometry
	mov	r12d,CSD_TAB_CAPACITY

  layout_clear_loop:
	mov	dword [rsi+CSD_TAB_ITEM.flags],0
	fastcall CsdTabGeometryClear,rdi
	add	rsi,sizeof.CSD_TAB_ITEM
	add	rdi,sizeof.CSD_TAB_GEOMETRY
	dec	r12d
	jnz	layout_clear_loop

	fastcall CsdDipToPx,CSD_TAB_MIN_WIDTH_DIP,dword [current_dpi]
	mov	[min_w],eax
	fastcall CsdDipToPx,CSD_TAB_PREF_WIDTH_DIP,dword [current_dpi]
	mov	[pref_w],eax
	fastcall CsdDipToPx,CSD_TAB_CLOSE_DIP,dword [current_dpi]
	mov	[close_w],eax
	fastcall CsdDipToPx,CSD_TAB_PAD_DIP,dword [current_dpi]
	mov	[pad_w],eax
	fastcall CsdDipToPx,CSD_TAB_GAP_DIP,dword [current_dpi]
	mov	[gap_w],eax

	mov	eax,[rbx+RECT.top]
	mov	[top],eax
	mov	eax,[rbx+RECT.bottom]
	mov	[bottom],eax
	mov	eax,[rbx+RECT.right]
	mov	[right_edge],eax
	sub	eax,[rbx+RECT.left]
	mov	[avail_w],eax
	cmp	eax,[min_w]
	jl	layout_all_drag
	mov	eax,[tabstrip.count]
	test	eax,eax
	jz	layout_all_drag

	mov	eax,[avail_w]
	add	eax,[gap_w]
	mov	ecx,[min_w]
	add	ecx,[gap_w]
	xor	edx,edx
	div	ecx
	test	eax,eax
	jz	layout_all_drag
	cmp	eax,[tabstrip.count]
	jbe	layout_visible_ready
	mov	eax,[tabstrip.count]

  layout_visible_ready:
	mov	[visible_count],eax
	mov	eax,[tabstrip.activeIndex]
	test	eax,eax
	jge	layout_active_nonnegative
	xor	eax,eax

  layout_active_nonnegative:
	cmp	eax,[tabstrip.count]
	jl	layout_active_ready
	mov	eax,[tabstrip.count]
	dec	eax

  layout_active_ready:
	cmp	eax,[visible_count]
	jl	layout_start_zero
	sub	eax,[visible_count]
	inc	eax
	jmp	layout_start_clamp

  layout_start_zero:
	xor	eax,eax

  layout_start_clamp:
	mov	[start_index],eax
	add	eax,[visible_count]
	cmp	eax,[tabstrip.count]
	jle	layout_width
	mov	eax,[tabstrip.count]
	sub	eax,[visible_count]
	mov	[start_index],eax

  layout_width:
	mov	eax,[visible_count]
	dec	eax
	imul	eax,[gap_w]
	mov	ecx,[avail_w]
	sub	ecx,eax
	mov	eax,ecx
	xor	edx,edx
	div	dword [visible_count]
	cmp	eax,[pref_w]
	jbe	layout_width_min
	mov	eax,[pref_w]

  layout_width_min:
	cmp	eax,[min_w]
	jae	layout_width_ready
	mov	eax,[min_w]

  layout_width_ready:
	mov	[tab_w],eax
	test	dword [layout_flags],CSD_LAYOUT_RTL
	jnz	layout_rtl_start

	mov	eax,[rbx+RECT.left]
	mov	[xpos],eax
	mov	eax,[start_index]
	mov	[tab_index],eax
	mov	r15d,[visible_count]

  layout_tab_loop:
	mov	eax,[tab_index]
	imul	eax,sizeof.CSD_TAB_ITEM
	lea	rsi,[tab_items+eax]
	or	dword [rsi+CSD_TAB_ITEM.flags],CSD_TAB_FLAG_VISIBLE
	mov	eax,[tab_index]
	imul	eax,sizeof.CSD_TAB_GEOMETRY
	lea	rdi,[tab_geometry+eax]

	mov	eax,[xpos]
	mov	[rdi+CSD_TAB_GEOMETRY.tabRect.left],eax
	mov	eax,[top]
	mov	[rdi+CSD_TAB_GEOMETRY.tabRect.top],eax
	mov	eax,[xpos]
	add	eax,[tab_w]
	mov	[rdi+CSD_TAB_GEOMETRY.tabRect.right],eax
	mov	eax,[bottom]
	mov	[rdi+CSD_TAB_GEOMETRY.tabRect.bottom],eax

	mov	eax,[rdi+CSD_TAB_GEOMETRY.tabRect.right]
	sub	eax,[pad_w]
	mov	[rdi+CSD_TAB_GEOMETRY.closeRect.right],eax
	sub	eax,[close_w]
	mov	[rdi+CSD_TAB_GEOMETRY.closeRect.left],eax
	mov	eax,[top]
	mov	[rdi+CSD_TAB_GEOMETRY.closeRect.top],eax
	mov	eax,[bottom]
	mov	[rdi+CSD_TAB_GEOMETRY.closeRect.bottom],eax

	mov	eax,[rdi+CSD_TAB_GEOMETRY.tabRect.left]
	add	eax,[pad_w]
	mov	[rdi+CSD_TAB_GEOMETRY.titleRect.left],eax
	mov	eax,[rdi+CSD_TAB_GEOMETRY.closeRect.left]
	sub	eax,[pad_w]
	mov	[rdi+CSD_TAB_GEOMETRY.titleRect.right],eax
	mov	eax,[top]
	mov	[rdi+CSD_TAB_GEOMETRY.titleRect.top],eax
	mov	eax,[bottom]
	mov	[rdi+CSD_TAB_GEOMETRY.titleRect.bottom],eax

	mov	eax,[rdi+CSD_TAB_GEOMETRY.tabRect.right]
	mov	[xpos],eax
	dec	r15d
	jz	layout_after_tabs
	mov	r13d,[xpos]
	mov	r14d,r13d
	add	r14d,[gap_w]
	fastcall AddCaptionDragRect,r13d,dword [top],r14d,dword [bottom]
	mov	[xpos],r14d
	inc	dword [tab_index]
	jmp	layout_tab_loop

  layout_after_tabs:
	fastcall AddCaptionDragRect,dword [xpos],dword [top],\
		dword [right_edge],dword [bottom]
	fastcall ApplyTabStates
	ret

  layout_rtl_start:
	mov	eax,[rbx+RECT.right]
	mov	[xpos],eax
	mov	eax,[start_index]
	mov	[tab_index],eax
	mov	r15d,[visible_count]

  layout_rtl_loop:
	mov	eax,[tab_index]
	imul	eax,sizeof.CSD_TAB_ITEM
	lea	rsi,[tab_items+eax]
	or	dword [rsi+CSD_TAB_ITEM.flags],CSD_TAB_FLAG_VISIBLE
	mov	eax,[tab_index]
	imul	eax,sizeof.CSD_TAB_GEOMETRY
	lea	rdi,[tab_geometry+eax]

	mov	eax,[xpos]
	mov	[rdi+CSD_TAB_GEOMETRY.tabRect.right],eax
	sub	eax,[tab_w]
	mov	[rdi+CSD_TAB_GEOMETRY.tabRect.left],eax
	mov	eax,[top]
	mov	[rdi+CSD_TAB_GEOMETRY.tabRect.top],eax
	mov	eax,[bottom]
	mov	[rdi+CSD_TAB_GEOMETRY.tabRect.bottom],eax

	mov	eax,[rdi+CSD_TAB_GEOMETRY.tabRect.left]
	add	eax,[pad_w]
	mov	[rdi+CSD_TAB_GEOMETRY.closeRect.left],eax
	add	eax,[close_w]
	mov	[rdi+CSD_TAB_GEOMETRY.closeRect.right],eax
	mov	eax,[top]
	mov	[rdi+CSD_TAB_GEOMETRY.closeRect.top],eax
	mov	eax,[bottom]
	mov	[rdi+CSD_TAB_GEOMETRY.closeRect.bottom],eax

	mov	eax,[rdi+CSD_TAB_GEOMETRY.closeRect.right]
	add	eax,[pad_w]
	mov	[rdi+CSD_TAB_GEOMETRY.titleRect.left],eax
	mov	eax,[rdi+CSD_TAB_GEOMETRY.tabRect.right]
	sub	eax,[pad_w]
	mov	[rdi+CSD_TAB_GEOMETRY.titleRect.right],eax
	mov	eax,[top]
	mov	[rdi+CSD_TAB_GEOMETRY.titleRect.top],eax
	mov	eax,[bottom]
	mov	[rdi+CSD_TAB_GEOMETRY.titleRect.bottom],eax

	mov	eax,[rdi+CSD_TAB_GEOMETRY.tabRect.left]
	mov	[xpos],eax
	dec	r15d
	jz	layout_rtl_after_tabs
	mov	r14d,[xpos]
	mov	r13d,r14d
	sub	r13d,[gap_w]
	fastcall AddCaptionDragRect,r13d,dword [top],r14d,dword [bottom]
	mov	[xpos],r13d
	inc	dword [tab_index]
	jmp	layout_rtl_loop

  layout_rtl_after_tabs:
	fastcall AddCaptionDragRect,dword [rbx+RECT.left],dword [top],\
		dword [xpos],dword [bottom]
	fastcall ApplyTabStates
	ret

  layout_all_drag:
	fastcall AddCaptionDragRect,dword [rbx+RECT.left],dword [rbx+RECT.top],\
		dword [rbx+RECT.right],dword [rbx+RECT.bottom]
	fastcall ApplyTabStates
	ret
endp

proc SetCaptionVisible index,visible
	mov	[caption_visible+rcx*4],edx
	ret
endp

proc ComputeMinimumTrackClientWidth uses rbx
    locals
	edge_w		dd ?
	button_w	dd ?
	tab_min_w	dd ?
	drag_pad_w	dd ?
	caption_w	dd ?
	body_w		dd ?
    endl

	mov	ebx,[current_dpi]
	test	ebx,ebx
	jnz	min_track_have_dpi
	mov	ebx,CSD_DPI_BASE

  min_track_have_dpi:
	fastcall CsdDipToPx,CSD_EDGE_DIP,ebx
	mov	[edge_w],eax
	fastcall CsdDipToPx,CSD_BUTTON_WIDTH_DIP,ebx
	mov	[button_w],eax
	fastcall CsdDipToPx,CSD_TAB_MIN_WIDTH_DIP,ebx
	mov	[tab_min_w],eax
	fastcall CsdDipToPx,CSD_DRAG_PAD_DIP,ebx
	mov	[drag_pad_w],eax
	fastcall CsdDipToPx,CSD_MIN_BODY_W_DIP,ebx
	mov	[body_w],eax

	mov	eax,[edge_w]
	shl	eax,1
	mov	[caption_w],eax
	mov	eax,[button_w]
	imul	eax,5
	add	[caption_w],eax
	mov	eax,[tab_min_w]
	add	[caption_w],eax
	mov	eax,[drag_pad_w]
	shl	eax,1
	add	[caption_w],eax

	mov	eax,[caption_w]
	cmp	eax,[body_w]
	jae	min_track_width_done
	mov	eax,[body_w]

  min_track_width_done:
	ret
endp

proc ComputeResponsivePolicy uses rbx rsi rdi, clientp
    locals
	available_w	dd ?
	edge_w		dd ?
	button_w	dd ?
	search_w	dd ?
	tab_min_w	dd ?
	drag_pad_w	dd ?
	required_w	dd ?
    endl

	mov	rsi,rcx
	mov	rdi,caption_visible
	mov	ecx,CSD_CAPTION_CONTROL_COUNT
	mov	eax,1
	rep	stosd
	mov	dword [caption_visible+CSD_CAPTION_OVERFLOW_INDEX*4],0
	mov	dword [caption_hidden_count],0
	mov	dword [caption_overflow_count],0

	mov	eax,[rsi+RECT.right]
	sub	eax,[rsi+RECT.left]
	mov	[available_w],eax
	mov	ebx,[current_dpi]
	test	ebx,ebx
	jnz	responsive_have_dpi
	mov	ebx,CSD_DPI_BASE

  responsive_have_dpi:
	fastcall CsdDipToPx,CSD_EDGE_DIP,ebx
	mov	[edge_w],eax
	fastcall CsdDipToPx,CSD_BUTTON_WIDTH_DIP,ebx
	mov	[button_w],eax
	fastcall CsdDipToPx,\
		dword [caption_responsive+CSD_CAPTION_SEARCH_INDEX*sizeof.CSD_CAPTION_RESPONSIVE+CSD_CAPTION_RESPONSIVE.minWidthDip],\
		ebx
	mov	[search_w],eax
	fastcall CsdDipToPx,CSD_TAB_MIN_WIDTH_DIP,ebx
	mov	[tab_min_w],eax
	fastcall CsdDipToPx,CSD_DRAG_PAD_DIP,ebx
	mov	[drag_pad_w],eax

	mov	eax,[edge_w]
	shl	eax,1
	mov	[required_w],eax
	mov	eax,[button_w]
	imul	eax,6
	add	[required_w],eax
	mov	eax,[search_w]
	add	[required_w],eax
	mov	eax,[tab_min_w]
	add	[required_w],eax
	mov	eax,[drag_pad_w]
	shl	eax,1
	add	[required_w],eax

	mov	eax,[available_w]
	cmp	eax,[required_w]
	jae	responsive_policy_minimum
	fastcall SetCaptionVisible,CSD_CAPTION_SEARCH_INDEX,0
	inc	dword [caption_hidden_count]
	inc	dword [caption_overflow_count]
	mov	eax,[search_w]
	sub	[required_w],eax
	mov	eax,[button_w]
	add	[required_w],eax
	fastcall SetCaptionVisible,CSD_CAPTION_OVERFLOW_INDEX,1

	mov	eax,[available_w]
	cmp	eax,[required_w]
	jae	responsive_policy_minimum
	fastcall SetCaptionVisible,CSD_CAPTION_SETTINGS_INDEX,0
	inc	dword [caption_hidden_count]
	inc	dword [caption_overflow_count]
	mov	eax,[button_w]
	sub	[required_w],eax

	mov	eax,[available_w]
	cmp	eax,[required_w]
	jae	responsive_policy_minimum
	fastcall SetCaptionVisible,CSD_CAPTION_NEWTAB_INDEX,0
	inc	dword [caption_hidden_count]
	inc	dword [caption_overflow_count]
	mov	eax,[button_w]
	sub	[required_w],eax

  responsive_policy_minimum:
	fastcall ComputeMinimumTrackClientWidth
	mov	[min_track_client_w],eax
	fastcall CsdDipToPx,CSD_MIN_BODY_H_DIP,dword [current_dpi]
	mov	[min_track_client_h],eax
	ret
endp

proc LayoutCaptionControls uses rbx rsi rdi r12 r13 r14 r15
    locals
	left_x		dd ?
	right_x		dd ?
	title_top	dd ?
	title_bottom	dd ?
	drag_pad	dd ?
	control_w	dd ?
    endl

	invoke	GetClientRect,[hMain],addr client_rect
	fastcall ComputeResponsivePolicy,addr client_rect
	mov	ebx,[current_dpi]
	fastcall CsdDipToPx,CSD_EDGE_DIP,ebx
	mov	[title_top],eax
	mov	edx,[client_rect.left]
	add	edx,eax
	mov	[left_x],edx
	mov	edx,[client_rect.right]
	sub	edx,eax
	mov	[right_x],edx
	fastcall CsdDipToPx,CSD_TITLE_HEIGHT_DIP,ebx
	add	eax,[client_rect.top]
	mov	[title_bottom],eax
	fastcall CsdDipToPx,CSD_DRAG_PAD_DIP,ebx
	mov	[drag_pad],eax

	mov	rsi,caption_descriptors
	mov	rdi,caption_geometry
	mov	r12,caption_visible
	mov	r15d,CSD_CAPTION_CONTROL_COUNT

  layout_caption_loop:
	lea	rcx,[rdi+CSD_CAPTION_GEOMETRY.rect]
	fastcall CsdRectClear,rcx
	cmp	dword [r12],0
	je	layout_caption_next

	mov	ecx,[rsi+CSD_CAPTION_DESCRIPTOR.widthDip]
	fastcall CsdDipToPx,ecx,ebx
	mov	[control_w],eax
	test	dword [layout_flags],CSD_LAYOUT_RTL
	jnz	layout_caption_rtl
	test	[rsi+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_ALIGN_TRAILING
	jz	layout_caption_leading

	mov	eax,[right_x]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.right],eax
	sub	eax,[control_w]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.left],eax
	mov	[right_x],eax
	jmp	layout_caption_vertical

  layout_caption_rtl:
	test	[rsi+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_ALIGN_TRAILING
	jz	layout_caption_rtl_leading

	mov	eax,[left_x]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.left],eax
	add	eax,[control_w]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.right],eax
	mov	[left_x],eax
	jmp	layout_caption_vertical

  layout_caption_rtl_leading:
	mov	eax,[right_x]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.right],eax
	sub	eax,[control_w]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.left],eax
	mov	[right_x],eax
	jmp	layout_caption_vertical

  layout_caption_leading:
	mov	eax,[left_x]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.left],eax
	add	eax,[control_w]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.right],eax
	mov	[left_x],eax

  layout_caption_vertical:
	mov	eax,[title_top]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.top],eax
	mov	eax,[title_bottom]
	mov	[rdi+CSD_CAPTION_GEOMETRY.rect.bottom],eax

  layout_caption_next:
	add	rsi,sizeof.CSD_CAPTION_DESCRIPTOR
	add	rdi,sizeof.CSD_CAPTION_GEOMETRY
	add	r12,4
	dec	r15d
	jnz	layout_caption_loop

	mov	eax,[left_x]
	add	eax,[drag_pad]
	mov	[caption_drag_rect.left],eax
	mov	eax,[right_x]
	sub	eax,[drag_pad]
	mov	[caption_drag_rect.right],eax
	mov	eax,[title_top]
	mov	[caption_drag_rect.top],eax
	mov	eax,[title_bottom]
	mov	[caption_drag_rect.bottom],eax
	fastcall LayoutTabs,addr caption_drag_rect
	fastcall NormalizeTabKeyboard
	ret
endp

proc CreateCaptionChildren hwnd
	mov	[hwnd],rcx
	cmp	qword [hSearchEdit],0
	jne	create_children_done
	invoke	CreateWindowExW,WS_EX_CLIENTEDGE or WS_EX_NOINHERITLAYOUT,'EDIT','Search',\
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
	test	rcx,rcx
	jz	layout_children_done
	cmp	dword [caption_visible+CSD_CAPTION_SEARCH_INDEX*4],0
	jne	layout_children_show
	invoke	GetFocus
	cmp	rax,[hSearchEdit]
	jne	layout_children_hide
	invoke	SetFocus,[hMain]

  layout_children_hide:
	invoke	ShowWindow,[hSearchEdit],SW_HIDE
	ret

  layout_children_show:
	invoke	ShowWindow,[hSearchEdit],SW_SHOW
	fastcall CsdCaptionMoveChild,[hSearchEdit],caption_geometry,CSD_CAPTION_SEARCH_INDEX,\
		dword [current_dpi],CSD_CHILD_PAD_X_DIP,CSD_CHILD_PAD_Y_DIP

  layout_children_done:
	ret
endp

proc TabPartFromClientPoint uses rbx rsi rdi r12, x,y
    locals
	xpos dd ?
	ypos dd ?
    endl

	mov	[xpos],ecx
	mov	[ypos],edx
	xor	ebx,ebx
	mov	edi,[tabstrip.count]
	test	edi,edi
	jz	tab_part_none
	mov	rsi,tab_geometry
	mov	r12,tab_items

  tab_part_loop:
	test	dword [r12+CSD_TAB_ITEM.flags],CSD_TAB_FLAG_VISIBLE
	jz	tab_part_next
	fastcall CsdRectContains,addr rsi+CSD_TAB_GEOMETRY.closeRect,\
		dword [xpos],dword [ypos]
	test	eax,eax
	jnz	tab_part_close
	fastcall CsdRectContains,addr rsi+CSD_TAB_GEOMETRY.tabRect,\
		dword [xpos],dword [ypos]
	test	eax,eax
	jnz	tab_part_body

  tab_part_next:
	add	rsi,sizeof.CSD_TAB_GEOMETRY
	add	r12,sizeof.CSD_TAB_ITEM
	inc	ebx
	dec	edi
	jnz	tab_part_loop

  tab_part_none:
	mov	eax,CSD_TAB_NO_INDEX
	mov	edx,CSD_TAB_PART_NONE
	ret

  tab_part_close:
	mov	eax,ebx
	mov	edx,CSD_TAB_PART_CLOSE
	ret

  tab_part_body:
	mov	eax,ebx
	mov	edx,CSD_TAB_PART_BODY
	ret
endp

proc PointInCaptionDragRects uses rbx rsi, x,y
    locals
	xpos dd ?
	ypos dd ?
    endl

	mov	[xpos],ecx
	mov	[ypos],edx
	mov	ebx,[caption_drag_rect_count]
	test	ebx,ebx
	jz	drag_rect_no
	mov	rsi,caption_drag_rects

  drag_rect_loop:
	fastcall CsdRectContains,rsi,dword [xpos],dword [ypos]
	test	eax,eax
	jnz	drag_rect_yes
	add	rsi,sizeof.RECT
	dec	ebx
	jnz	drag_rect_loop

  drag_rect_no:
	xor	eax,eax
	ret

  drag_rect_yes:
	mov	eax,1
	ret
endp

proc DrawCaptionControls uses rbx rsi rdi r12 r13 r14 r15, hdc
    locals
	focus_rect	RECT
	stock_brush	dq ?
	from_fill	dd ?
	to_fill		dd ?
	fill_color	dd ?
	from_glyph	dd ?
	glyph_color	dd ?
	phase		dd ?
    endl

	mov	rbx,rcx
	mov	rsi,caption_descriptors
	mov	rdi,caption_geometry
	mov	r15,caption_state
	mov	r14d,CSD_CAPTION_CONTROL_COUNT
	invoke	GetStockObject,DC_BRUSH
	mov	[stock_brush],rax

  draw_loop:
	lea	r12,[rdi+CSD_CAPTION_GEOMETRY.rect]
	test	[rsi+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_CHILD
	jnz	draw_next

	movzx	edx,byte [r15+CSD_CAPTION_STATE.fromValue]
	fastcall CaptionFillColorForState,rsi,edx
	mov	[from_fill],eax
	movzx	edx,byte [r15+CSD_CAPTION_STATE.value]
	fastcall CaptionFillColorForState,rsi,edx
	mov	[to_fill],eax
	movzx	r8d,byte [r15+CSD_CAPTION_STATE.fadePhase]
	mov	[phase],r8d
	fastcall CsdBlendColorref,dword [from_fill],dword [to_fill],r8d
	mov	[fill_color],eax
	invoke	SetDCBrushColor,rbx,dword [fill_color]
	invoke	FillRect,rbx,r12,[stock_brush]

	movzx	ecx,byte [r15+CSD_CAPTION_STATE.fromValue]
	fastcall CaptionGlyphColorForState,ecx
	mov	[from_glyph],eax
	movzx	ecx,byte [r15+CSD_CAPTION_STATE.value]
	fastcall CaptionGlyphColorForState,ecx
	mov	[glyph_color],eax
	fastcall CsdBlendColorref,dword [from_glyph],dword [glyph_color],dword [phase]
	mov	[glyph_color],eax

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
	cmp	dword [tab_keyboard.active],0
	je	draw_next
	cmp	dword [tab_keyboard.index],CSD_TAB_KEYBOARD_OVERFLOW_INDEX
	jne	draw_next
	cmp	dword [tab_keyboard.part],CSD_TAB_PART_OVERFLOW
	jne	draw_next
	cmp	dword [rsi+CSD_CAPTION_DESCRIPTOR.id],ID_CAPTION_OVERFLOW
	jne	draw_next
	invoke	CopyRect,addr focus_rect,r12
	inc	dword [focus_rect.left]
	inc	dword [focus_rect.top]
	dec	dword [focus_rect.right]
	dec	dword [focus_rect.bottom]
	invoke	DrawFocusRect,rbx,addr focus_rect

  draw_next:
	add	rsi,sizeof.CSD_CAPTION_DESCRIPTOR
	add	rdi,sizeof.CSD_CAPTION_GEOMETRY
	add	r15,sizeof.CSD_CAPTION_STATE
	dec	r14d
	jnz	draw_loop
	ret
endp

proc DrawTabs uses rbx rsi rdi r12 r13 r14 r15, hdc
    locals
	focus_rect	RECT
	stock_brush	dq ?
	old_bk		dd ?
	old_color	dd ?
	old_font	dq ?
	fill_color	dd ?
	text_color	dd ?
	text_flags	dd ?
    endl

	mov	rbx,rcx
	invoke	GetStockObject,DC_BRUSH
	mov	[stock_brush],rax
	invoke	SetBkMode,rbx,TRANSPARENT
	mov	[old_bk],eax
	invoke	SelectObject,rbx,[hTextFont]
	mov	[old_font],rax
	invoke	SetTextColor,rbx,dword [current_theme.bodyTextColor]
	mov	[old_color],eax
	mov	dword [text_flags],\
		DT_LEFT or DT_VCENTER or DT_SINGLELINE or DT_END_ELLIPSIS or DT_NOPREFIX
	test	dword [layout_flags],CSD_LAYOUT_RTL
	jz	draw_tabs_direction_ready
	mov	dword [text_flags],\
		DT_RIGHT or DT_VCENTER or DT_SINGLELINE or DT_END_ELLIPSIS or DT_NOPREFIX or DT_RTLREADING

  draw_tabs_direction_ready:

	xor	r14d,r14d
	mov	r15d,[tabstrip.count]
	test	r15d,r15d
	jz	draw_tabs_done
	mov	rsi,tab_items
	mov	rdi,tab_geometry
	mov	r12,tab_state

  draw_tabs_loop:
	test	dword [rsi+CSD_TAB_ITEM.flags],CSD_TAB_FLAG_VISIBLE
	jz	draw_tabs_next
	movzx	ecx,byte [r12+CSD_TAB_STATE.value]
	fastcall TabFillColorForState,ecx
	mov	[fill_color],eax
	invoke	SetDCBrushColor,rbx,dword [fill_color]
	invoke	FillRect,rbx,addr rdi+CSD_TAB_GEOMETRY.tabRect,[stock_brush]
	invoke	FrameRect,rbx,addr rdi+CSD_TAB_GEOMETRY.tabRect,[hEdgeBrush]

	movzx	ecx,byte [r12+CSD_TAB_STATE.value]
	fastcall TabTextColorForState,ecx
	mov	[text_color],eax
	invoke	SetTextColor,rbx,dword [text_color]
	invoke	DrawTextW,rbx,[rsi+CSD_TAB_ITEM.titlep],-1,\
		addr rdi+CSD_TAB_GEOMETRY.titleRect,\
		dword [text_flags]

	cmp	r14d,[tabstrip.activeIndex]
	je	draw_tab_close
	cmp	r14d,[tabstrip.hotIndex]
	je	draw_tab_close
	cmp	dword [tab_keyboard.active],0
	je	draw_tab_focus
	cmp	r14d,[tab_keyboard.index]
	jne	draw_tab_focus
	cmp	dword [tab_keyboard.part],CSD_TAB_PART_CLOSE
	jne	draw_tab_focus

  draw_tab_close:
	mov	eax,[current_theme.titleTextColor]
	cmp	byte [r12+CSD_TAB_STATE.closeValue],CSD_STATE_PRESSED
	jne	draw_tab_close_not_pressed
	mov	eax,CSD_STATE_CLOSE_PRESSED
	jmp	draw_tab_close_color

  draw_tab_close_not_pressed:
	cmp	byte [r12+CSD_TAB_STATE.closeValue],CSD_STATE_HOVER
	jne	draw_tab_close_color
	mov	eax,CSD_STATE_CLOSE_HOVER

  draw_tab_close_color:
	fastcall FontIcon_DrawGlyph,rbx,ICON_CLOSE,\
		addr rdi+CSD_TAB_GEOMETRY.closeRect,[hCaptionFont],eax

  draw_tab_focus:
	cmp	dword [tab_keyboard.active],0
	je	draw_tabs_next
	cmp	r14d,[tab_keyboard.index]
	jne	draw_tabs_next
	cmp	dword [tab_keyboard.part],CSD_TAB_PART_BODY
	je	draw_tab_body_focus
	cmp	dword [tab_keyboard.part],CSD_TAB_PART_CLOSE
	jne	draw_tabs_next
	invoke	CopyRect,addr focus_rect,addr rdi+CSD_TAB_GEOMETRY.closeRect
	jmp	draw_tab_focus_rect

  draw_tab_body_focus:
	invoke	CopyRect,addr focus_rect,addr rdi+CSD_TAB_GEOMETRY.tabRect

  draw_tab_focus_rect:
	inc	dword [focus_rect.left]
	inc	dword [focus_rect.top]
	dec	dword [focus_rect.right]
	dec	dword [focus_rect.bottom]
	invoke	DrawFocusRect,rbx,addr focus_rect

  draw_tabs_next:
	add	rsi,sizeof.CSD_TAB_ITEM
	add	rdi,sizeof.CSD_TAB_GEOMETRY
	add	r12,sizeof.CSD_TAB_STATE
	inc	r14d
	dec	r15d
	jnz	draw_tabs_loop

  draw_tabs_done:
	invoke	SelectObject,rbx,[old_font]
	invoke	SetTextColor,rbx,[old_color]
	invoke	SetBkMode,rbx,[old_bk]
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
	fastcall DrawTabs,rbx

	invoke	SetBkMode,rbx,TRANSPARENT
	mov	[old_bk],eax
	invoke	SelectObject,rbx,[hTextFont]
	mov	[old_font],rax
	invoke	SetTextColor,rbx,dword [current_theme.bodyTextColor]
	mov	[old_color],eax

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
	menu_point	POINT
	title_h		dd ?
	menu_flags	dd ?
	menu_command	dd ?
    endl

	mov	[hwnd],rcx
	cmp	edx,-1
	jne	menu_from_lparam
	mov	eax,dword [caption_geometry+CSD_CAPTION_SYSTEM_INDEX*sizeof.CSD_CAPTION_GEOMETRY+CSD_CAPTION_GEOMETRY.rect.left]
	cmp	eax,dword [caption_geometry+CSD_CAPTION_SYSTEM_INDEX*sizeof.CSD_CAPTION_GEOMETRY+CSD_CAPTION_GEOMETRY.rect.right]
	jge	menu_from_window
	test	dword [layout_flags],CSD_LAYOUT_RTL
	jz	menu_keyboard_ltr
	mov	eax,dword [caption_geometry+CSD_CAPTION_SYSTEM_INDEX*sizeof.CSD_CAPTION_GEOMETRY+CSD_CAPTION_GEOMETRY.rect.right]
	jmp	menu_keyboard_x_ready

  menu_keyboard_ltr:
	mov	eax,dword [caption_geometry+CSD_CAPTION_SYSTEM_INDEX*sizeof.CSD_CAPTION_GEOMETRY+CSD_CAPTION_GEOMETRY.rect.left]

  menu_keyboard_x_ready:
	mov	[menu_point.x],eax
	mov	eax,dword [caption_geometry+CSD_CAPTION_SYSTEM_INDEX*sizeof.CSD_CAPTION_GEOMETRY+CSD_CAPTION_GEOMETRY.rect.bottom]
	mov	[menu_point.y],eax
	invoke	ClientToScreen,[hwnd],addr menu_point
	mov	eax,[menu_point.x]
	mov	[xpos],eax
	mov	eax,[menu_point.y]
	mov	[ypos],eax
	jmp	menu_position_ready

  menu_from_window:
	fastcall CsdDipToPx,CSD_TITLE_HEIGHT_DIP,dword [current_dpi]
	mov	[title_h],eax
	invoke	GetWindowRect,[hwnd],addr window_rect
	mov	eax,[window_rect.left]
	test	dword [layout_flags],CSD_LAYOUT_RTL
	jz	menu_window_x_ready
	mov	eax,[window_rect.right]

  menu_window_x_ready:
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
	fastcall RtlPopupMenuFlags,TPM_RETURNCMD or TPM_RIGHTBUTTON
	mov	[menu_flags],eax
	invoke	TrackPopupMenu,rbx,dword [menu_flags],dword [xpos],dword [ypos],0,[hwnd],0
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
	fastcall TabPartFromClientPoint,dword [client_x],dword [client_y]
	cmp	eax,CSD_TAB_NO_INDEX
	jne	hittest_client
	fastcall PointInCaptionDragRects,dword [client_x],dword [client_y]
	test	eax,eax
	jz	hittest_client
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

proc ToggleLayoutDirection hwnd
	mov	[hwnd],rcx
	xor	dword [layout_flags],CSD_LAYOUT_RTL
	invoke	ReleaseCapture
	fastcall CsdCaptionMouseLeave,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT
	fastcall ClearTabInput
	fastcall LayoutCaptionControls
	fastcall LayoutCaptionChildren
	fastcall UpdateThemeStatus
	invoke	InvalidateRect,[hwnd],0,1
	fastcall StartCaptionFadeTimer,[hwnd]
	mov	eax,1
	ret
endp

proc AddDemoTab
    locals
	index	dd ?
	titlep	dq ?
    endl

	mov	eax,[tabstrip.count]
	cmp	eax,CSD_TAB_CAPACITY
	jae	add_demo_tab_no
	mov	[index],eax
	fastcall TabTitleForIndex,dword [index]
	mov	[titlep],rax
	fastcall InitOneTab,dword [index],[titlep]
	mov	eax,[index]
	inc	dword [tabstrip.count]
	mov	[tabstrip.activeIndex],eax
	fastcall CsdTabStripClearInput,addr tabstrip
	fastcall ApplyTabStates
	mov	eax,1
	ret

  add_demo_tab_no:
	xor	eax,eax
	ret
endp

proc CloseDemoTab uses rsi rdi, index
    locals
	move_count dd ?
    endl

	mov	dword [index],ecx
	cmp	ecx,0
	jl	close_demo_tab_no
	cmp	ecx,[tabstrip.count]
	jge	close_demo_tab_no
	cmp	dword [tabstrip.count],1
	jle	close_demo_tab_no
	mov	eax,[tabstrip.count]
	dec	eax
	sub	eax,ecx
	mov	[move_count],eax
	jz	close_demo_tab_tail

	mov	eax,ecx
	imul	eax,sizeof.CSD_TAB_ITEM
	lea	rdi,[tab_items+eax]
	lea	rsi,[rdi+sizeof.CSD_TAB_ITEM]
	mov	ecx,[move_count]
	imul	ecx,sizeof.CSD_TAB_ITEM
	rep	movsb

	mov	eax,dword [index]
	imul	eax,sizeof.CSD_TAB_STATE
	lea	rdi,[tab_state+eax]
	lea	rsi,[rdi+sizeof.CSD_TAB_STATE]
	mov	ecx,[move_count]
	imul	ecx,sizeof.CSD_TAB_STATE
	rep	movsb

  close_demo_tab_tail:
	dec	dword [tabstrip.count]
	mov	eax,[tabstrip.activeIndex]
	cmp	eax,dword [index]
	jl	close_demo_tab_active_ready
	cmp	eax,0
	jle	close_demo_tab_active_zero
	dec	eax
	jmp	close_demo_tab_store_active

  close_demo_tab_active_zero:
	xor	eax,eax

  close_demo_tab_store_active:
	mov	[tabstrip.activeIndex],eax

  close_demo_tab_active_ready:
	fastcall CsdTabStripClearInput,addr tabstrip
	fastcall ApplyTabStates
	mov	eax,1
	ret

  close_demo_tab_no:
	xor	eax,eax
	ret
endp

proc ShowOverflowMenu uses rbx, hwnd,lparam_value
    locals
	pt	POINT
	menu_flags dd ?
	command dd ?
    endl

	mov	[hwnd],rcx
	mov	eax,edx
	movsx	ecx,ax
	sar	eax,16
	movsx	edx,ax
	mov	[pt.x],ecx
	mov	[pt.y],edx

	cmp	dword [caption_overflow_count],0
	jne	overflow_have_items
	mov	rax,status_overflow
	mov	[status_message],rax
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret

  overflow_have_items:
	invoke	CreatePopupMenu
	mov	rbx,rax
	mov	[hOverflowMenu],rax
	test	rax,rax
	jz	overflow_no

	cmp	dword [caption_visible+CSD_CAPTION_SEARCH_INDEX*4],0
	jne	overflow_skip_search
	invoke	AppendMenuW,rbx,MF_STRING,\
		dword [caption_responsive+CSD_CAPTION_SEARCH_INDEX*sizeof.CSD_CAPTION_RESPONSIVE+CSD_CAPTION_RESPONSIVE.overflowCommand],\
		addr overflow_search_text

  overflow_skip_search:
	cmp	dword [caption_visible+CSD_CAPTION_NEWTAB_INDEX*4],0
	jne	overflow_skip_newtab
	invoke	AppendMenuW,rbx,MF_STRING,\
		dword [caption_responsive+CSD_CAPTION_NEWTAB_INDEX*sizeof.CSD_CAPTION_RESPONSIVE+CSD_CAPTION_RESPONSIVE.overflowCommand],\
		addr overflow_newtab_text

  overflow_skip_newtab:
	cmp	dword [caption_visible+CSD_CAPTION_SETTINGS_INDEX*4],0
	jne	overflow_skip_settings
	invoke	AppendMenuW,rbx,MF_STRING,\
		dword [caption_responsive+CSD_CAPTION_SETTINGS_INDEX*sizeof.CSD_CAPTION_RESPONSIVE+CSD_CAPTION_RESPONSIVE.overflowCommand],\
		addr overflow_settings_text

  overflow_skip_settings:
	invoke	ClientToScreen,[hwnd],addr pt
	fastcall RtlPopupMenuFlags,TPM_RETURNCMD or TPM_RIGHTBUTTON or TPM_NONOTIFY
	mov	[menu_flags],eax
	invoke	TrackPopupMenu,rbx,dword [menu_flags],dword [pt.x],dword [pt.y],0,[hwnd],0
	mov	[command],eax
	invoke	DestroyMenu,rbx
	mov	qword [hOverflowMenu],0

	cmp	dword [command],ID_OVERFLOW_SEARCH
	je	overflow_search
	cmp	dword [command],ID_OVERFLOW_NEWTAB
	je	overflow_newtab
	cmp	dword [command],ID_OVERFLOW_SETTINGS
	je	overflow_settings
	mov	eax,1
	ret

  overflow_search:
	mov	rax,status_search_hidden
	mov	[status_message],rax
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret

  overflow_newtab:
	fastcall AddDemoTab
	test	eax,eax
	jz	overflow_no
	fastcall LayoutCaptionControls
	fastcall LayoutCaptionChildren
	fastcall UpdateThemeStatus
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret

  overflow_settings:
	fastcall CycleBackdropMode,[hwnd]
	mov	eax,1
	ret

  overflow_no:
	xor	eax,eax
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
	cmp	edx,ID_CAPTION_OVERFLOW
	je	caption_click_overflow
	cmp	edx,ID_CAPTION_SEARCH
	je	caption_click_search
	cmp	edx,ID_CAPTION_NEWTAB
	je	caption_click_newtab
	cmp	edx,ID_CAPTION_SETTINGS
	je	caption_click_settings
	jmp	caption_click_no

  caption_click_overflow:
	fastcall ShowOverflowMenu,[hwnd],dword [lparam_value]
	ret

  caption_click_search:
	mov	rax,status_edit
	mov	[status_message],rax
	jmp	caption_click_redraw

  caption_click_newtab:
	fastcall AddDemoTab
	test	eax,eax
	jz	caption_click_no
	fastcall LayoutCaptionControls
	fastcall LayoutCaptionChildren
	fastcall UpdateThemeStatus
	jmp	caption_click_redraw

  caption_click_settings:
	fastcall CycleBackdropMode,[hwnd]
	mov	eax,1
	ret

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
	changed dd ?
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
	mov	dword [changed],0
	fastcall CsdCaptionTrackMouseLeave,[hwnd],addr caption_input
	fastcall CsdCaptionSetHot,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [index]
	or	[changed],eax
	cmp	dword [index],CSD_CAPTION_NO_INDEX
	jne	mousemove_clear_tab
	fastcall TabPartFromClientPoint,dword [xpos],dword [ypos]
	fastcall SetTabHot,eax,edx
	jmp	mousemove_tab_ready

  mousemove_clear_tab:
	fastcall SetTabHot,CSD_TAB_NO_INDEX,CSD_TAB_PART_NONE

  mousemove_tab_ready:
	or	[changed],eax
	cmp	dword [changed],0
	jz	mousemove_done
	invoke	InvalidateRect,[hwnd],0,1
	fastcall StartCaptionFadeTimer,[hwnd]

  mousemove_done:
	ret
endp

proc HandleCaptionMouseLeave hwnd
    locals
	changed dd ?
    endl

	mov	[hwnd],rcx
	mov	dword [changed],0
	fastcall CsdCaptionMouseLeave,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT
	or	[changed],eax
	fastcall ClearTabInput
	or	[changed],eax
	cmp	dword [changed],0
	jz	mouseleave_done
	invoke	InvalidateRect,[hwnd],0,1
	fastcall StartCaptionFadeTimer,[hwnd]

  mouseleave_done:
	ret
endp

proc HandleCaptionLButtonDown hwnd,wparam,lparam
    locals
	xpos	dd ?
	ypos	dd ?
	index	dd ?
	tab_index dd ?
	tab_part dd ?
	changed dd ?
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
	je	lbuttondown_tab
	invoke	SetCapture,[hwnd]
	fastcall CsdCaptionTrackMouseLeave,[hwnd],addr caption_input
	fastcall ClearTabInput
	fastcall CsdCaptionSetHot,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [index]
	fastcall CsdCaptionSetPressed,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [index]
	invoke	InvalidateRect,[hwnd],0,1
	fastcall StartCaptionFadeTimer,[hwnd]

  lbuttondown_done:
	ret

  lbuttondown_tab:
	fastcall TabPartFromClientPoint,dword [xpos],dword [ypos]
	mov	[tab_index],eax
	mov	[tab_part],edx
	cmp	eax,CSD_TAB_NO_INDEX
	je	lbuttondown_done
	invoke	SetCapture,[hwnd]
	fastcall CsdCaptionTrackMouseLeave,[hwnd],addr caption_input
	mov	dword [changed],0
	fastcall CsdCaptionSetHot,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,CSD_CAPTION_NO_INDEX
	or	[changed],eax
	fastcall CsdCaptionSetPressed,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,CSD_CAPTION_NO_INDEX
	or	[changed],eax
	fastcall SetTabHot,dword [tab_index],dword [tab_part]
	or	[changed],eax
	fastcall SetTabPressed,dword [tab_index],dword [tab_part]
	or	[changed],eax
	invoke	InvalidateRect,[hwnd],0,1
	fastcall StartCaptionFadeTimer,[hwnd]
	ret
endp

proc HandleCaptionLButtonUp hwnd,wparam,lparam
    locals
	xpos		dd ?
	ypos		dd ?
	index		dd ?
	pressed_index	dd ?
	tab_index	dd ?
	tab_part	dd ?
	pressed_tab_index dd ?
	pressed_tab_part dd ?
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
	mov	eax,[tabstrip.pressedIndex]
	mov	[pressed_tab_index],eax
	mov	eax,[tabstrip.pressedPart]
	mov	[pressed_tab_part],eax
	mov	dword [changed],0
	invoke	ReleaseCapture
	fastcall CsdCaptionSetHot,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,dword [index]
	or	dword [changed],eax
	cmp	dword [index],CSD_CAPTION_NO_INDEX
	jne	lbuttonup_clear_tab_hot
	fastcall TabPartFromClientPoint,dword [xpos],dword [ypos]
	mov	[tab_index],eax
	mov	[tab_part],edx
	fastcall SetTabHot,dword [tab_index],dword [tab_part]
	jmp	lbuttonup_tab_hot_ready

  lbuttonup_clear_tab_hot:
	mov	dword [tab_index],CSD_TAB_NO_INDEX
	mov	dword [tab_part],CSD_TAB_PART_NONE
	fastcall SetTabHot,CSD_TAB_NO_INDEX,CSD_TAB_PART_NONE

  lbuttonup_tab_hot_ready:
	or	dword [changed],eax
	fastcall CsdCaptionSetPressed,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT,CSD_CAPTION_NO_INDEX
	or	dword [changed],eax
	fastcall SetTabPressed,CSD_TAB_NO_INDEX,CSD_TAB_PART_NONE
	or	dword [changed],eax
	cmp	dword [changed],0
	je	lbuttonup_check_command
	invoke	InvalidateRect,[hwnd],0,1
	fastcall StartCaptionFadeTimer,[hwnd]

  lbuttonup_check_command:
	cmp	dword [pressed_index],CSD_CAPTION_NO_INDEX
	je	lbuttonup_check_tab
	mov	eax,[pressed_index]
	cmp	eax,[index]
	jne	lbuttonup_check_tab
	fastcall HandleCaptionClientClick,[hwnd],dword [lparam_value]
	jmp	lbuttonup_done

  lbuttonup_check_tab:
	cmp	dword [pressed_tab_index],CSD_TAB_NO_INDEX
	je	lbuttonup_done
	mov	eax,[pressed_tab_index]
	cmp	eax,[tab_index]
	jne	lbuttonup_done
	mov	eax,[pressed_tab_part]
	cmp	eax,[tab_part]
	jne	lbuttonup_done
	cmp	eax,CSD_TAB_PART_BODY
	je	lbuttonup_select_tab
	cmp	eax,CSD_TAB_PART_CLOSE
	je	lbuttonup_close_tab
	jmp	lbuttonup_done

  lbuttonup_select_tab:
	fastcall SetActiveTab,dword [tab_index]
	test	eax,eax
	jz	lbuttonup_done
	mov	rax,status_select_tab
	mov	[status_message],rax
	fastcall UpdateThemeStatus
	invoke	InvalidateRect,[hwnd],0,1
	jmp	lbuttonup_done

  lbuttonup_close_tab:
	fastcall CloseDemoTab,dword [tab_index]
	test	eax,eax
	jz	lbuttonup_done
	fastcall LayoutCaptionControls
	fastcall LayoutCaptionChildren
	fastcall UpdateThemeStatus
	invoke	InvalidateRect,[hwnd],0,1

  lbuttonup_done:
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
	mov	rax,status_edit
	mov	[status_message],rax
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret

  command_no:
	xor	eax,eax
	ret
endp

proc HandleGetMinMaxInfo uses rbx, hwnd,minmaxp
    locals
	dpi_value dd ?
	width	  dd ?
	height	  dd ?
    endl

	mov	[hwnd],rcx
	mov	rbx,rdx
	mov	eax,[current_dpi]
	test	eax,eax
	jnz	minmax_have_dpi
	fastcall CsdGetWindowDpi,[hwnd]
	fastcall SetDpiState,eax
	mov	eax,[current_dpi]

  minmax_have_dpi:
	mov	[dpi_value],eax
	fastcall ComputeMinimumTrackClientWidth
	mov	[width],eax
	fastcall CsdDipToPx,CSD_MIN_BODY_H_DIP,dword [dpi_value]
	mov	[height],eax

	mov	dword [initial_rect.left],0
	mov	dword [initial_rect.top],0
	mov	eax,[width]
	mov	[initial_rect.right],eax
	mov	eax,[height]
	mov	[initial_rect.bottom],eax
	fastcall CsdAdjustWindowRectForDpi,addr initial_rect,\
		WS_OVERLAPPEDWINDOW or WS_CLIPCHILDREN,WS_EX_APPWINDOW,dword [dpi_value]
	mov	eax,[initial_rect.right]
	sub	eax,[initial_rect.left]
	mov	[rbx+MINMAXINFO.ptMinTrackSize.x],eax
	mov	eax,[initial_rect.bottom]
	sub	eax,[initial_rect.top]
	mov	[rbx+MINMAXINFO.ptMinTrackSize.y],eax
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
		WM_GETMINMAXINFO,		wnd_getminmaxinfo,\
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
		WM_GETOBJECT,			wnd_getobject,\
		WM_KEYDOWN,			wnd_keydown,\
		WM_SYSKEYDOWN,			wnd_syskeydown,\
		WM_KILLFOCUS,			wnd_killfocus,\
		WM_TIMER,			wnd_timer,\
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
	mov	eax,dword [wparam]
	and	eax,0FFFFh
	cmp	eax,WA_INACTIVE
	jne	wnd_activate_done
	fastcall ClearTabKeyboard,[hwnd]

  wnd_activate_done:
	xor	eax,eax
	ret

  wnd_nchittest:
	fastcall HitTestCsdFrame,[hwnd],r9
	ret

  wnd_getminmaxinfo:
	fastcall HandleGetMinMaxInfo,[hwnd],[lparam]
	xor	eax,eax
	ret

  wnd_create:
	mov	rax,[hwnd]
	mov	[hMain],rax
	invoke	DwmExtendFrameIntoClientArea,[hwnd],addr dwm_margins
	fastcall RefreshDpiState,[hwnd]
	fastcall CsdSnapProbePolicy,addr snap_policy
	fastcall CsdBackdropProbePolicy,addr backdrop_policy
	fastcall CsdCaptionInputInit,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT
	fastcall InitTabs
	fastcall InitTabKeyboard
	fastcall InitUiaProviders
	fastcall CreatePaintObjects
	fastcall LayoutCaptionControls
	fastcall CreateCaptionChildren,[hwnd]
	fastcall LayoutCaptionChildren
	fastcall UpdateThemeStatus
	fastcall CsdRefreshSystemMenuState,[hwnd]
	xor	eax,eax
	ret

  wnd_size:
	fastcall LayoutCaptionControls
	fastcall LayoutCaptionChildren
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

  wnd_command:
	fastcall HandleCommand,[hwnd],r8,r9
	xor	eax,eax
	ret

  wnd_ctlcoloredit:
	fastcall HandleCtlColorEdit,r8,r9
	ret

  wnd_getobject:
	fastcall HandleGetObject,[hwnd],[wparam],[lparam]
	test	rax,rax
	jz	wnd_default
	ret

  wnd_keydown:
	fastcall HandleTabKeyDown,[hwnd],r8,r9
	test	eax,eax
	jnz	wnd_keydown_done
	cmp	dword [wparam],VK_F2
	je	wnd_toggle_layout
	cmp	dword [wparam],VK_ESCAPE
	jne	wnd_default
	invoke	DestroyWindow,[hwnd]

  wnd_keydown_done:
	xor	eax,eax
	ret

  wnd_toggle_layout:
	fastcall ToggleLayoutDirection,[hwnd]
	xor	eax,eax
	ret

  wnd_syskeydown:
	fastcall HandleTabSysKeyDown,[hwnd],r8,r9
	test	eax,eax
	jz	wnd_default
	xor	eax,eax
	ret

  wnd_killfocus:
	fastcall ClearTabKeyboard,[hwnd]
	xor	eax,eax
	ret

  wnd_timer:
	cmp	dword [wparam],CSD_FADE_TIMER_ID
	jne	wnd_default
	fastcall HandleCaptionFadeTimer,[hwnd]
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
	fastcall CsdBackdropSetRequested,addr backdrop_policy,DWMSBT_NONE
	fastcall CsdBackdropApply,[hwnd],addr backdrop_policy
	fastcall StopCaptionFadeTimer,[hwnd]
	fastcall DisconnectUiaProvider
	fastcall DestroyCaptionChildren
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
		WS_OVERLAPPEDWINDOW or WS_CLIPCHILDREN,\
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
