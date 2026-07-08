; addon/csd/snap.inc - maximize hit-test policy for Windows 11 snap layouts.

CSD_SNAP_WINDOWS11_BUILD	= 22000

struct CSD_SNAP_POLICY
  snapCapable	dd ?
  maxHitCode	dd ?
  osMajor	dd ?
  osMinor	dd ?
  osBuild	dd ?
ends

proc CsdSnapProbePolicy uses rbx, policyp
    locals
	osver		OSVERSIONINFO
	ntdll		dq ?
	rtlGetVersion	dq ?
    endl

	mov	rbx,rcx
	mov	dword [rbx+CSD_SNAP_POLICY.snapCapable],0
	mov	dword [rbx+CSD_SNAP_POLICY.maxHitCode],HTCLIENT
	mov	dword [rbx+CSD_SNAP_POLICY.osMajor],0
	mov	dword [rbx+CSD_SNAP_POLICY.osMinor],0
	mov	dword [rbx+CSD_SNAP_POLICY.osBuild],0

	mov	dword [osver.dwOSVersionInfoSize],sizeof.OSVERSIONINFO
	invoke	GetModuleHandle,'ntdll.dll'
	mov	[ntdll],rax
	test	rax,rax
	jz	snap_done
	invoke	GetProcAddress,[ntdll],<A,'RtlGetVersion'>
	mov	[rtlGetVersion],rax
	test	rax,rax
	jz	snap_done
	fastcall [rtlGetVersion],addr osver
	test	eax,eax
	jnz	snap_done

	mov	eax,[osver.dwMajorVersion]
	mov	[rbx+CSD_SNAP_POLICY.osMajor],eax
	mov	eax,[osver.dwMinorVersion]
	mov	[rbx+CSD_SNAP_POLICY.osMinor],eax
	mov	eax,[osver.dwBuildNumber]
	mov	[rbx+CSD_SNAP_POLICY.osBuild],eax

	cmp	dword [osver.dwMajorVersion],10
	jg	snap_enable
	jne	snap_done
	cmp	dword [osver.dwBuildNumber],CSD_SNAP_WINDOWS11_BUILD
	jl	snap_done

  snap_enable:
	mov	dword [rbx+CSD_SNAP_POLICY.snapCapable],1
	mov	dword [rbx+CSD_SNAP_POLICY.maxHitCode],HTMAXBUTTON

  snap_done:
	mov	eax,[rbx+CSD_SNAP_POLICY.snapCapable]
	ret
endp
