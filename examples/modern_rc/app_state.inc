APP_REGISTRY_ROOT equ 'Software\Fasm2Examples\ModernRcMruEdit'

define __GLOBAL_DATA__ app_state_data
define __GLOBAL_BSS__ app_state_bss

macro app_state_data
	app_name		du 'Modern RC MRU Edit',0
	class_name		du 'Fasm2ModernRcMruEdit',0
	untitled_name		du '(untitled)',0
	dirty_suffix		du ' *',0
	title_format		du '%ls%ls - Modern RC MRU Edit',0
	recent_empty_label	du '(empty)',0
	file_filter		du 'Text files (*.txt)',0,'*.txt',0,'All files (*.*)',0,'*.*',0,0
	default_ext		du 'txt',0

	align 8
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
purge app_state_data
end macro

macro app_state_bss
	align 8
	hInstance	dq ?
	hMain		dq ?
	hEdit		dq ?
	hMenubar	dq ?
	hAccel		dq ?
	hFileMenu	dq ?
	hRecentMenu	dq ?
	hEditMenu	dq ?
	hHelpMenu	dq ?
	hFindDlg	dq ?
	msg		MSG
	client_rect	RECT
	current_path	rw MAX_PATH
	dialog_path	rw MAX_PATH
	title_buffer	rw MAX_PATH + 64
	menu_label	rw MAX_PATH + 24
	bytes_done	dd ?
	file_high	dd ?
	dirty		dd ?
	suppress_change dd ?
purge app_state_bss
end macro
