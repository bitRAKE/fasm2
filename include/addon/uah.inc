; uah.inc - UserApiHook support for code that consumes WM_UAH* menu messages.
;
; Host requirements:
;   include after a Win64 Windows include with proc/endp, invoke, and structs
;   expand uah_bss, or provide equivalent storage for uah_runtime
;   call UahOpen during startup when private uxtheme wrappers are desired
;   call UahClose during shutdown
;
; The WM_UAH* messages and uxtheme ordinals are undocumented. Keep all usage
; tolerant: a CBT hook, creation hook, or subclass can use the message seam
; without the private exports, and the wrappers below are no-ops when an export
; is unavailable.

macro uah_bss
	align 8
	uah_runtime UAH_RUNTIME
purge uah_bss
end macro
define __GLOBAL_BSS__ uah_bss

WM_UAHDESTROYWINDOW	= 0090h
WM_UAHDRAWMENU		= 0091h
WM_UAHDRAWMENUITEM	= 0092h ; lParam is UAHDRAWMENUITEM.
WM_UAHINITMENU		= 0093h
WM_UAHMEASUREMENUITEM	= 0094h ; lParam is UAHMEASUREMENUITEM.
WM_UAHNCPAINTMENUPOPUP	= 0095h
WM_UAHUPDATE		= 0096h

UAH_CLASS_CHARS		= 16
UAH_LOAD_LIBRARY_SEARCH_SYSTEM32 = LOAD_LIBRARY_SEARCH_SYSTEM32

UAH_ORD_REFRESH_IMMERSIVE_COLORS	= 104
UAH_ORD_ALLOW_DARK_MODE_FOR_WINDOW	= 133
UAH_ORD_SET_PREFERRED_APP_MODE		= 135
UAH_ORD_FLUSH_MENU_THEMES		= 136

UAH_APPMODE_DEFAULT	= 0
UAH_APPMODE_ALLOWDARK	= 1
UAH_APPMODE_FORCEDARK	= 2
UAH_APPMODE_FORCELIGHT	= 3

struct UAHMENUITEMMETRICS
	; C union:
	;   rgsizeBar[2] and rgsizePopup[4].
	; The popup form is the larger storage; both views start at offset zero.
	rgsize		rb 4 * 8 ; SIZE[4], each { DWORD cx, cy }.
ends
UAHMENUITEMMETRICS.rgsizeBar	= UAHMENUITEMMETRICS.rgsize
UAHMENUITEMMETRICS.rgsizePopup	= UAHMENUITEMMETRICS.rgsize

struct UAHMENUPOPUPMETRICS
	rgcx		rd 4
	fUpdateMaxWidths dd ?
ends

struct UAHMENU
	hmenu		dq ?
	hdc		dq ?
	dwFlags		dd ?
			dd ?
ends

struct UAHMENUITEM
	iPosition	dd ?
	umim		UAHMENUITEMMETRICS
	umpm		UAHMENUPOPUPMETRICS
ends

struct UAHDRAWMENUITEM
	dis		DRAWITEMSTRUCT
	um		UAHMENU
	umi		UAHMENUITEM
ends

struct UAHMEASUREMENUITEM
	mis		MEASUREITEMSTRUCT
	um		UAHMENU
	umi		UAHMENUITEM
ends

struct UAH_RUNTIME
	Module		dq ?
	SetPreferredAppMode dq ?
	FlushMenuThemes dq ?
	AllowDarkModeForWindow dq ?
	RefreshImmersiveColorPolicyState dq ?
ends

proc UahRuntimeOpen runtimep
    locals
	module dq ?
    endl
	mov	[runtimep],rcx
	invoke	RtlZeroMemory,[runtimep],sizeof.UAH_RUNTIME

	invoke	LoadLibraryExW,'uxtheme.dll',0,LOAD_LIBRARY_SEARCH_SYSTEM32
	mov	[module],rax
	test	rax,rax
	jz	.fail

	mov	rcx,[runtimep]
	mov	[rcx+UAH_RUNTIME.Module],rax

	invoke	GetProcAddress,[module],UAH_ORD_SET_PREFERRED_APP_MODE
	mov	rcx,[runtimep]
	mov	[rcx+UAH_RUNTIME.SetPreferredAppMode],rax

	invoke	GetProcAddress,[module],UAH_ORD_FLUSH_MENU_THEMES
	mov	rcx,[runtimep]
	mov	[rcx+UAH_RUNTIME.FlushMenuThemes],rax

	invoke	GetProcAddress,[module],UAH_ORD_ALLOW_DARK_MODE_FOR_WINDOW
	mov	rcx,[runtimep]
	mov	[rcx+UAH_RUNTIME.AllowDarkModeForWindow],rax

	invoke	GetProcAddress,[module],UAH_ORD_REFRESH_IMMERSIVE_COLORS
	mov	rcx,[runtimep]
	mov	[rcx+UAH_RUNTIME.RefreshImmersiveColorPolicyState],rax

	mov	eax,1
	ret
  .fail:
	xor	eax,eax
	ret
endp

proc UahRuntimeClose runtimep
	mov	[runtimep],rcx
	mov	rax,[rcx+UAH_RUNTIME.Module]
	test	rax,rax
	jz	.clear
	invoke	FreeLibrary,rax
  .clear:
	invoke	RtlZeroMemory,[runtimep],sizeof.UAH_RUNTIME
	ret
endp

proc UahRuntimeSetPreferredAppMode runtimep,mode
	mov	[runtimep],rcx
	mov	dword [mode],edx
	mov	rax,[rcx+UAH_RUNTIME.SetPreferredAppMode]
	test	rax,rax
	jz	.missing
	fastcall rax,dword [mode]
	ret
  .missing:
	xor	eax,eax
	ret
endp

proc UahRuntimeFlushMenuThemes runtimep
	mov	rax,[rcx+UAH_RUNTIME.FlushMenuThemes]
	test	rax,rax
	jz	.missing
	fastcall rax
	mov	eax,1
	ret
  .missing:
	xor	eax,eax
	ret
endp

proc UahRuntimeAllowDarkModeForWindow runtimep,hwnd,dark
	mov	[runtimep],rcx
	mov	[hwnd],rdx
	mov	dword [dark],r8d
	mov	rax,[rcx+UAH_RUNTIME.AllowDarkModeForWindow]
	test	rax,rax
	jz	.missing
	fastcall rax,[hwnd],dword [dark]
	ret
  .missing:
	xor	eax,eax
	ret
endp

proc UahRuntimeRefreshImmersiveColorPolicyState runtimep
	mov	rax,[rcx+UAH_RUNTIME.RefreshImmersiveColorPolicyState]
	test	rax,rax
	jz	.missing
	fastcall rax
	mov	eax,1
	ret
  .missing:
	xor	eax,eax
	ret
endp

proc UahIsMenuWindow hwnd
    locals
	class_name rw UAH_CLASS_CHARS
    endl
	mov	[hwnd],rcx
	invoke	GetClassNameW,[hwnd],addr class_name,UAH_CLASS_CHARS
	test	eax,eax
	jz	.no
	invoke	lstrcmpW,addr class_name,'#32768'
	test	eax,eax
	jnz	.no
	mov	eax,1
	ret
  .no:
	xor	eax,eax
	ret
endp

proc UahOpen
	fastcall UahRuntimeOpen,uah_runtime
	ret
endp

proc UahClose
	fastcall UahRuntimeClose,uah_runtime
	ret
endp

proc UahSetPreferredAppMode mode
	mov	dword [mode],ecx
	fastcall UahRuntimeSetPreferredAppMode,uah_runtime,dword [mode]
	ret
endp

proc UahFlushMenuThemes
	fastcall UahRuntimeFlushMenuThemes,uah_runtime
	ret
endp

proc UahAllowDarkModeForWindow hwnd,dark
	mov	[hwnd],rcx
	mov	dword [dark],edx
	fastcall UahRuntimeAllowDarkModeForWindow,uah_runtime,[hwnd],dword [dark]
	ret
endp

proc UahRefreshImmersiveColorPolicyState
	fastcall UahRuntimeRefreshImmersiveColorPolicyState,uah_runtime
	ret
endp
