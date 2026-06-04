
format PE64 NX console 6.0
entry start

include 'common_console64.inc'
include 'equates/ntdll.inc'

section '.data' data readable writeable

  console_storage

  minimum_resolution dd ?
  maximum_resolution dd ?
  current_resolution dd ?

section '.text' code readable executable

  fastcall.frame = 0

  start:
        sub     rsp,8+TEST_FRAME
        console_init

        invoke  NtQueryTimerResolution,addr minimum_resolution,addr maximum_resolution,addr current_resolution
        test    eax,eax
        jnz     failed

        cmp     dword [minimum_resolution],0
        je      failed
        cmp     dword [maximum_resolution],0
        je      failed
        cmp     dword [current_resolution],0
        je      failed

        test_pass 'timer_resolution: PASS'

  failed:
        test_fail 1,'timer_resolution: FAIL'

  TEST_FRAME := fastcall.frame

section '.idata' import data readable writeable

  ntdll_sdk_imports
