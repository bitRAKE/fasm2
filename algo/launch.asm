if __FILE__ <> __SOURCE__

	extrn LaunchNormal ; lpCurrentDirectory,lpCommandLine,lpEnvironment
	extrn LaunchElevated ; lpCurrentDirectory,lpCommandLine,lpEnvironment
	extrn LaunchWithLowIntegrity ; lpCurrentDirectory,lpCommandLine,lpEnvironment
	extrn LaunchRestricted ; lpCurrentDirectory,lpCommandLine,lpEnvironment

else ; fasm2 launch.asm

format MS64 NEWCOFF
include 'win64w.inc'

extrn '__imp_CreateProcessW' as CreateProcessW:QWORD
extrn '__imp_CloseHandle' as CloseHandle:QWORD


section '.text$LaunchNormal' code readable executable comdat align 16
LaunchNormal:
public LaunchNormal
cvproc LaunchNormal

; Optimization case: parameter location on the stack known during determination of parameter values reduces data movement, but requires invocation deconstruction.

; RCX:	lpCurrentDirectory
; RDX:	lpCommandLine
; R8:	lpEnvironment

; * Use quote marks around application pathname.

	virtual at rsp
		.lpApplicationName	dq ?	; [in, OPTIONAL]      LPCWSTR
		.lpCommandLine		dq ?	; [in, out, optional] LPWSTR
		.lpProcessAttributes	dq ?	; [in, OPTIONAL]      LPSECURITY_ATTRIBUTES
		.lpThreadAttributes	dq ?	; [in, OPTIONAL]      LPSECURITY_ATTRIBUTES
		.bInheritHandles	dd ?,?	; [in]                BOOL
		.dwCreationFlags	dd ?,?	; [in]                DWORD
		.lpEnvironment		dq ?	; [in, optional]      LPVOID
		.lpCurrentDirectory	dq ?	; [in, optional]      LPCWSTR
		.lpStartupInfo		dq ?	; [in]                LPSTARTUPINFOW
		.lpProcessInformation	dq ?	; [out]               LPPROCESS_INFORMATION

		.frame := $-$$ + 8
	end virtual
	sub rsp, .frame
	cvframe .frame

	cvlocal .lpApplicationName:CV_T_64PVOID
	cvlocal .lpCommandLine:CV_T_64PVOID
	cvlocal .lpProcessAttributes:CV_T_64PVOID
	cvlocal .lpThreadAttributes:CV_T_64PVOID
	cvlocal .bInheritHandles:CV_T_UINT4
	cvlocal .dwCreationFlags:CV_T_UINT4
	cvlocal .lpEnvironment:CV_T_64PVOID
	cvlocal .lpCurrentDirectory:CV_T_64PVOID
	cvlocal .lpStartupInfo:CV_T_64PVOID
	cvlocal .lpProcessInformation:CV_T_64PVOID

	jrcxz .have_cwd
	cmp word [rcx], 0	; wchar_t
	jnz .have_cwd
	xor ecx, ecx		; calling process cwd is used
.have_cwd:
	mov [.lpCurrentDirectory], rcx

	mov eax, CREATE_UNICODE_ENVIRONMENT
	test r8, r8
	cmovz eax, r8d
	or eax, CREATE_NEW_CONSOLE
	mov [.dwCreationFlags], eax
	mov [.lpEnvironment], r8

	xor ecx, ecx
	xor r8, r8
	xor r9, r9
	mov [.bInheritHandles], ecx
	lea r10, [_si]
	lea r11, [_pi]
	mov [.lpStartupInfo], r10
	mov [.lpProcessInformation], r11
	call [CreateProcessW]
	test eax, eax ; BOOL
	jz .no_process
	mov rcx, [_pi.hThread]
	call [CloseHandle]
	mov rcx, [_pi.hProcess]
	call [CloseHandle]
.no_process:
	add rsp, .frame
	retn
cvendp


section '.data$LaunchNormal' data readable writeable comdat associative LaunchNormal align 16
	_si	STARTUPINFO cb:sizeof _si
; ... or w/ flag EXTENDED_STARTUPINFO_PRESENT
;	_si	STARTUPINFOEX cb:sizeof _si
	_pi	PROCESS_INFORMATION

;-------------------------------------------------------------------------------
; ShellExecuteEx has no lpEnvironment equivalent - an elevated process gets whatever environment the elevated host (consent.exe's spawned shell) naturally has, not a custom block. Any Environment entries on the profile are silently inert here; DebugExecuteProfile warns about this before calling in.
;
; Start-Process -FilePath helper.exe -ArgumentList '--do-admin-thing' -Verb RunAs
;
; Or via the shell COM object, no third-party tools
;	powershell -c "(New-Object -ComObject Shell.Application).ShellExecute('helper.exe','','','runas',1)"

section '.text$LaunchElevatedRunAs' code readable executable comdat align 16
LaunchElevatedRunAs:
public LaunchElevatedRunAs
cvproc LaunchElevatedRunAs
	virtual at rsp
		rq 4 ; shadow space
		.frame := $-$$ + 8
	end virtual
	sub rsp, .frame
	cvframe .frame

	xor eax, eax

	jrcxz .no_path
	cmp word [rcx], 0
	cmovz rcx, rax
.no_path:
	mov [.sei.lpDirectory], rcx
	lea rcx, [.sei]

	mov [rcx + SHELLEXECUTEINFOW.lpFile], rdx

	test r8, r8
	jz .no_params
	cmp word [r8], 0
	cmovz r8, rax
.no_params:
	mov [rcx + SHELLEXECUTEINFOW.lpParameters], r8

	call [ShellExecuteExW]
	test eax, eax ; BOOL, did user decline (ERROR_CANCELLED)?
	jz .no_process
	mov rcx, [.sei.hProcess]
	jrcxz .no_process
	call [CloseHandle]
.no_process:
	add rsp, .frame
	retn
cvendp


section '.rdata$LaunchElevatedRunAs' data readable comdat exactmatch align 1
	_runas du 'runas',0

section '.data$LaunchElevatedRunAs' data readable writeable comdat associative LaunchElevatedRunAs align 16
	sei SHELLEXECUTEINFOW cbSize: sizeof SHELLEXECUTEINFOW,\
		fMask: SEE_MASK_NOCLOSEPROCESS,\; populate hProcess
		lpVerb: _runas,\
		nShow: SW_SHOWNORMAL

;-------------------------------------------------------------------------------

section '.text$LaunchWithLowIntegrity' code readable executable comdat align 16
LaunchWithLowIntegrity:
public LaunchWithLowIntegrity
cvproc LaunchWithLowIntegrity
	retn
cvendp

section '.data$LaunchWithLowIntegrity' data readable writeable comdat associative LaunchWithLowIntegrity align 16

;-------------------------------------------------------------------------------

section '.text$LaunchRestricted' code readable executable comdat align 16
LaunchRestricted:
public LaunchRestricted
cvproc LaunchRestricted
	retn
cvendp

section '.data$LaunchRestricted' data readable writeable comdat associative LaunchRestricted align 16

end if ; __FILE__ <> __SOURCE__
