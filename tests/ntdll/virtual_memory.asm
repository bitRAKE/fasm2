
format PE64 NX console 6.0
entry start

include 'common_console64.inc'
include 'equates/ntdll_undoc.inc'

section '.data' data readable writeable

  console_storage

  base_address dq ?
  region_size dq ?

section '.text' code readable executable

  fastcall.frame = 0

  start:
        sub     rsp,8+TEST_FRAME
        console_init

        mov     qword [base_address],0
        mov     qword [region_size],4096

        invoke  NtAllocateVirtualMemory,NtCurrentProcess,addr base_address,0,addr region_size,MEM_RESERVE+MEM_COMMIT,PAGE_READWRITE
        test    eax,eax
        jnz     failed

        mov     rax,[base_address]
        test    rax,rax
        jz      failed
        mov     rdx,0CAFEBABE12345678h
        mov     [rax],rdx
        cmp     qword [rax],rdx
        jne     release_fail

        mov     qword [region_size],0
        invoke  NtFreeVirtualMemory,NtCurrentProcess,addr base_address,addr region_size,MEM_RELEASE
        test    eax,eax
        jnz     failed

        test_pass 'virtual_memory: PASS'

  release_fail:
        mov     qword [region_size],0
        invoke  NtFreeVirtualMemory,NtCurrentProcess,addr base_address,addr region_size,MEM_RELEASE

  failed:
        test_fail 1,'virtual_memory: FAIL'

  TEST_FRAME := fastcall.frame

section '.idata' import data readable writeable

  ntdll_phnt_imports
