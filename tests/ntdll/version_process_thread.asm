
format PE64 NX console 6.0
entry start

include 'common_console64.inc'
include 'equates/ntdll_undoc.inc'

struct RTL_OSVERSIONINFOW
  dwOSVersionInfoSize dd ?
  dwMajorVersion      dd ?
  dwMinorVersion      dd ?
  dwBuildNumber       dd ?
  dwPlatformId        dd ?
  szCSDVersion        dw 128 dup (?)
ends

section '.data' data readable writeable

  console_storage

  return_length dd ?
  version RTL_OSVERSIONINFOW
  pbi PROCESS_BASIC_INFORMATION
  tbi THREAD_BASIC_INFORMATION

section '.text' code readable executable

  fastcall.frame = 0

  start:
        sub     rsp,8+TEST_FRAME
        console_init

        mov     dword [version.dwOSVersionInfoSize],sizeof.RTL_OSVERSIONINFOW
        invoke  RtlGetVersion,addr version
        test    eax,eax
        jnz     failed

        cmp     dword [version.dwMajorVersion],6
        jb      failed
        cmp     dword [version.dwBuildNumber],0
        je      failed

        invoke  NtQueryInformationProcess,NtCurrentProcess,ProcessBasicInformation,addr pbi,sizeof.PROCESS_BASIC_INFORMATION,addr return_length
        test    eax,eax
        jnz     failed

        cmp     qword [pbi.UniqueProcessId],0
        je      failed

        invoke  NtQueryInformationThread,NtCurrentThread,ThreadBasicInformation,addr tbi,sizeof.THREAD_BASIC_INFORMATION,addr return_length
        test    eax,eax
        jnz     failed

        mov     rax,[pbi.UniqueProcessId]
        cmp     [tbi.ClientId.UniqueProcess],rax
        jne     failed

        cmp     qword [tbi.ClientId.UniqueThread],0
        je      failed

        test_pass 'version_process_thread: PASS'

  failed:
        test_fail 1,'version_process_thread: FAIL'

  TEST_FRAME := fastcall.frame

section '.idata' import data readable writeable

  ntdll_phnt_imports
