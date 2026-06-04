
include 'win64a.inc'

STD_OUTPUT_HANDLE = -11

STATUS_SUCCESS              = 00000000h
STATUS_NOT_IMPLEMENTED      = 0C0000002h
STATUS_INVALID_INFO_CLASS   = 0C0000003h
STATUS_INFO_LENGTH_MISMATCH = 0C0000004h
STATUS_BUFFER_TOO_SMALL     = 0C0000023h

HEAP_GROWABLE = 00000002h

macro console_storage
  console_stdout dq ?
  console_written dd ?
end macro

macro console_init
  invoke GetStdHandle,STD_OUTPUT_HANDLE
  mov [console_stdout],rax
end macro

macro console_write text&
  local message, after_message, message_size
  jmp after_message
  message db text,13,10
  message_size = $ - message
  after_message:
  invoke WriteFile,[console_stdout],message,message_size,addr console_written,0
end macro

macro test_pass text&
  console_write text
  invoke ExitProcess,0
end macro

macro test_skip text&
  console_write text
  invoke ExitProcess,0
end macro

macro test_fail code*,text&
  console_write text
  invoke ExitProcess,code
end macro

macro ntdll_sdk_imports
  library kernel32,'KERNEL32.DLL',\
          ntdll,'NTDLL.DLL'
  include 'api/kernel32.inc'
  include 'api/ntdll.inc'
end macro

macro ntdll_phnt_imports
  library kernel32,'KERNEL32.DLL',\
          ntdll,'NTDLL.DLL'
  include 'api/kernel32.inc'
  include 'api/ntdll_undoc.inc'
end macro
