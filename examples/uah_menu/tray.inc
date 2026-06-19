proc SetupTrayIcon hwnd
	mov	[hwnd],rcx
	invoke	RtlZeroMemory,tray_icon,sizeof.NOTIFYICONDATAW
	mov	[tray_icon.cbSize],sizeof.NOTIFYICONDATAW
	mov	rax,[hwnd]
	mov	[tray_icon.hWnd],rax
	mov	[tray_icon.uID],1
	mov	[tray_icon.uFlags],NIF_MESSAGE or NIF_ICON or NIF_TIP
	mov	[tray_icon.uCallbackMessage],WM_TRAY_CALLBACK
	invoke	LoadIconW,0,IDI_APPLICATION
	mov	[tray_icon.hIcon],rax
	invoke	lstrcpynW,addr tray_icon.szTip,'UAH Menu RichEdit',128
	invoke	Shell_NotifyIcon,NIM_ADD,tray_icon
	mov	[tray_icon.uVersion],NOTIFYICON_VERSION_4
	invoke	Shell_NotifyIcon,NIM_SETVERSION,tray_icon
	ret
endp

proc CleanupTrayIcon
	cmp	qword [tray_icon.hWnd],0
	je	.done
	invoke	Shell_NotifyIcon,NIM_DELETE,tray_icon
	mov	qword [tray_icon.hWnd],0
  .done:
	ret
endp

proc HandleTrayCallback hwnd,wparam,lparam
    locals
	pt POINT
    endl
	mov	[hwnd],rcx
	mov	[wparam],rdx
	mov	[lparam],r8
	; NOTIFYICON_VERSION_4 packs the notification code in LOWORD(lParam).
	movzx	eax,word [lparam]
	cmp	eax,WM_RBUTTONUP
	je	.menu
	cmp	eax,WM_CONTEXTMENU
	je	.menu
	cmp	eax,NIN_SELECT
	je	.menu
	cmp	eax,WM_LBUTTONDBLCLK
	je	.show
	xor	eax,eax
	ret
  .show:
	invoke	ShowWindow,[hwnd],SW_RESTORE
	invoke	SetForegroundWindow,[hwnd]
	mov	eax,1
	ret
  .menu:
	invoke	GetCursorPos,addr pt
	fastcall ShowTrayMenu,dword [pt.x],dword [pt.y]
	mov	eax,1
	ret
endp
