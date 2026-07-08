; addon/csd/backdrop.inc - DWM system backdrop policy helpers.

DWMWA_USE_HOSTBACKDROPBRUSH	= 17
DWMWA_SYSTEMBACKDROP_TYPE	= 38
DWMWA_REDIRECTIONBITMAP_ALPHA	= 39

DWMSBT_AUTO			= 0
DWMSBT_NONE			= 1
DWMSBT_MAINWINDOW		= 2
DWMSBT_TRANSIENTWINDOW		= 3
DWMSBT_TABBEDWINDOW		= 4

CSD_BACKDROP_WINDOWS11_BUILD	= 22621
CSD_BACKDROP_ALPHA_BUILD	= 26100
CSD_BACKDROP_E_NOTIMPL		= 80004001h

struct CSD_BACKDROP_POLICY
  capable	dd ?
  osMajor	dd ?
  osMinor	dd ?
  osBuild	dd ?
  requestedType dd ?
  appliedType	dd ?
  readbackType dd ?
  lastResult	dd ?
  readbackResult dd ?
ends

struct CSD_BACKDROP_VIEWER_CONFIG
  hostEnabled	dd ?
  alphaEnabled	dd ?
  alphaCapable	dd ?
  hostResult	dd ?
  alphaResult	dd ?
ends

proc CsdBackdropProbePolicy uses rbx, policyp
    locals
	osver		OSVERSIONINFO
	ntdll		dq ?
	rtlGetVersion	dq ?
    endl

	mov	rbx,rcx
	mov	dword [rbx+CSD_BACKDROP_POLICY.capable],0
	mov	dword [rbx+CSD_BACKDROP_POLICY.osMajor],0
	mov	dword [rbx+CSD_BACKDROP_POLICY.osMinor],0
	mov	dword [rbx+CSD_BACKDROP_POLICY.osBuild],0
	mov	dword [rbx+CSD_BACKDROP_POLICY.requestedType],DWMSBT_MAINWINDOW
	mov	dword [rbx+CSD_BACKDROP_POLICY.appliedType],DWMSBT_NONE
	mov	dword [rbx+CSD_BACKDROP_POLICY.readbackType],DWMSBT_NONE
	mov	dword [rbx+CSD_BACKDROP_POLICY.lastResult],CSD_BACKDROP_E_NOTIMPL
	mov	dword [rbx+CSD_BACKDROP_POLICY.readbackResult],CSD_BACKDROP_E_NOTIMPL

	mov	dword [osver.dwOSVersionInfoSize],sizeof.OSVERSIONINFO
	invoke	GetModuleHandle,'ntdll.dll'
	mov	[ntdll],rax
	test	rax,rax
	jz	backdrop_probe_done
	invoke	GetProcAddress,[ntdll],<A,'RtlGetVersion'>
	mov	[rtlGetVersion],rax
	test	rax,rax
	jz	backdrop_probe_done
	fastcall [rtlGetVersion],addr osver
	test	eax,eax
	jnz	backdrop_probe_done

	mov	eax,[osver.dwMajorVersion]
	mov	[rbx+CSD_BACKDROP_POLICY.osMajor],eax
	mov	eax,[osver.dwMinorVersion]
	mov	[rbx+CSD_BACKDROP_POLICY.osMinor],eax
	mov	eax,[osver.dwBuildNumber]
	mov	[rbx+CSD_BACKDROP_POLICY.osBuild],eax

	cmp	dword [osver.dwMajorVersion],10
	jg	backdrop_probe_enable
	jne	backdrop_probe_done
	cmp	dword [osver.dwBuildNumber],CSD_BACKDROP_WINDOWS11_BUILD
	jl	backdrop_probe_done

  backdrop_probe_enable:
	mov	dword [rbx+CSD_BACKDROP_POLICY.capable],1
	mov	dword [rbx+CSD_BACKDROP_POLICY.lastResult],0

  backdrop_probe_done:
	mov	eax,[rbx+CSD_BACKDROP_POLICY.capable]
	ret
endp

proc CsdBackdropSetRequested policyp,requestedType
	cmp	edx,DWMSBT_NONE
	jb	backdrop_set_none
	cmp	edx,DWMSBT_TABBEDWINDOW
	ja	backdrop_set_none
	mov	[rcx+CSD_BACKDROP_POLICY.requestedType],edx
	mov	eax,edx
	ret

  backdrop_set_none:
	mov	dword [rcx+CSD_BACKDROP_POLICY.requestedType],DWMSBT_NONE
	mov	eax,DWMSBT_NONE
	ret
endp

proc CsdBackdropCycle policyp
	cmp	dword [rcx+CSD_BACKDROP_POLICY.capable],0
	jne	backdrop_cycle_ready
	xor	eax,eax
	ret

  backdrop_cycle_ready:
	mov	eax,[rcx+CSD_BACKDROP_POLICY.requestedType]
	cmp	eax,DWMSBT_NONE
	je	backdrop_cycle_main
	cmp	eax,DWMSBT_MAINWINDOW
	je	backdrop_cycle_transient
	cmp	eax,DWMSBT_TRANSIENTWINDOW
	je	backdrop_cycle_tabbed
	mov	eax,DWMSBT_NONE
	jmp	backdrop_cycle_store

  backdrop_cycle_main:
	mov	eax,DWMSBT_MAINWINDOW
	jmp	backdrop_cycle_store

  backdrop_cycle_transient:
	mov	eax,DWMSBT_TRANSIENTWINDOW
	jmp	backdrop_cycle_store

  backdrop_cycle_tabbed:
	mov	eax,DWMSBT_TABBEDWINDOW

  backdrop_cycle_store:
	mov	[rcx+CSD_BACKDROP_POLICY.requestedType],eax
	mov	eax,1
	ret
endp

proc CsdBackdropApply hwnd,policyp
    locals
	value dd ?
    endl

	mov	[hwnd],rcx
	mov	[policyp],rdx
	cmp	dword [rdx+CSD_BACKDROP_POLICY.capable],0
	jne	backdrop_apply_capable
	mov	dword [rdx+CSD_BACKDROP_POLICY.appliedType],DWMSBT_NONE
	mov	dword [rdx+CSD_BACKDROP_POLICY.readbackType],DWMSBT_NONE
	mov	dword [rdx+CSD_BACKDROP_POLICY.lastResult],CSD_BACKDROP_E_NOTIMPL
	mov	dword [rdx+CSD_BACKDROP_POLICY.readbackResult],CSD_BACKDROP_E_NOTIMPL
	mov	eax,CSD_BACKDROP_E_NOTIMPL
	ret

  backdrop_apply_capable:
	mov	eax,[rdx+CSD_BACKDROP_POLICY.requestedType]
	mov	[value],eax
	mov	dword [rdx+CSD_BACKDROP_POLICY.readbackType],DWMSBT_NONE
	invoke	DwmSetWindowAttribute,[hwnd],DWMWA_SYSTEMBACKDROP_TYPE,addr value,4
	mov	rdx,[policyp]
	mov	[rdx+CSD_BACKDROP_POLICY.lastResult],eax
	mov	[rdx+CSD_BACKDROP_POLICY.readbackResult],eax
	test	eax,eax
	jnz	backdrop_apply_done
	mov	ecx,[value]
	mov	[rdx+CSD_BACKDROP_POLICY.appliedType],ecx
	invoke	DwmGetWindowAttribute,[hwnd],DWMWA_SYSTEMBACKDROP_TYPE,addr value,4
	mov	rdx,[policyp]
	mov	[rdx+CSD_BACKDROP_POLICY.readbackResult],eax
	test	eax,eax
	jnz	backdrop_apply_done
	mov	ecx,[value]
	mov	[rdx+CSD_BACKDROP_POLICY.readbackType],ecx

  backdrop_apply_done:
	mov	eax,[rdx+CSD_BACKDROP_POLICY.lastResult]
	ret
endp

proc CsdBackdropViewerConfigInit configp,policyp
	mov	dword [rcx+CSD_BACKDROP_VIEWER_CONFIG.hostEnabled],1
	mov	dword [rcx+CSD_BACKDROP_VIEWER_CONFIG.alphaEnabled],1
	mov	dword [rcx+CSD_BACKDROP_VIEWER_CONFIG.alphaCapable],0
	mov	dword [rcx+CSD_BACKDROP_VIEWER_CONFIG.hostResult],CSD_BACKDROP_E_NOTIMPL
	mov	dword [rcx+CSD_BACKDROP_VIEWER_CONFIG.alphaResult],CSD_BACKDROP_E_NOTIMPL
	cmp	dword [rdx+CSD_BACKDROP_POLICY.capable],0
	je	backdrop_viewer_init_done
	cmp	dword [rdx+CSD_BACKDROP_POLICY.osBuild],CSD_BACKDROP_ALPHA_BUILD
	jl	backdrop_viewer_init_done
	mov	dword [rcx+CSD_BACKDROP_VIEWER_CONFIG.alphaCapable],1

  backdrop_viewer_init_done:
	ret
endp

proc CsdBackdropViewerApply hwnd,configp
    locals
	value dd ?
    endl

	mov	[hwnd],rcx
	mov	[configp],rdx
	mov	eax,[rdx+CSD_BACKDROP_VIEWER_CONFIG.hostEnabled]
	mov	[value],eax
	invoke	DwmSetWindowAttribute,[hwnd],DWMWA_USE_HOSTBACKDROPBRUSH,addr value,4
	mov	rdx,[configp]
	mov	[rdx+CSD_BACKDROP_VIEWER_CONFIG.hostResult],eax
	cmp	dword [rdx+CSD_BACKDROP_VIEWER_CONFIG.alphaCapable],0
	jne	backdrop_viewer_apply_alpha
	mov	dword [rdx+CSD_BACKDROP_VIEWER_CONFIG.alphaResult],CSD_BACKDROP_E_NOTIMPL
	mov	eax,[rdx+CSD_BACKDROP_VIEWER_CONFIG.hostResult]
	ret

  backdrop_viewer_apply_alpha:
	mov	eax,[rdx+CSD_BACKDROP_VIEWER_CONFIG.alphaEnabled]
	mov	[value],eax
	invoke	DwmSetWindowAttribute,[hwnd],DWMWA_REDIRECTIONBITMAP_ALPHA,addr value,4
	mov	rdx,[configp]
	mov	[rdx+CSD_BACKDROP_VIEWER_CONFIG.alphaResult],eax
	mov	eax,[rdx+CSD_BACKDROP_VIEWER_CONFIG.hostResult]
	ret
endp

proc CsdBackdropViewerDisable hwnd,configp
    locals
	value dd ?
    endl

	mov	[hwnd],rcx
	mov	[configp],rdx
	mov	dword [value],0
	invoke	DwmSetWindowAttribute,[hwnd],DWMWA_USE_HOSTBACKDROPBRUSH,addr value,4
	mov	rdx,[configp]
	mov	[rdx+CSD_BACKDROP_VIEWER_CONFIG.hostResult],eax
	cmp	dword [rdx+CSD_BACKDROP_VIEWER_CONFIG.alphaCapable],0
	je	backdrop_viewer_disable_done
	invoke	DwmSetWindowAttribute,[hwnd],DWMWA_REDIRECTIONBITMAP_ALPHA,addr value,4
	mov	rdx,[configp]
	mov	[rdx+CSD_BACKDROP_VIEWER_CONFIG.alphaResult],eax

  backdrop_viewer_disable_done:
	ret
endp
