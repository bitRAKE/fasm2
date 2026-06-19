; uah_menu.inc - CBT discovery hook for transient UAH popup menu windows.
;
; Host requirements:
;   include after windows.inc and addon/uah.inc
;   expand uah_menu_bss, or provide equivalent storage
;   call UahOpen before installing when private uxtheme wrappers are desired
;   call UahMenuHook_Install with a popup-window subclass proc
;   call UahMenuHook_Uninstall during shutdown
;
; This helper intentionally owns only the discovery step. It filters CBT
; creation events through UahIsMenuWindow and attaches the caller's subclass to
; the system #32768 popup window. The caller owns WM_UAH* drawing, measuring,
; and subclass removal on WM_NCDESTROY.

macro uah_menu_bss
	align 8
	uah_menu_hook		dq ?
	uah_menu_owner		dq ?
	uah_menu_subclass_proc	dq ?
	uah_menu_subclass_id	dq ?
	uah_menu_dark		dd ?
				dd ?
purge uah_menu_bss
end macro
define __GLOBAL_BSS__ uah_menu_bss

UAH_MENU_DEFAULT_SUBCLASS_ID = 20260619

proc UahMenuHook_Install owner,subclass_proc,subclass_id,dark
	mov	[uah_menu_owner],rcx
	mov	[uah_menu_subclass_proc],rdx
	mov	[uah_menu_subclass_id],r8
	mov	[uah_menu_dark],r9d

	cmp	qword [uah_menu_hook],0
	jne	.installed
	cmp	qword [uah_menu_subclass_proc],0
	je	.fail

	invoke	GetCurrentThreadId
	invoke	SetWindowsHookEx,WH_CBT,addr UahMenuHook_CbtProc,0,eax
	mov	[uah_menu_hook],rax
	test	rax,rax
	jz	.fail
  .installed:
	mov	eax,1
	ret
  .fail:
	xor	eax,eax
	ret
endp

proc UahMenuHook_Uninstall
	mov	rcx,[uah_menu_hook]
	jrcxz	.clear
	invoke	UnhookWindowsHookEx,rcx
  .clear:
	mov	qword [uah_menu_hook],0
	mov	qword [uah_menu_owner],0
	mov	qword [uah_menu_subclass_proc],0
	mov	qword [uah_menu_subclass_id],0
	mov	dword [uah_menu_dark],0
	ret
endp

proc UahMenuHook_SetDark dark
	mov	dword [uah_menu_dark],ecx
	ret
endp

proc UahMenuHook_CbtProc code,wparam,lparam
	mov	dword [code],ecx
	mov	[wparam],rdx
	mov	[lparam],r8

	cmp	ecx,HCBT_CREATEWND
	jne	.next
	cmp	qword [uah_menu_subclass_proc],0
	je	.next
	fastcall UahIsMenuWindow,[wparam]
	test	eax,eax
	jz	.next
	invoke	SetWindowSubclass,[wparam],[uah_menu_subclass_proc],\
		[uah_menu_subclass_id],[uah_menu_owner]
	fastcall UahAllowDarkModeForWindow,[wparam],dword [uah_menu_dark]

  .next:
	invoke	CallNextHookEx,[uah_menu_hook],dword [code],[wparam],[lparam]
	ret
endp
