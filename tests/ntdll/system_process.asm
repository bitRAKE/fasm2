
format PE64 NX console 6.0
entry start

include 'common_console64.inc'
include 'equates/ntdll_undoc.inc'

PROCESS_BUFFER_SIZE = 4 * 1024 * 1024

section '.data' data readable writeable

  console_storage

  return_length dd ?
  buffer dq ?
  buffer_size dq ?
  pbi PROCESS_BASIC_INFORMATION

section '.text' code readable executable

  fastcall.frame = 0

  start:
        sub     rsp,8+TEST_FRAME
        console_init

        invoke  NtQueryInformationProcess,NtCurrentProcess,ProcessBasicInformation,addr pbi,sizeof.PROCESS_BASIC_INFORMATION,addr return_length
        test    eax,eax
        jnz     failed

        mov     qword [buffer],0
        mov     qword [buffer_size],PROCESS_BUFFER_SIZE
        invoke  NtAllocateVirtualMemory,NtCurrentProcess,addr buffer,0,addr buffer_size,MEM_RESERVE+MEM_COMMIT,PAGE_READWRITE
        test    eax,eax
        jnz     failed

        invoke  NtQuerySystemInformation,SystemProcessInformation,[buffer],PROCESS_BUFFER_SIZE,addr return_length
        test    eax,eax
        jnz     release_fail

        mov     rsi,[buffer]
        mov     rbx,[pbi.UniqueProcessId]
        mov     ecx,4096

  find_process:
        cmp     [rsi+SYSTEM_PROCESS_INFORMATION.UniqueProcessId],rbx
        je      found_process

        mov     eax,[rsi+SYSTEM_PROCESS_INFORMATION.NextEntryOffset]
        test    eax,eax
        jz      release_fail
        add     rsi,rax
        dec     ecx
        jnz     find_process
        jmp     release_fail

  found_process:
        cmp     dword [rsi+SYSTEM_PROCESS_INFORMATION.NumberOfThreads],0
        je      release_fail
        mov     rax,qword [rsi+SYSTEM_PROCESS_INFORMATION.Threads+SYSTEM_THREAD_INFORMATION.ClientId+CLIENT_ID.UniqueProcess]
        cmp     rax,rbx
        jne     release_fail

        mov     qword [buffer_size],0
        invoke  NtFreeVirtualMemory,NtCurrentProcess,addr buffer,addr buffer_size,MEM_RELEASE
        test    eax,eax
        jnz     failed

        test_pass 'system_process: PASS'

  release_fail:
        mov     qword [buffer_size],0
        invoke  NtFreeVirtualMemory,NtCurrentProcess,addr buffer,addr buffer_size,MEM_RELEASE

  failed:
        test_fail 1,'system_process: FAIL'

  TEST_FRAME := fastcall.frame

section '.idata' import data readable writeable

  ntdll_phnt_imports
