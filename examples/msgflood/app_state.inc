; app_state.inc - process storage and data owned by the executable.

class_name GLOBWSTR 'Fasm2MsgFloodEditLog',0

define __GLOBAL_DATA__ app_state_data
define __GLOBAL_BSS__ app_state_bss

macro app_state_data
	guid_acdc:
		dd 05D3E9A59h
		dw 0E9D5h,04B00h
		db 0A6h,0BDh,0FFh,034h,0FFh,051h,065h,048h
	guid_display:
		dd 06FE69556h
		dw 0704Ah,047A0h
		db 08Fh,024h,0C2h,08Dh,093h,06Fh,0DAh,047h
	guid_tray:
		dd 07324FDD1h
		dw 05E53h,047B7h
		db 09Bh,05Dh,069h,01Ch,0A8h,061h,0D3h,02Eh

	align 8
	wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
		style: CS_DBLCLKS or CS_HREDRAW or CS_VREDRAW,\
		lpfnWndProc: WindowProc,\
		hbrBackground: COLOR_WINDOW + 1,\
		lpszClassName: class_name
purge app_state_data
end macro

macro app_state_bss
	align 8
	hInstance	dq ?
	hMain		dq ?
	hEdit		dq ?
	hMenubar	dq ?
	hFileMenu	dq ?
	hLogMenu	dq ?
	hFilterMenu	dq ?
	oldEditProc	dq ?
	msg		MSG
	client_rect	RECT
purge app_state_bss
end macro
