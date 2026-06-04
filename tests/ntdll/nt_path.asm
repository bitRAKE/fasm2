
format PE64 NX console 6.0
entry start

include 'common_console64.inc'
include 'equates/ntdll_undoc.inc'

section '.data' data readable writeable

  console_storage

  dos_path du 'C:\Windows\System32\ntdll.dll',0
  nt_name UNICODE_STRING

section '.text' code readable executable

  fastcall.frame = 0

  start:
        sub     rsp,8+TEST_FRAME
        console_init

        invoke  RtlDosPathNameToNtPathName_U_WithStatus,dos_path,addr nt_name,0,0
        test    eax,eax
        jnz     failed

        cmp     word [nt_name.Length],8
        jb      free_fail

        mov     rax,[nt_name.Buffer]
        test    rax,rax
        jz      free_fail
        cmp     word [rax],'\'
        jne     free_fail

        invoke  RtlFreeUnicodeString,addr nt_name
        test_pass 'nt_path: PASS'

  free_fail:
        invoke  RtlFreeUnicodeString,addr nt_name

  failed:
        test_fail 1,'nt_path: FAIL'

  TEST_FRAME := fastcall.frame

section '.idata' import data readable writeable

  ntdll_phnt_imports
