define __GLOBAL_DATA__ app_state_data
define __GLOBAL_BSS__ app_state_bss

macro app_state_data
	; WNDCLASSEX needs a stable class-name pointer at registration time.
	class_name	du 'Fasm2UahMenuRichEdit',0

	align 8
	wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_DBLCLKS or CS_HREDRAW or CS_VREDRAW,\
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
purge app_state_data
end macro

macro app_state_bss
	align 8
	hInstance	dq ?
	hMain		dq ?
	hRich		dq ?
	hMsftEdit	dq ?
	hMenubar	dq ?
	hAccel		dq ?
	hMenuFontText	dq ?
	hMenuFontSymbol dq ?
	hMenuFontGlyph	dq ?
	menu_font_dpi	dd ?
	dark_mode	dd ?
	msgTaskbar	dd ?
	msg		MSG
	client_rect	RECT
	tray_icon	NOTIFYICONDATAW
purge app_state_bss
end macro
