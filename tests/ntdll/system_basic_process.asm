
format PE64 NX console 6.0
entry start

include 'common_console64.inc'
include 'equates/ntdll.inc'

PROCESS_BUFFER_SIZE = 4 * 1024 * 1024

section '.data' data readable writeable

  console_storage

  return_length dd ?
  buffer dq ?

section '.text' code readable executable

  fastcall.frame = 0

  start:
        sub     rsp,8+TEST_FRAME
        console_init

        invoke  VirtualAlloc,0,PROCESS_BUFFER_SIZE,MEM_RESERVE+MEM_COMMIT,PAGE_READWRITE
        test    rax,rax
        jz      failed
        mov     [buffer],rax

        invoke  NtQuerySystemInformation,SystemBasicProcessInformation,[buffer],PROCESS_BUFFER_SIZE,addr return_length
        test    eax,eax
        jz      validate

        cmp     eax,STATUS_INVALID_INFO_CLASS
        je      unsupported
        cmp     eax,STATUS_NOT_IMPLEMENTED
        je      unsupported
        jmp     release_fail

  validate:
        mov     rsi,[buffer]
        cmp     dword [rsi+SYSTEM_BASICPROCESS_INFORMATION.NextEntryOffset],0
        jne     release_pass
        cmp     qword [rsi+SYSTEM_BASICPROCESS_INFORMATION.UniqueProcessId],0
        jne     release_pass
        cmp     word [rsi+SYSTEM_BASICPROCESS_INFORMATION.ImageName.Length],0
        jne     release_pass
        jmp     release_fail

  unsupported:
        invoke  VirtualFree,[buffer],0,MEM_RELEASE
        test    eax,eax
        jz      failed
        test_skip 'system_basic_process: SKIP'

  release_pass:
        invoke  VirtualFree,[buffer],0,MEM_RELEASE
        test    eax,eax
        jz      failed
        test_pass 'system_basic_process: PASS'

  release_fail:
        invoke  VirtualFree,[buffer],0,MEM_RELEASE

  failed:
        test_fail 1,'system_basic_process: FAIL'

  TEST_FRAME := fastcall.frame

section '.idata' import data readable writeable

  ntdll_sdk_imports
