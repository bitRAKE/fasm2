; 10_a11y_keyboard.asm - keyboard and high-contrast caption access.
;
; This starts from 09's live EDIT caption slot and discharges the keyboard,
; high-contrast, and UI Automation provider parts of the accessibility debt.

ADDON_WINDOWS_RESOURCE equ '10_a11y_keyboard.res'

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
CSD_STATE_BUTTON_HOVER	= 00DCEED7h
CSD_STATE_BUTTON_PRESSED = 00BBD8B4h
CSD_STATE_BUTTON_INACTIVE = 006A706Ah
CSD_STATE_CLOSE_HOVER	= 003232E8h
CSD_STATE_CLOSE_PRESSED = 001818B8h
CSD_STATE_TEXT_INACTIVE = 00AEB6AEh
HCF_HIGHCONTRASTON	= 00000001h
WM_GETOBJECT		= 003Dh
UiaRootObjectId		= -25
UiaAppendRuntimeId	= 3

S_OK			= 00000000h
E_NOTIMPL		= 080004001h
E_NOINTERFACE		= 080004002h
E_POINTER		= 080004003h
E_FAIL			= 080004005h
E_OUTOFMEMORY		= 08007000Eh

ProviderOptions_ServerSideProvider = 00000002h
NavigateDirection_Parent	= 0
NavigateDirection_NextSibling	= 1
NavigateDirection_PreviousSibling = 2
NavigateDirection_FirstChild	= 3
NavigateDirection_LastChild	= 4

UIA_InvokePatternId			= 10000
UIA_BoundingRectanglePropertyId		= 30001
UIA_ControlTypePropertyId		= 30003
UIA_NamePropertyId			= 30005
UIA_HasKeyboardFocusPropertyId		= 30008
UIA_IsKeyboardFocusablePropertyId	= 30009
UIA_IsEnabledPropertyId		= 30010
UIA_AutomationIdPropertyId		= 30011
UIA_ButtonControlTypeId		= 50000

VT_EMPTY		= 0
VT_I4			= 3
VT_BSTR			= 8
VT_BOOL		= 11
VARIANT_TRUE		= -1

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
CSD_KEYBOARD_ORDER_COUNT = 5

struct HIGHCONTRASTW
  cbSize		dd ?
  dwFlags		dd ?
  lpszDefaultScheme	dq ?
ends

struct CSD_CAPTION_KEYBOARD
  active	dd ?
  focusIndex	dd ?
  orderIndex	dd ?
ends

struct CSD_VARIANT
  vt		dw ?
  reserved1	dw ?
  reserved2	dw ?
  reserved3	dw ?
  value	dq ?
  extra		dq ?
ends

struct UIA_RECT
  left		dq ?
  top		dq ?
  width	dq ?
  height	dq ?
ends

struct CSD_UIA_PROVIDER
  simpleVtbl	dq ?
  simpleOwner	dq ?
  fragmentVtbl	dq ?
  fragmentOwner dq ?
  rootVtbl	dq ?
  rootOwner	dq ?
  invokeVtbl	dq ?
  invokeOwner	dq ?
  refCount	dd ?
  index		dd ?
  orderIndex	dd ?
  reserved	dd ?
ends

CSD_UIA_INTERFACE_OWNER = 8
CSD_UIA_SIMPLE_OFFSET	= CSD_UIA_PROVIDER.simpleVtbl
CSD_UIA_FRAGMENT_OFFSET = CSD_UIA_PROVIDER.fragmentVtbl
CSD_UIA_ROOT_OFFSET	= CSD_UIA_PROVIDER.rootVtbl
CSD_UIA_INVOKE_OFFSET	= CSD_UIA_PROVIDER.invokeVtbl

define __GLOBAL_DATA__ csd_data
define __GLOBAL_BSS__ csd_bss

macro csd_data
	app_name	du 'CSD 10 - keyboard caption access',0
	class_name	du 'Fasm2CsdA11yKeyboard',0
	title_text	du '10  Keyboard and high-contrast caption access',0
	body_text	du 'This version keeps the 09 caption EDIT child and adds keyboard and UI Automation access for the owner-drawn caption buttons.',13,10,13,10
			du 'F10 or Alt enters the caption, Left/Right move focus across System menu, Settings, Minimize, Maximize/Restore, and Close, Enter/Space invokes the focused row, Alt+Space opens the real system menu, and Escape exits caption mode.',13,10,13,10
			du 'High contrast switches caption colors to system colors and keeps a visible focus rectangle. Inspect/Narrator can discover the owner-drawn caption buttons through the local UIA provider.',0
	status_format	du 'Caption edit HWND %p | snap %s | keyboard %s | high contrast %s',0
	status_edit	du 'Type in the caption edit. The child HWND owns focus, text input, and IME.',0
	status_settings	du 'Settings caption button clicked.',0
	uia_name_root	du 'CSD 10 - keyboard caption access',0
	uia_name_system du 'System menu',0
	uia_name_settings du 'Settings',0
	uia_name_minimize du 'Minimize',0
	uia_name_maximize du 'Maximize',0
	uia_name_restore du 'Restore',0
	uia_name_close	du 'Close',0
	uia_id_root	du 'caption.root',0
	uia_id_system	du 'caption.system',0
	uia_id_settings du 'caption.settings',0
	uia_id_minimize du 'caption.minimize',0
	uia_id_maximize du 'caption.maximize',0
	uia_id_close	du 'caption.close',0
	status_keyboard_on du 'on',0
	status_keyboard_off du 'off',0
	status_high_on	du 'on',0
	status_high_off	du 'off',0
	theme_dark	du 'dark',0
	theme_light	du 'light',0
	snap_on		du 'HTMAXBUTTON',0
	snap_off	du 'HTCLIENT',0
	keyboard_order	dd 0,2,5,4,3

	align 8
	IID_IUnknown			dd 000000000h
					dw 00000h,00000h
					db 0C0h,000h,000h,000h,000h,000h,000h,046h
	IID_IRawElementProviderSimple	dd 0D6DD68D1h
					dw 086FDh,04332h
					db 086h,066h,09Ah,0BEh,0DEh,0A2h,0D2h,04Ch
	IID_IRawElementProviderFragmentRoot dd 0620CE2A5h
					dw 0AB8Fh,040A9h
					db 086h,0CBh,0DEh,03Ch,075h,059h,09Bh,058h
	IID_IRawElementProviderFragment	dd 0F7063DA8h
					dw 08359h,0439Ch
					db 092h,097h,0BBh,0C5h,029h,09Ah,07Dh,087h
	IID_IInvokeProvider		dd 054FCB24Bh
					dw 0E18Eh,047A2h
					db 0B4h,0D3h,0ECh,0CBh,0E7h,075h,099h,0A2h

	uia_simple_vtbl dq UiaProvider_QueryInterface,UiaProvider_AddRef,\
		UiaProvider_Release,UiaSimple_ProviderOptions,\
		UiaSimple_GetPatternProvider,UiaSimple_GetPropertyValue,\
		UiaSimple_HostRawElementProvider
	uia_fragment_vtbl dq UiaProvider_QueryInterface,UiaProvider_AddRef,\
		UiaProvider_Release,UiaFragment_Navigate,\
		UiaFragment_GetRuntimeId,UiaFragment_BoundingRectangle,\
		UiaFragment_EmbeddedFragmentRoots,UiaFragment_SetFocus,\
		UiaFragment_FragmentRoot
	uia_root_vtbl dq UiaProvider_QueryInterface,UiaProvider_AddRef,\
		UiaProvider_Release,UiaRoot_ElementProviderFromPoint,\
		UiaRoot_GetFocus
	uia_invoke_vtbl dq UiaProvider_QueryInterface,UiaProvider_AddRef,\
		UiaProvider_Release,UiaInvoke_Invoke

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
	high_contrast	HIGHCONTRASTW
	current_dpi	dd ?
	resize_border_px dd ?
	high_contrast_active dd ?
	button_hover_color dd ?
	button_pressed_color dd ?
	button_inactive_color dd ?
	close_hover_color dd ?
	close_pressed_color dd ?
	text_inactive_color dd ?
	current_theme	CSD_THEME
	caption_input	CSD_CAPTION_INPUT
	caption_keyboard CSD_CAPTION_KEYBOARD
	snap_policy	CSD_SNAP_POLICY
	hSearchEdit	dq ?
	hUiFont		dq ?
	hUiaCore	dq ?
	hOleAut		dq ?
	pUiaReturnRawElementProvider dq ?
	pUiaHostProviderFromHwnd dq ?
	pUiaDisconnectProvider dq ?
	pSysAllocString dq ?
	pSafeArrayCreateVector dq ?
	pSafeArrayPutElement dq ?
	pSafeArrayDestroy dq ?
	status_buffer	rw 128
	uia_ready	dd ?
	uia_initialized dd ?
	align 8
	uia_root	rb sizeof.CSD_UIA_PROVIDER
	uia_children	rb sizeof.CSD_UIA_PROVIDER * CSD_KEYBOARD_ORDER_COUNT
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
		hButtonHoverBrush,	dword [button_hover_color],\
		hButtonPressedBrush,	dword [button_pressed_color],\
		hButtonInactiveBrush,	dword [button_inactive_color],\
		hCloseBrush,		dword [current_theme.closeColor],\
		hCloseHoverBrush,	dword [close_hover_color],\
		hClosePressedBrush,	dword [close_pressed_color],\
		hEdgeBrush,		dword [current_theme.borderColor]

		invoke	CreateSolidBrush,color
		mov	[handle],rax
	end iterate
	ret
endp

proc RefreshHighContrast
	mov	dword [high_contrast.cbSize],sizeof.HIGHCONTRASTW
	mov	dword [high_contrast.dwFlags],0
	mov	qword [high_contrast.lpszDefaultScheme],0
	mov	dword [high_contrast_active],0
	invoke	SystemParametersInfoW,SPI_GETHIGHCONTRAST,sizeof.HIGHCONTRASTW,\
		addr high_contrast,0
	test	eax,eax
	jz	high_contrast_colors_normal
	test	dword [high_contrast.dwFlags],HCF_HIGHCONTRASTON
	jz	high_contrast_colors_normal

	mov	dword [high_contrast_active],1
	invoke	GetSysColor,COLOR_WINDOW
	mov	[current_theme.bodyBackColor],eax
	invoke	GetSysColor,COLOR_WINDOWTEXT
	mov	[current_theme.bodyTextColor],eax
	mov	[current_theme.statusTextColor],eax
	invoke	GetSysColor,COLOR_HIGHLIGHT
	mov	[current_theme.captionColor],eax
	mov	[current_theme.buttonColor],eax
	mov	[button_hover_color],eax
	mov	[button_pressed_color],eax
	mov	[current_theme.closeColor],eax
	mov	[close_hover_color],eax
	mov	[close_pressed_color],eax
	invoke	GetSysColor,COLOR_HIGHLIGHTTEXT
	mov	[current_theme.titleTextColor],eax
	mov	[text_inactive_color],eax
	invoke	GetSysColor,COLOR_BTNFACE
	mov	[button_inactive_color],eax
	invoke	GetSysColor,COLOR_WINDOWFRAME
	mov	[current_theme.borderColor],eax
	ret

  high_contrast_colors_normal:
	mov	dword [button_hover_color],CSD_STATE_BUTTON_HOVER
	mov	dword [button_pressed_color],CSD_STATE_BUTTON_PRESSED
	mov	dword [button_inactive_color],CSD_STATE_BUTTON_INACTIVE
	mov	dword [close_hover_color],CSD_STATE_CLOSE_HOVER
	mov	dword [close_pressed_color],CSD_STATE_CLOSE_PRESSED
	mov	dword [text_inactive_color],CSD_STATE_TEXT_INACTIVE
	ret
endp

proc UpdateThemeStatus uses rbx
    locals
	keyboard_state	dq ?
	contrast_state	dq ?
    endl

	mov	rbx,snap_off
	cmp	dword [snap_policy.snapCapable],0
	je	status_snap_ready
	mov	rbx,snap_on
  status_snap_ready:
	mov	rax,status_keyboard_off
	cmp	dword [caption_keyboard.active],0
	je	status_keyboard_ready
	mov	rax,status_keyboard_on
  status_keyboard_ready:
	mov	[keyboard_state],rax

	mov	rax,status_high_off
	cmp	dword [high_contrast_active],0
	je	status_contrast_ready
	mov	rax,status_high_on
  status_contrast_ready:
	mov	[contrast_state],rax

	invoke	wsprintfW,addr status_buffer,status_format,\
		[hSearchEdit],rbx,[keyboard_state],[contrast_state]
	mov	rax,status_buffer
	mov	[status_message],rax
	ret
endp

proc RefreshThemeAndFrame hwnd
	mov	[hwnd],rcx
	fastcall CsdThemeRefresh,addr current_theme
	fastcall CsdThemeApplyDwm,[hwnd],addr current_theme
	fastcall RefreshHighContrast
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
	invoke	GetStockObject,DEFAULT_GUI_FONT
	mov	[hUiFont],rax
	invoke	SendMessageW,[hSearchEdit],WM_SETFONT,[hUiFont],1
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

proc InitCaptionKeyboard
	mov	dword [caption_keyboard.active],0
	mov	dword [caption_keyboard.focusIndex],CSD_CAPTION_NO_INDEX
	mov	dword [caption_keyboard.orderIndex],0
	ret
endp

proc LoadUiaApis
	cmp	dword [uia_ready],0
	jne	load_uia_done

	invoke	LoadLibraryW,'UIAutomationCore.dll'
	mov	[hUiaCore],rax
	test	rax,rax
	jz	load_uia_done
	invoke	GetProcAddress,[hUiaCore],<A,'UiaReturnRawElementProvider'>
	mov	[pUiaReturnRawElementProvider],rax
	invoke	GetProcAddress,[hUiaCore],<A,'UiaHostProviderFromHwnd'>
	mov	[pUiaHostProviderFromHwnd],rax
	invoke	GetProcAddress,[hUiaCore],<A,'UiaDisconnectProvider'>
	mov	[pUiaDisconnectProvider],rax

	invoke	LoadLibraryW,'oleaut32.dll'
	mov	[hOleAut],rax
	test	rax,rax
	jz	load_uia_done
	invoke	GetProcAddress,[hOleAut],<A,'SysAllocString'>
	mov	[pSysAllocString],rax
	invoke	GetProcAddress,[hOleAut],<A,'SafeArrayCreateVector'>
	mov	[pSafeArrayCreateVector],rax
	invoke	GetProcAddress,[hOleAut],<A,'SafeArrayPutElement'>
	mov	[pSafeArrayPutElement],rax
	invoke	GetProcAddress,[hOleAut],<A,'SafeArrayDestroy'>
	mov	[pSafeArrayDestroy],rax

	iterate pointer, pUiaReturnRawElementProvider,pUiaHostProviderFromHwnd,\
		pUiaDisconnectProvider,pSysAllocString,pSafeArrayCreateVector,\
		pSafeArrayPutElement,pSafeArrayDestroy
		cmp	qword [pointer],0
		je	load_uia_done
	end iterate
	mov	dword [uia_ready],1

  load_uia_done:
	mov	eax,[uia_ready]
	ret
endp

proc InitOneUiaProvider providerp,index,orderIndex
	lea	rax,[uia_simple_vtbl]
	mov	[rcx+CSD_UIA_PROVIDER.simpleVtbl],rax
	mov	[rcx+CSD_UIA_PROVIDER.simpleOwner],rcx
	lea	rax,[uia_fragment_vtbl]
	mov	[rcx+CSD_UIA_PROVIDER.fragmentVtbl],rax
	mov	[rcx+CSD_UIA_PROVIDER.fragmentOwner],rcx
	lea	rax,[uia_root_vtbl]
	mov	[rcx+CSD_UIA_PROVIDER.rootVtbl],rax
	mov	[rcx+CSD_UIA_PROVIDER.rootOwner],rcx
	lea	rax,[uia_invoke_vtbl]
	mov	[rcx+CSD_UIA_PROVIDER.invokeVtbl],rax
	mov	[rcx+CSD_UIA_PROVIDER.invokeOwner],rcx
	mov	dword [rcx+CSD_UIA_PROVIDER.refCount],1
	mov	[rcx+CSD_UIA_PROVIDER.index],edx
	mov	[rcx+CSD_UIA_PROVIDER.orderIndex],r8d
	mov	dword [rcx+CSD_UIA_PROVIDER.reserved],0
	ret
endp

proc InitUiaProviders uses rbx rsi
	fastcall LoadUiaApis
	test	eax,eax
	jz	init_uia_done
	fastcall InitOneUiaProvider,uia_root,CSD_CAPTION_NO_INDEX,\
		CSD_CAPTION_NO_INDEX

	xor	esi,esi
	mov	rbx,uia_children
  init_uia_loop:
	mov	eax,[keyboard_order+rsi*4]
	fastcall InitOneUiaProvider,rbx,eax,esi
	add	rbx,sizeof.CSD_UIA_PROVIDER
	inc	esi
	cmp	esi,CSD_KEYBOARD_ORDER_COUNT
	jb	init_uia_loop
	mov	dword [uia_initialized],1

  init_uia_done:
	ret
endp

proc DisconnectUiaProvider
	cmp	dword [uia_initialized],0
	je	disconnect_uia_done
	cmp	qword [pUiaDisconnectProvider],0
	je	disconnect_uia_mark
	fastcall [pUiaDisconnectProvider],uia_root+CSD_UIA_SIMPLE_OFFSET

  disconnect_uia_mark:
	mov	dword [uia_initialized],0

  disconnect_uia_done:
	ret
endp

proc UiaGuidEquals guidp,knownp
	mov	rax,[rcx]
	cmp	rax,[rdx]
	jne	guid_not_equal
	mov	rax,[rcx+8]
	cmp	rax,[rdx+8]
	jne	guid_not_equal
	mov	eax,1
	ret

  guid_not_equal:
	xor	eax,eax
	ret
endp

proc UiaProviderFromOrder orderIndex
	cmp	ecx,CSD_KEYBOARD_ORDER_COUNT
	jae	provider_from_order_no
	imul	ecx,sizeof.CSD_UIA_PROVIDER
	lea	rax,[uia_children+rcx]
	ret

  provider_from_order_no:
	xor	eax,eax
	ret
endp

proc UiaProviderFromIndex index
	xor	eax,eax

  provider_from_index_loop:
	cmp	ecx,[keyboard_order+rax*4]
	je	provider_from_index_found
	inc	eax
	cmp	eax,CSD_KEYBOARD_ORDER_COUNT
	jb	provider_from_index_loop
	xor	eax,eax
	ret

  provider_from_index_found:
	imul	eax,sizeof.CSD_UIA_PROVIDER
	lea	rax,[uia_children+rax]
	ret
endp

proc UiaReturnFragmentInterface providerp,outp
	test	rdx,rdx
	jz	return_fragment_no
	mov	qword [rdx],0
	test	rcx,rcx
	jz	return_fragment_no
	lea	rax,[rcx+CSD_UIA_FRAGMENT_OFFSET]
	mov	[rdx],rax
	inc	dword [rcx+CSD_UIA_PROVIDER.refCount]
	mov	eax,1
	ret

  return_fragment_no:
	xor	eax,eax
	ret
endp

proc UiaVariantEmpty variantp
	mov	qword [rcx],0
	mov	qword [rcx+8],0
	mov	qword [rcx+16],0
	ret
endp

proc UiaVariantI4 variantp,value
	fastcall UiaVariantEmpty,rcx
	mov	word [rcx+CSD_VARIANT.vt],VT_I4
	mov	dword [rcx+CSD_VARIANT.value],edx
	xor	eax,eax
	ret
endp

proc UiaVariantBool variantp,value
	fastcall UiaVariantEmpty,rcx
	mov	word [rcx+CSD_VARIANT.vt],VT_BOOL
	xor	eax,eax
	test	edx,edx
	jz	variant_bool_store
	mov	ax,VARIANT_TRUE

  variant_bool_store:
	mov	word [rcx+CSD_VARIANT.value],ax
	xor	eax,eax
	ret
endp

proc UiaVariantBstr variantp,stringp
	mov	[variantp],rcx
	fastcall UiaVariantEmpty,rcx
	mov	word [rcx+CSD_VARIANT.vt],VT_BSTR
	fastcall [pSysAllocString],rdx
	test	rax,rax
	jz	variant_bstr_oom
	mov	rcx,[variantp]
	mov	[rcx+CSD_VARIANT.value],rax
	xor	eax,eax
	ret

  variant_bstr_oom:
	mov	eax,E_OUTOFMEMORY
	ret
endp

proc UiaCaptionNameForProvider providerp
	mov	eax,[rcx+CSD_UIA_PROVIDER.index]
	cmp	eax,CSD_CAPTION_NO_INDEX
	je	caption_name_root
	cmp	eax,0
	je	caption_name_system
	cmp	eax,2
	je	caption_name_settings
	cmp	eax,5
	je	caption_name_minimize
	cmp	eax,4
	je	caption_name_maximize
	cmp	eax,3
	je	caption_name_close
	xor	eax,eax
	ret

  caption_name_root:
	mov	rax,uia_name_root
	ret
  caption_name_system:
	mov	rax,uia_name_system
	ret
  caption_name_settings:
	mov	rax,uia_name_settings
	ret
  caption_name_minimize:
	mov	rax,uia_name_minimize
	ret
  caption_name_maximize:
	invoke	IsZoomed,[hMain]
	test	eax,eax
	jz	caption_name_maximize_ready
	mov	rax,uia_name_restore
	ret
  caption_name_maximize_ready:
	mov	rax,uia_name_maximize
	ret
  caption_name_close:
	mov	rax,uia_name_close
	ret
endp

proc UiaCaptionAutomationIdForProvider providerp
	mov	eax,[rcx+CSD_UIA_PROVIDER.index]
	cmp	eax,CSD_CAPTION_NO_INDEX
	je	caption_id_root
	cmp	eax,0
	je	caption_id_system
	cmp	eax,2
	je	caption_id_settings
	cmp	eax,5
	je	caption_id_minimize
	cmp	eax,4
	je	caption_id_maximize
	cmp	eax,3
	je	caption_id_close
	xor	eax,eax
	ret

  caption_id_root:
	mov	rax,uia_id_root
	ret
  caption_id_system:
	mov	rax,uia_id_system
	ret
  caption_id_settings:
	mov	rax,uia_id_settings
	ret
  caption_id_minimize:
	mov	rax,uia_id_minimize
	ret
  caption_id_maximize:
	mov	rax,uia_id_maximize
	ret
  caption_id_close:
	mov	rax,uia_id_close
	ret
endp

proc UiaProvider_QueryInterface uses rbx rsi rdi, thisp,riidp,ppvp
	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	rsi,rdx
	mov	rdi,r8
	test	rdi,rdi
	jz	query_interface_bad_pointer
	mov	qword [rdi],0

	fastcall UiaGuidEquals,rsi,IID_IUnknown
	test	eax,eax
	jnz	query_interface_simple
	fastcall UiaGuidEquals,rsi,IID_IRawElementProviderSimple
	test	eax,eax
	jnz	query_interface_simple
	fastcall UiaGuidEquals,rsi,IID_IRawElementProviderFragment
	test	eax,eax
	jnz	query_interface_fragment
	fastcall UiaGuidEquals,rsi,IID_IRawElementProviderFragmentRoot
	test	eax,eax
	jnz	query_interface_root
	fastcall UiaGuidEquals,rsi,IID_IInvokeProvider
	test	eax,eax
	jnz	query_interface_invoke
	mov	eax,E_NOINTERFACE
	ret

  query_interface_simple:
	lea	rax,[rbx+CSD_UIA_SIMPLE_OFFSET]
	jmp	query_interface_return

  query_interface_fragment:
	lea	rax,[rbx+CSD_UIA_FRAGMENT_OFFSET]
	jmp	query_interface_return

  query_interface_root:
	cmp	dword [rbx+CSD_UIA_PROVIDER.index],CSD_CAPTION_NO_INDEX
	jne	query_interface_no_interface
	lea	rax,[rbx+CSD_UIA_ROOT_OFFSET]
	jmp	query_interface_return

  query_interface_invoke:
	cmp	dword [rbx+CSD_UIA_PROVIDER.index],CSD_CAPTION_NO_INDEX
	je	query_interface_no_interface
	lea	rax,[rbx+CSD_UIA_INVOKE_OFFSET]

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

proc UiaProvider_AddRef thisp
	mov	rax,[rcx+CSD_UIA_INTERFACE_OWNER]
	inc	dword [rax+CSD_UIA_PROVIDER.refCount]
	mov	eax,[rax+CSD_UIA_PROVIDER.refCount]
	ret
endp

proc UiaProvider_Release thisp
	mov	rcx,[rcx+CSD_UIA_INTERFACE_OWNER]
	cmp	dword [rcx+CSD_UIA_PROVIDER.refCount],1
	jbe	provider_release_done
	dec	dword [rcx+CSD_UIA_PROVIDER.refCount]

  provider_release_done:
	mov	eax,[rcx+CSD_UIA_PROVIDER.refCount]
	ret
endp

proc UiaSimple_ProviderOptions thisp,pRetVal
	test	rdx,rdx
	jz	simple_options_bad_pointer
	mov	dword [rdx],ProviderOptions_ServerSideProvider
	xor	eax,eax
	ret

  simple_options_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaSimple_GetPatternProvider thisp,patternId,pRetVal
	mov	rax,[rcx+CSD_UIA_INTERFACE_OWNER]
	test	r8,r8
	jz	simple_pattern_bad_pointer
	mov	qword [r8],0
	cmp	edx,UIA_InvokePatternId
	jne	simple_pattern_done
	cmp	dword [rax+CSD_UIA_PROVIDER.index],CSD_CAPTION_NO_INDEX
	je	simple_pattern_done
	lea	rdx,[rax+CSD_UIA_INVOKE_OFFSET]
	mov	[r8],rdx
	inc	dword [rax+CSD_UIA_PROVIDER.refCount]

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
	fastcall UiaVariantEmpty,rdi

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
	xor	eax,eax
	ret

  simple_property_name:
	fastcall UiaCaptionNameForProvider,rbx
	test	rax,rax
	jz	simple_property_done_empty
	fastcall UiaVariantBstr,rdi,rax
	ret

  simple_property_automation_id:
	fastcall UiaCaptionAutomationIdForProvider,rbx
	test	rax,rax
	jz	simple_property_done_empty
	fastcall UiaVariantBstr,rdi,rax
	ret

  simple_property_control_type:
	cmp	dword [rbx+CSD_UIA_PROVIDER.index],CSD_CAPTION_NO_INDEX
	je	simple_property_done_empty
	fastcall UiaVariantI4,rdi,UIA_ButtonControlTypeId
	ret

  simple_property_enabled:
	mov	edx,1
	fastcall UiaVariantBool,rdi,edx
	ret

  simple_property_focusable:
	xor	edx,edx
	cmp	dword [rbx+CSD_UIA_PROVIDER.index],CSD_CAPTION_NO_INDEX
	setne	dl
	fastcall UiaVariantBool,rdi,edx
	ret

  simple_property_has_focus:
	xor	edx,edx
	cmp	dword [caption_keyboard.active],0
	je	simple_property_has_focus_ready
	mov	eax,[rbx+CSD_UIA_PROVIDER.index]
	cmp	eax,[caption_keyboard.focusIndex]
	sete	dl
  simple_property_has_focus_ready:
	fastcall UiaVariantBool,rdi,edx
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
	cmp	dword [rax+CSD_UIA_PROVIDER.index],CSD_CAPTION_NO_INDEX
	jne	simple_host_done
	fastcall [pUiaHostProviderFromHwnd],[hMain],rdx
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
	cmp	eax,CSD_CAPTION_NO_INDEX
	je	root_point_done
	fastcall UiaProviderFromIndex,eax
	test	rax,rax
	jz	root_point_done
	mov	rbx,rax
	fastcall UiaReturnFragmentInterface,rbx,[outp]

  root_point_done:
	xor	eax,eax
	ret

  root_point_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaRoot_GetFocus thisp,pRetVal
	test	rdx,rdx
	jz	root_focus_bad_pointer
	mov	qword [rdx],0
	cmp	dword [caption_keyboard.active],0
	je	root_focus_done
	mov	ecx,[caption_keyboard.focusIndex]
	cmp	ecx,CSD_CAPTION_NO_INDEX
	je	root_focus_done
	fastcall UiaProviderFromIndex,ecx
	test	rax,rax
	jz	root_focus_done
	fastcall UiaReturnFragmentInterface,rax,rdx

  root_focus_done:
	xor	eax,eax
	ret

  root_focus_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaFragment_Navigate uses rbx rsi, thisp,direction,pRetVal
	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	esi,edx
	test	r8,r8
	jz	fragment_navigate_bad_pointer
	mov	qword [r8],0

	cmp	dword [rbx+CSD_UIA_PROVIDER.index],CSD_CAPTION_NO_INDEX
	jne	fragment_navigate_child
	cmp	esi,NavigateDirection_FirstChild
	je	fragment_navigate_root_first
	cmp	esi,NavigateDirection_LastChild
	je	fragment_navigate_root_last
	jmp	fragment_navigate_done

  fragment_navigate_root_first:
	fastcall UiaProviderFromOrder,0
	fastcall UiaReturnFragmentInterface,rax,r8
	jmp	fragment_navigate_done

  fragment_navigate_root_last:
	fastcall UiaProviderFromOrder,CSD_KEYBOARD_ORDER_COUNT - 1
	fastcall UiaReturnFragmentInterface,rax,r8
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
	fastcall UiaReturnFragmentInterface,uia_root,r8
	jmp	fragment_navigate_done

  fragment_navigate_next:
	mov	ecx,[rbx+CSD_UIA_PROVIDER.orderIndex]
	inc	ecx
	fastcall UiaProviderFromOrder,ecx
	fastcall UiaReturnFragmentInterface,rax,r8
	jmp	fragment_navigate_done

  fragment_navigate_previous:
	mov	ecx,[rbx+CSD_UIA_PROVIDER.orderIndex]
	test	ecx,ecx
	jz	fragment_navigate_done
	dec	ecx
	fastcall UiaProviderFromOrder,ecx
	fastcall UiaReturnFragmentInterface,rax,r8

  fragment_navigate_done:
	xor	eax,eax
	ret

  fragment_navigate_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc UiaFragment_GetRuntimeId uses rbx, thisp,pRetVal
    locals
	arrayp	dq ?
	outp	dq ?
	index	dd ?
	value	dd ?
    endl

	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	[outp],rdx
	test	rdx,rdx
	jz	fragment_runtime_bad_pointer
	mov	qword [rdx],0
	fastcall [pSafeArrayCreateVector],VT_I4,0,3
	test	rax,rax
	jz	fragment_runtime_oom
	mov	[arrayp],rax

	mov	dword [index],0
	mov	dword [value],UiaAppendRuntimeId
	fastcall [pSafeArrayPutElement],[arrayp],addr index,addr value
	test	eax,eax
	jnz	fragment_runtime_fail
	mov	dword [index],1
	mov	eax,dword [hMain]
	mov	[value],eax
	fastcall [pSafeArrayPutElement],[arrayp],addr index,addr value
	test	eax,eax
	jnz	fragment_runtime_fail
	mov	dword [index],2
	mov	eax,[rbx+CSD_UIA_PROVIDER.index]
	inc	eax
	mov	[value],eax
	fastcall [pSafeArrayPutElement],[arrayp],addr index,addr value
	test	eax,eax
	jnz	fragment_runtime_fail

	mov	rax,[arrayp]
	mov	rdx,[outp]
	mov	[rdx],rax
	xor	eax,eax
	ret

  fragment_runtime_fail:
	fastcall [pSafeArrayDestroy],[arrayp]
	mov	eax,E_FAIL
	ret

  fragment_runtime_oom:
	mov	eax,E_OUTOFMEMORY
	ret

  fragment_runtime_bad_pointer:
	mov	eax,E_POINTER
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
	cmp	dword [rbx+CSD_UIA_PROVIDER.index],CSD_CAPTION_NO_INDEX
	je	fragment_bounds_root

	mov	eax,[rbx+CSD_UIA_PROVIDER.index]
	imul	eax,sizeof.CSD_CAPTION_GEOMETRY
	lea	rsi,[caption_geometry+rax+CSD_CAPTION_GEOMETRY.rect]
	mov	eax,[rsi+RECT.left]
	mov	[pt0.x],eax
	mov	eax,[rsi+RECT.top]
	mov	[pt0.y],eax
	mov	eax,[rsi+RECT.right]
	mov	[pt1.x],eax
	mov	eax,[rsi+RECT.bottom]
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
	mov	rbx,[rcx+CSD_UIA_INTERFACE_OWNER]
	cmp	dword [rbx+CSD_UIA_PROVIDER.index],CSD_CAPTION_NO_INDEX
	je	fragment_set_focus_done
	fastcall EnterCaptionKeyboard,[hMain]
	fastcall ApplyCaptionKeyboardOrder,dword [rbx+CSD_UIA_PROVIDER.orderIndex]
	fastcall UpdateThemeStatus
	invoke	InvalidateRect,[hMain],0,1

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

proc UiaInvoke_Invoke thisp
	mov	rcx,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	edx,[rcx+CSD_UIA_PROVIDER.index]
	fastcall InvokeCaptionIndex,[hMain],edx
	test	eax,eax
	jz	uia_invoke_fail
	xor	eax,eax
	ret

  uia_invoke_fail:
	mov	eax,E_FAIL
	ret
endp

proc HandleGetObject hwnd,wparam,lparam
    locals
	providerp	dq ?
    endl

	mov	[hwnd],rcx
	cmp	r8d,UiaRootObjectId
	jne	getobject_no
	cmp	dword [uia_initialized],0
	je	getobject_no
	lea	rax,[uia_root+CSD_UIA_SIMPLE_OFFSET]
	mov	[providerp],rax
	fastcall [pUiaReturnRawElementProvider],[hwnd],rdx,r8,[providerp]
	ret

  getobject_no:
	xor	eax,eax
	ret
endp

proc ApplyCaptionKeyboardOrder orderIndex
	mov	[caption_keyboard.orderIndex],ecx
	movsxd	rax,ecx
	mov	eax,[keyboard_order+rax*4]
	mov	[caption_keyboard.focusIndex],eax
	ret
endp

proc EnterCaptionKeyboard hwnd
	mov	[hwnd],rcx
	mov	dword [caption_keyboard.active],1
	fastcall ApplyCaptionKeyboardOrder,0
	invoke	SetFocus,[hwnd]
	invoke	ReleaseCapture
	fastcall UpdateThemeStatus
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret
endp

proc ClearCaptionKeyboard hwnd
	mov	[hwnd],rcx
	cmp	dword [caption_keyboard.active],0
	je	clear_keyboard_no
	mov	dword [caption_keyboard.active],0
	mov	dword [caption_keyboard.focusIndex],CSD_CAPTION_NO_INDEX
	mov	dword [caption_keyboard.orderIndex],0
	fastcall UpdateThemeStatus
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret

  clear_keyboard_no:
	xor	eax,eax
	ret
endp

proc MoveCaptionKeyboardFocus hwnd,delta
    locals
	next_order dd ?
    endl

	mov	[hwnd],rcx
	mov	dword [delta],edx
	cmp	dword [caption_keyboard.active],0
	jne	move_keyboard_active
	fastcall EnterCaptionKeyboard,[hwnd]
	ret

  move_keyboard_active:
	mov	eax,[caption_keyboard.orderIndex]
	add	eax,dword [delta]
	cmp	eax,0
	jge	move_keyboard_check_end
	mov	eax,CSD_KEYBOARD_ORDER_COUNT - 1
	jmp	move_keyboard_apply

  move_keyboard_check_end:
	cmp	eax,CSD_KEYBOARD_ORDER_COUNT
	jl	move_keyboard_apply
	xor	eax,eax

  move_keyboard_apply:
	mov	[next_order],eax
	fastcall ApplyCaptionKeyboardOrder,dword [next_order]
	fastcall UpdateThemeStatus
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
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
	focus_rect	RECT
	button_brush	dq ?
	glyph_color	dd ?
	state_value	dd ?
	control_index	dd ?
    endl

	mov	rbx,rcx
	mov	rsi,caption_descriptors
	mov	rdi,caption_geometry
	mov	r15,caption_state
	mov	r14d,CSD_CAPTION_CONTROL_COUNT
	mov	dword [control_index],0

  draw_loop:
	lea	r12,[rdi+CSD_CAPTION_GEOMETRY.rect]
	test	[rsi+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_CHILD
	jnz	draw_next
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
	mov	eax,[text_inactive_color]
	mov	[glyph_color],eax
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
	mov	eax,[text_inactive_color]
	mov	[glyph_color],eax

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
	cmp	dword [caption_keyboard.active],0
	je	draw_next
	mov	eax,[control_index]
	cmp	eax,[caption_keyboard.focusIndex]
	jne	draw_next
	invoke	CopyRect,addr focus_rect,r12
	invoke	InflateRect,addr focus_rect,-4,-4
	invoke	DrawFocusRect,rbx,addr focus_rect

  draw_next:
	add	rsi,sizeof.CSD_CAPTION_DESCRIPTOR
	add	rdi,sizeof.CSD_CAPTION_GEOMETRY
	add	r15,sizeof.CSD_CAPTION_STATE
	inc	dword [control_index]
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
	mov	rax,status_edit
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

proc InvokeCaptionIndex uses rbx, hwnd,index
    locals
	sys_command dd ?
    endl

	mov	[hwnd],rcx
	mov	eax,edx
	cmp	eax,CSD_CAPTION_NO_INDEX
	je	caption_invoke_no
	cmp	eax,CSD_CAPTION_CONTROL_COUNT
	jae	caption_invoke_no
	imul	eax,sizeof.CSD_CAPTION_DESCRIPTOR
	lea	rbx,[caption_descriptors+rax]

	test	[rbx+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_SYSTEM
	jz	caption_invoke_not_system
	fastcall ShowSystemMenu,[hwnd],-1
	mov	eax,1
	ret

  caption_invoke_not_system:
	test	[rbx+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_SYSCMD
	jz	caption_invoke_not_syscmd
	mov	eax,[rbx+CSD_CAPTION_DESCRIPTOR.command]
	mov	[sys_command],eax
	cmp	[rbx+CSD_CAPTION_DESCRIPTOR.id],ID_CAPTION_MAX
	jne	caption_invoke_dispatch_syscmd
	invoke	IsZoomed,[hwnd]
	test	eax,eax
	jz	caption_invoke_dispatch_syscmd
	mov	dword [sys_command],SC_RESTORE

  caption_invoke_dispatch_syscmd:
	fastcall CsdDispatchSystemCommand,[hwnd],dword [sys_command]
	mov	eax,1
	ret

  caption_invoke_not_syscmd:
	test	[rbx+CSD_CAPTION_DESCRIPTOR.flags],CSD_CAPTION_APP
	jz	caption_invoke_no
	cmp	[rbx+CSD_CAPTION_DESCRIPTOR.command],ID_CAPTION_SETTINGS
	jne	caption_invoke_no
	mov	rax,status_settings
	mov	[status_message],rax
	invoke	InvalidateRect,[hwnd],0,1
	mov	eax,1
	ret

  caption_invoke_no:
	xor	eax,eax
	ret
endp

proc InvokeCaptionKeyboardFocus hwnd
	mov	[hwnd],rcx
	cmp	dword [caption_keyboard.active],0
	je	keyboard_invoke_no
	mov	edx,[caption_keyboard.focusIndex]
	fastcall InvokeCaptionIndex,[hwnd],edx
	ret

  keyboard_invoke_no:
	xor	eax,eax
	ret
endp

proc HandleCaptionKeyDown hwnd,wparam,lparam
	mov	[hwnd],rcx
	cmp	edx,VK_F10
	je	keydown_enter_caption
	cmp	edx,VK_LEFT
	je	keydown_left
	cmp	edx,VK_RIGHT
	je	keydown_right
	cmp	edx,VK_RETURN
	je	keydown_invoke
	cmp	edx,VK_SPACE
	je	keydown_invoke
	cmp	edx,VK_ESCAPE
	je	keydown_escape
	xor	eax,eax
	ret

  keydown_enter_caption:
	fastcall EnterCaptionKeyboard,[hwnd]
	ret

  keydown_left:
	cmp	dword [caption_keyboard.active],0
	je	keydown_no
	fastcall MoveCaptionKeyboardFocus,[hwnd],-1
	ret

  keydown_right:
	cmp	dword [caption_keyboard.active],0
	je	keydown_no
	fastcall MoveCaptionKeyboardFocus,[hwnd],1
	ret

  keydown_invoke:
	fastcall InvokeCaptionKeyboardFocus,[hwnd]
	ret

  keydown_escape:
	fastcall ClearCaptionKeyboard,[hwnd]
	ret

  keydown_no:
	xor	eax,eax
	ret
endp

proc HandleCaptionSysKeyDown hwnd,wparam,lparam
	mov	[hwnd],rcx
	cmp	edx,VK_SPACE
	je	syskeydown_system_menu
	cmp	edx,VK_MENU
	je	syskeydown_enter_caption
	cmp	edx,VK_F10
	je	syskeydown_enter_caption
	xor	eax,eax
	ret

  syskeydown_system_menu:
	fastcall ShowSystemMenu,[hwnd],-1
	mov	eax,1
	ret

  syskeydown_enter_caption:
	fastcall EnterCaptionKeyboard,[hwnd]
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
		WM_GETOBJECT,			wnd_getobject,\
		WM_SETFOCUS,			wnd_setfocus,\
		WM_KILLFOCUS,			wnd_killfocus,\
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
		WM_SYSKEYDOWN,			wnd_syskeydown,\
		WM_KEYDOWN,			wnd_keydown,\
		WM_ERASEBKGND,			wnd_erasebkgnd,\
		WM_PAINT,			wnd_paint,\
		WM_NCDESTROY,			wnd_ncdestroy,\
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
	movzx	eax,r8w
	cmp	eax,WA_INACTIVE
	jne	wnd_activate_done
	fastcall ClearCaptionKeyboard,[hwnd]
  wnd_activate_done:
	xor	eax,eax
	ret

  wnd_getobject:
	fastcall HandleGetObject,[hwnd],r8,r9
	test	rax,rax
	jz	wnd_default
	ret

  wnd_setfocus:
	xor	eax,eax
	ret

  wnd_killfocus:
	fastcall ClearCaptionKeyboard,[hwnd]
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
	fastcall CreateCaptionChildren,[hwnd]
	fastcall LayoutCaptionChildren
	fastcall CsdCaptionInputInit,addr caption_input,caption_state,\
		CSD_CAPTION_CONTROL_COUNT
	fastcall InitCaptionKeyboard
	fastcall InitUiaProviders
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

  wnd_syskeydown:
	fastcall HandleCaptionSysKeyDown,[hwnd],r8,r9
	test	eax,eax
	jz	wnd_default
	xor	eax,eax
	ret

  wnd_keydown:
	fastcall HandleCaptionKeyDown,[hwnd],r8,r9
	test	eax,eax
	jnz	wnd_keydown_handled
	cmp	r8d,VK_ESCAPE
	jne	wnd_default
	invoke	DestroyWindow,[hwnd]
  wnd_keydown_handled:
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

  wnd_ncdestroy:
	fastcall DisconnectUiaProvider
	jmp	wnd_default

  wnd_destroy:
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
