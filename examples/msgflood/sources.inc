; sources.inc - opt the main HWND into as many message sources as practical.

define __GLOBAL_BSS__ sources_bss

macro sources_bss
	align 8
	msgShell	dd ?
	msgTaskbar	dd ?
	dd ?
	hDeviceNotify	dq ?
	hPowerAcDc	dq ?
	hPowerDisplay	dq ?
	hWtsapi		dq ?
	pSetProcessDpiAwarenessContext dq ?
	pChangeWindowMessageFilterEx dq ?
	pAddClipboardFormatListener dq ?
	pRemoveClipboardFormatListener dq ?
	pWTSRegisterSessionNotification dq ?
	pWTSUnRegisterSessionNotification dq ?
	raw_mouse	RAWINPUTDEVICE
	raw_keyboard	RAWINPUTDEVICE
	raw_size	dd ?
	dd ?
	raw_buffer	rb 256
	dev_broadcast	rb 32
	align 8
	tray_icon	NOTIFYICONDATAW
purge sources_bss
end macro

proc LoadOptionalApis
    locals
	hUser32 dq ?
    endl

	invoke	GetModuleHandle,'user32.dll'
	mov	[hUser32],rax
	test	rax,rax
	jz	.wts

	invoke	GetProcAddress,[hUser32],<A,'SetProcessDpiAwarenessContext'>
	mov	[pSetProcessDpiAwarenessContext],rax
	invoke	GetProcAddress,[hUser32],<A,'ChangeWindowMessageFilterEx'>
	mov	[pChangeWindowMessageFilterEx],rax
	invoke	GetProcAddress,[hUser32],<A,'AddClipboardFormatListener'>
	mov	[pAddClipboardFormatListener],rax
	invoke	GetProcAddress,[hUser32],<A,'RemoveClipboardFormatListener'>
	mov	[pRemoveClipboardFormatListener],rax

  .wts:
	invoke	LoadLibrary,'wtsapi32.dll'
	mov	[hWtsapi],rax
	test	rax,rax
	jz	.done
	invoke	GetProcAddress,[hWtsapi],<A,'WTSRegisterSessionNotification'>
	mov	[pWTSRegisterSessionNotification],rax
	invoke	GetProcAddress,[hWtsapi],<A,'WTSUnRegisterSessionNotification'>
	mov	[pWTSUnRegisterSessionNotification],rax
  .done:
	ret
endp

proc AllowFilteredMessage hwnd,msgid
	cmp	qword [pChangeWindowMessageFilterEx],0
	je	.done
	invoke	pChangeWindowMessageFilterEx,rcx,edx,MSGFLT_ALLOW,0
  .done:
	ret
endp

proc AllowUipiMessages hwnd
	mov	[hwnd],rcx
	fastcall AllowFilteredMessage,[hwnd],WM_DROPFILES
	fastcall AllowFilteredMessage,[hwnd],WM_COPYDATA
	fastcall AllowFilteredMessage,[hwnd],WM_COPYGLOBALDATA
	cmp	dword [msgShell],0
	je	.taskbar
	fastcall AllowFilteredMessage,[hwnd],dword [msgShell]
  .taskbar:
	cmp	dword [msgTaskbar],0
	je	.done
	fastcall AllowFilteredMessage,[hwnd],dword [msgTaskbar]
  .done:
	ret
endp

proc SetupRawInput hwnd
	mov	[raw_mouse.usUsagePage],1
	mov	[raw_mouse.usUsage],2
	mov	[raw_mouse.dwFlags],RIDEV_INPUTSINK or RIDEV_DEVNOTIFY
	mov	[raw_mouse.hwndTarget],rcx

	mov	[raw_keyboard.usUsagePage],1
	mov	[raw_keyboard.usUsage],6
	mov	[raw_keyboard.dwFlags],RIDEV_INPUTSINK or RIDEV_DEVNOTIFY
	mov	[raw_keyboard.hwndTarget],rcx

	invoke	RegisterRawInputDevices,raw_mouse,2,sizeof.RAWINPUTDEVICE
	ret
endp

proc SetupDeviceBroadcast hwnd
	mov	[hwnd],rcx
	invoke	RtlZeroMemory,dev_broadcast,32
	mov	dword [dev_broadcast],32
	mov	dword [dev_broadcast+4],DBT_DEVTYP_DEVICEINTERFACE
	invoke	RegisterDeviceNotification,[hwnd],dev_broadcast,\
		DEVICE_NOTIFY_WINDOW_HANDLE or DEVICE_NOTIFY_ALL_INTERFACE_CLASSES
	mov	[hDeviceNotify],rax
	ret
endp

proc SetupPowerBroadcasts hwnd
	mov	[hwnd],rcx
	invoke	RegisterPowerSettingNotification,[hwnd],guid_acdc,DEVICE_NOTIFY_WINDOW_HANDLE
	mov	[hPowerAcDc],rax
	invoke	RegisterPowerSettingNotification,[hwnd],guid_display,DEVICE_NOTIFY_WINDOW_HANDLE
	mov	[hPowerDisplay],rax
	ret
endp

proc SetupTrayIcon hwnd
	mov	[hwnd],rcx
	invoke	RtlZeroMemory,tray_icon,sizeof.NOTIFYICONDATAW
	mov	[tray_icon.cbSize],sizeof.NOTIFYICONDATAW
	mov	rax,[hwnd]
	mov	[tray_icon.hWnd],rax
	mov	[tray_icon.uID],1
	mov	[tray_icon.uFlags],NIF_GUID or NIF_MESSAGE or NIF_ICON or NIF_TIP
	mov	[tray_icon.uCallbackMessage],WM_TRAY
	mov	rax,[guid_tray]
	mov	qword [tray_icon.guidItem],rax
	mov	rax,[guid_tray+8]
	mov	qword [tray_icon.guidItem+8],rax
	invoke	LoadIconW,0,IDI_APPLICATION
	mov	[tray_icon.hIcon],rax
	invoke	lstrcpynW,addr tray_icon.szTip,'msgflood',128
	invoke	Shell_NotifyIcon,NIM_ADD,tray_icon
	mov	[tray_icon.uVersion],NOTIFYICON_VERSION_4
	invoke	Shell_NotifyIcon,NIM_SETVERSION,tray_icon
	ret
endp

proc WireSources hwnd
	mov	[hwnd],rcx
	fastcall SetupRawInput,[hwnd]

	invoke	RegisterWindowMessage,'SHELLHOOK'
	mov	dword [msgShell],eax
	invoke	RegisterWindowMessage,'TaskbarCreated'
	mov	dword [msgTaskbar],eax
	invoke	RegisterShellHookWindow,[hwnd]

	cmp	qword [pAddClipboardFormatListener],0
	je	.device
	invoke	pAddClipboardFormatListener,[hwnd]

  .device:
	fastcall SetupDeviceBroadcast,[hwnd]
	fastcall SetupPowerBroadcasts,[hwnd]

	cmp	qword [pWTSRegisterSessionNotification],0
	je	.hotkey
	invoke	pWTSRegisterSessionNotification,[hwnd],NOTIFY_FOR_THIS_SESSION

  .hotkey:
	invoke	RegisterHotKey,[hwnd],ID_HOTKEY,MOD_CONTROL or MOD_ALT or MOD_NOREPEAT,'M'
	invoke	SetTimer,[hwnd],ID_TIMER,TIMER_INTERVAL_MS,0
	fastcall SetupTrayIcon,[hwnd]
	fastcall AllowUipiMessages,[hwnd]
	ret
endp

proc CleanupSources hwnd
	mov	[hwnd],rcx

	invoke	KillTimer,[hwnd],ID_TIMER
	invoke	UnregisterHotKey,[hwnd],ID_HOTKEY
	invoke	Shell_NotifyIcon,NIM_DELETE,tray_icon

	cmp	qword [hDeviceNotify],0
	je	.power_ac
	invoke	UnregisterDeviceNotification,[hDeviceNotify]
	mov	qword [hDeviceNotify],0

  .power_ac:
	cmp	qword [hPowerAcDc],0
	je	.power_display
	invoke	UnregisterPowerSettingNotification,[hPowerAcDc]
	mov	qword [hPowerAcDc],0

  .power_display:
	cmp	qword [hPowerDisplay],0
	je	.clipboard
	invoke	UnregisterPowerSettingNotification,[hPowerDisplay]
	mov	qword [hPowerDisplay],0

  .clipboard:
	cmp	qword [pRemoveClipboardFormatListener],0
	je	.wts
	invoke	pRemoveClipboardFormatListener,[hwnd]

  .wts:
	cmp	qword [pWTSUnRegisterSessionNotification],0
	je	.free_wts
	invoke	pWTSUnRegisterSessionNotification,[hwnd]

  .free_wts:
	cmp	qword [hWtsapi],0
	je	.done
	invoke	FreeLibrary,[hWtsapi]
	mov	qword [hWtsapi],0
  .done:
	ret
endp

proc RearmMouseTracking hwnd
    locals
	tme TRACKMOUSEEVENT
    endl
	mov	[tme.cbSize],sizeof.TRACKMOUSEEVENT
	mov	[tme.dwFlags],TME_LEAVE or TME_HOVER
	mov	[tme.hwndTrack],rcx
	mov	[tme.dwHoverTime],50
	invoke	TrackMouseEvent,addr tme
	ret
endp

proc DrainRawInput hraw
	mov	dword [raw_size],256
	invoke	GetRawInputData,rcx,RID_INPUT,raw_buffer,raw_size,sizeof.RAWINPUTHEADER
	ret
endp
