
format PE64 NX console 6.0
entry start

include 'common_console64.inc'
include 'equates/ntdll_undoc.inc'

section '.data' data readable writeable

  console_storage

  heap_handle dq ?
  block dq ?

section '.text' code readable executable

  fastcall.frame = 0

  start:
        sub     rsp,8+TEST_FRAME
        console_init

        invoke  RtlCreateHeap,HEAP_GROWABLE,0,0,0,0,0
        test    rax,rax
        jz      failed
        mov     [heap_handle],rax

        invoke  RtlAllocateHeap,[heap_handle],HEAP_ZERO_MEMORY,256
        test    rax,rax
        jz      destroy_fail
        mov     [block],rax

        cmp     qword [rax],0
        jne     free_fail
        mov     rdx,0123456789ABCDEFh
        mov     [rax],rdx

        invoke  RtlSizeHeap,[heap_handle],0,[block]
        cmp     rax,-1
        je      free_fail
        cmp     rax,256
        jb      free_fail

        invoke  RtlValidateHeap,[heap_handle],0,[block]
        test    eax,eax
        jz      free_fail

        invoke  RtlFreeHeap,[heap_handle],0,[block]
        test    eax,eax
        jz      destroy_fail

        invoke  RtlDestroyHeap,[heap_handle]
        test    rax,rax
        jnz     failed
        test_pass 'native_heap: PASS'

  free_fail:
        invoke  RtlFreeHeap,[heap_handle],0,[block]

  destroy_fail:
        invoke  RtlDestroyHeap,[heap_handle]

  failed:
        test_fail 1,'native_heap: FAIL'

  TEST_FRAME := fastcall.frame

section '.idata' import data readable writeable

  ntdll_phnt_imports
