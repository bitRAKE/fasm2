
; NTDLL.DLL structures and constants
; Windows SDK 10.0.26100.0 user-mode surface:
;   winternl.h, rtlsupportapi.h, ip2string.h/mstcpip.h, sysinfoapi.h, winnt.h.
; Use this as the SDK surface; use ntdll_undoc.inc instead for PHNT-native code.

iterate <PTR,PTRWORD,PTRBYTES>,\
    dd,dword,4,\
    dq,qword,8

  if x86.mode = 64
    indx 2
  end if

  NTDLL_PTR_SIZE = PTRBYTES

  ; Basic NT list and string structures

  struct LIST_ENTRY
    Flink PTR ?
    Blink PTR ?
  ends

  struct SINGLE_LIST_ENTRY
    Next PTR ?
  ends

  struct SLIST_ENTRY
    Next PTR ?
    if PTRBYTES = 8
      PTR ?      ; DECLSPEC_ALIGN(16) tail padding
    end if
  ends

  struct SLIST_HEADER
    Alignment dq ?
    if PTRBYTES = 8
      Region dq ?
    end if
  ends

  if PTRBYTES = 4
    SLIST_HEADER.Next  = 0
    SLIST_HEADER.Depth = 4
    SLIST_HEADER.CpuId = 6
  end if

  struct STRING
    Length        dw ?
    MaximumLength dw ?
    if PTRBYTES = 8
      dd ?        ; align Buffer
    end if
    Buffer PTR ?
  ends

  struct ANSI_STRING
    Length        dw ?
    MaximumLength dw ?
    if PTRBYTES = 8
      dd ?        ; align Buffer
    end if
    Buffer PTR ?
  ends

  struct OEM_STRING
    Length        dw ?
    MaximumLength dw ?
    if PTRBYTES = 8
      dd ?        ; align Buffer
    end if
    Buffer PTR ?
  ends

  struct UNICODE_STRING
    Length        dw ?
    MaximumLength dw ?
    if PTRBYTES = 8
      dd ?        ; align Buffer
    end if
    Buffer PTR ?
  ends

  struct CLIENT_ID
    UniqueProcess PTR ?
    UniqueThread  PTR ?
  ends

  ; PEB/TEB structures expose only the SDK fields. Reserved fields are not
  ; intended to describe the private NT layout beyond preserving these offsets.

  struct PEB_LDR_DATA
    Reserved1 rb 8
    Reserved2 PTR 3 dup (?)
    InMemoryOrderModuleList LIST_ENTRY
  ends

  struct LDR_DATA_TABLE_ENTRY
    Reserved1          PTR 2 dup (?)
    InMemoryOrderLinks LIST_ENTRY
    Reserved2          PTR 2 dup (?)
    DllBase            PTR ?
    Reserved3          PTR 2 dup (?)
    FullDllName        UNICODE_STRING
    Reserved4          rb 8
    Reserved5          PTR 3 dup (?)
    CheckSum           dd ?
    if PTRBYTES = 8
                       dd ?    ; union padding for PVOID Reserved6
    end if
    TimeDateStamp      dd ?
    if PTRBYTES = 8
                       dd ?    ; tail padding
    end if
  ends

  LDR_DATA_TABLE_ENTRY.Reserved6 = LDR_DATA_TABLE_ENTRY.CheckSum

  struct RTL_USER_PROCESS_PARAMETERS
    Reserved1 rb 16
    Reserved2     PTR 10 dup (?)
    ImagePathName UNICODE_STRING
    CommandLine   UNICODE_STRING
  ends

  struct PEB
    Reserved1              rb 2
    BeingDebugged          db ?
    Reserved2              rb 1
    if PTRBYTES = 8
                             dd ?      ; align Reserved3
    end if
    Reserved3              PTR 2 dup (?)
    Ldr                    PTR ?
    ProcessParameters      PTR ?
    Reserved4              PTR 3 dup (?)
    AtlThunkSListPtr       PTR ?
    Reserved5              PTR ?
    Reserved6              dd ?
    if PTRBYTES = 8
                             dd ?      ; align Reserved7
    end if
    Reserved7              PTR ?
    Reserved8              dd ?
    AtlThunkSListPtr32     dd ?
    Reserved9              PTR 45 dup (?)
    Reserved10             rb 96
    PostProcessInitRoutine PTR ?
    Reserved11             rb 128
    Reserved12             PTR 1 dup (?)
    SessionId              dd ?
    if PTRBYTES = 8
                             dd ?      ; tail padding
    end if
  ends

  struct TEB
    Reserved1                 PTR 12 dup (?)
    ProcessEnvironmentBlock   PTR ?
    Reserved2                 PTR 399 dup (?)
    Reserved3                 rb 1952
    TlsSlots                  PTR 64 dup (?)
    Reserved4                 rb 8
    Reserved5                 PTR 26 dup (?)
    ReservedForOle            PTR ?
    Reserved6                 PTR 4 dup (?)
    TlsExpansionSlots         PTR ?
  ends

  ; Native object, I/O, process, thread, and system query structures

  struct OBJECT_ATTRIBUTES
    Length                   dd ?
    if PTRBYTES = 8
                             dd ?    ; align RootDirectory
    end if
    RootDirectory            PTR ?
    ObjectName               PTR ?
    Attributes               dd ?
    if PTRBYTES = 8
                             dd ?    ; align SecurityDescriptor
    end if
    SecurityDescriptor       PTR ?
    SecurityQualityOfService PTR ?
  ends

  struct IO_STATUS_BLOCK
    Status      dd ?
    if PTRBYTES = 8
                dd ?        ; union padding for PVOID Pointer
    end if
    Information PTR ?
  ends

  IO_STATUS_BLOCK.Pointer = IO_STATUS_BLOCK.Status

  struct PROCESS_BASIC_INFORMATION
    Reserved1       PTR ?
    PebBaseAddress  PTR ?
    Reserved2       PTR 2 dup (?)
    UniqueProcessId PTR ?
    Reserved3       PTR ?
  ends

  struct SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
    IdleTime  dq ?
    KernelTime dq ?
    UserTime   dq ?
    Reserved1  dq 2 dup (?)
    Reserved2  dd ?
                dd ?        ; tail padding, sizeof=48 on x86/x64
  ends

  struct SYSTEM_PROCESS_INFORMATION
    NextEntryOffset      dd ?
    NumberOfThreads      dd ?
    Reserved1            rb 48
    ImageName            UNICODE_STRING
    BasePriority         dd ?
    if PTRBYTES = 8
                         dd ?        ; align UniqueProcessId
    end if
    UniqueProcessId      PTR ?
    Reserved2            PTR ?
    HandleCount          dd ?
    SessionId            dd ?
    Reserved3            PTR ?
    PeakVirtualSize      PTR ?
    VirtualSize          PTR ?
    Reserved4            dd ?
    if PTRBYTES = 8
                         dd ?        ; align PeakWorkingSetSize
    end if
    PeakWorkingSetSize   PTR ?
    WorkingSetSize       PTR ?
    Reserved5            PTR ?
    QuotaPagedPoolUsage  PTR ?
    Reserved6            PTR ?
    QuotaNonPagedPoolUsage PTR ?
    PagefileUsage        PTR ?
    PeakPagefileUsage    PTR ?
    PrivatePageCount     PTR ?
    Reserved7            dq 6 dup (?)
  ends

  struct SYSTEM_BASICPROCESS_INFORMATION
    NextEntryOffset              dd ?
    if PTRBYTES = 8
                                 dd ?        ; align UniqueProcessId
    end if
    UniqueProcessId             PTR ?
    InheritedFromUniqueProcessId PTR ?
    if PTRBYTES = 4
                                 dd ?        ; align SequenceNumber
    end if
    SequenceNumber              dq ?
    ImageName                   UNICODE_STRING
  ends

  struct SYSTEM_HANDLECOUNT_INFORMATION
    ProcessCount dd ?
    ThreadCount  dd ?
    HandleCount  dd ?
  ends

  struct SYSTEM_THREAD_INFORMATION
    Reserved1    dq 3 dup (?)
    Reserved2    dd ?
    if PTRBYTES = 8
                 dd ?        ; align StartAddress
    end if
    StartAddress PTR ?
    ClientId     CLIENT_ID
    Priority     dd ?
    BasePriority dd ?
    Reserved3    dd ?
    ThreadState  dd ?
    WaitReason   dd ?
                 dd ?        ; tail padding
  ends

  struct SYSTEM_REGISTRY_QUOTA_INFORMATION
    RegistryQuotaAllowed dd ?
    RegistryQuotaUsed    dd ?
    Reserved1            PTR ?
  ends

  struct SYSTEM_BASIC_INFORMATION
    Reserved1 rb 24
    Reserved2          PTR 4 dup (?)
    NumberOfProcessors db ?
                       rb PTRBYTES - 1
  ends

  struct SYSTEM_TIMEOFDAY_INFORMATION
    Reserved1 rb 48
  ends

  struct SYSTEM_PERFORMANCE_INFORMATION
    Reserved1 rb 312
  ends

  struct SYSTEM_EXCEPTION_INFORMATION
    Reserved1 rb 16
  ends

  struct SYSTEM_LOOKASIDE_INFORMATION
    Reserved1 rb 32
  ends

  struct SYSTEM_INTERRUPT_INFORMATION
    Reserved1 rb 24
  ends

  struct SYSTEM_POLICY_INFORMATION
    Reserved1 PTR 2 dup (?)
    Reserved2 dd 3 dup (?)
    if PTRBYTES = 8
              dd ?          ; tail padding
    end if
  ends

  struct THREAD_NAME_INFORMATION
    ThreadName UNICODE_STRING
  ends

  struct SYSTEM_CODEINTEGRITY_INFORMATION
    Length               dd ?
    CodeIntegrityOptions dd ?
  ends

  struct PUBLIC_OBJECT_BASIC_INFORMATION
    Attributes    dd ?
    GrantedAccess dd ?
    HandleCount   dd ?
    PointerCount  dd ?
    Reserved      dd 10 dup (?)
  ends

  struct PUBLIC_OBJECT_TYPE_INFORMATION
    TypeName UNICODE_STRING
    Reserved dd 22 dup (?)
  ends

  struct KEY_VALUE_ENTRY
    ValueName  PTR ?
    DataLength dd ?
    DataOffset dd ?
    Type       dd ?
    if PTRBYTES = 8
               dd ?        ; tail padding
    end if
  ends

  struct WINSTATIONINFORMATIONW
    Reserved2 rb 70
              rb 2         ; align LogonId
    LogonId   dd ?
    Reserved3 rb 1140
  ends

  ; Lightweight structures used by the SDK-declared RTL helper exports

  struct RTL_RUN_ONCE
    Ptr PTR ?
  ends

  struct RTL_BARRIER
    Reserved1 dd ?
    Reserved2 dd ?
    Reserved3 PTR 2 dup (?)
    Reserved4 dd ?
    Reserved5 dd ?
  ends

  struct NV_MEMORY_RANGE
    BaseAddress PTR ?
    Length      PTR ?
  ends

  struct CORRELATION_VECTOR
    Version db ?
    Vector  db 129 dup (?)
  ends

  struct in_addr
    S_addr dd ?
  ends

  struct IN_ADDR
    S_addr dd ?
  ends

  struct in6_addr
    Byte db 16 dup (?)
  ends

  struct IN6_ADDR
    Byte db 16 dup (?)
  ends

  struct DL_EUI48
    Byte db 6 dup (?)
  ends

  if PTRBYTES = 8

    struct RUNTIME_FUNCTION
      BeginAddress      dd ?
      EndAddress        dd ?
      UnwindInfoAddress dd ?
    ends

    RUNTIME_FUNCTION.UnwindData = RUNTIME_FUNCTION.UnwindInfoAddress

    struct SCOPE_TABLE
      Count          dd ?
      BeginAddress   dd ?
      EndAddress     dd ?
      HandlerAddress dd ?
      JumpTarget     dd ?
    ends

    struct UNWIND_HISTORY_TABLE_ENTRY
      ImageBase     PTR ?
      FunctionEntry PTR ?
    ends

    struct UNWIND_HISTORY_TABLE
      Count       dd ?
      LocalHint   db ?
      GlobalHint  db ?
      Search      db ?
      Once        db ?
      LowAddress  PTR ?
      HighAddress PTR ?
      EntryData   rb 12 * sizeof.UNWIND_HISTORY_TABLE_ENTRY
    ends

    UNWIND_HISTORY_TABLE.Entry = UNWIND_HISTORY_TABLE.EntryData

    struct KNONVOLATILE_CONTEXT_POINTERS
      FloatingContext PTR 16 dup (?)
      IntegerContext  PTR 16 dup (?)
    ends

    struct DISPATCHER_CONTEXT
      ControlPc        PTR ?
      ImageBase        PTR ?
      FunctionEntry    PTR ?
      EstablisherFrame PTR ?
      TargetIp         PTR ?
      ContextRecord    PTR ?
      LanguageHandler  PTR ?
      HandlerData      PTR ?
      HistoryTable     PTR ?
      ScopeIndex       dd ?
      Fill0            dd ?
    ends

  end if

  break
end iterate

; winternl.h enum values

FileDirectoryInformation = 1

ProcessBasicInformation  = 0
ProcessDebugPort         = 7
ProcessWow64Information  = 26
ProcessImageFileName     = 27
ProcessBreakOnTermination = 29

ThreadIsIoPending        = 16
ThreadNameInformation    = 38

SystemBasicInformation                = 0
SystemPerformanceInformation          = 2
SystemTimeOfDayInformation            = 3
SystemProcessInformation              = 5
SystemProcessorPerformanceInformation = 8
SystemInterruptInformation            = 23
SystemExceptionInformation            = 33
SystemRegistryQuotaInformation        = 37
SystemLookasideInformation            = 45
SystemCodeIntegrityInformation        = 103
SystemPolicyInformation               = 134
SystemBasicProcessInformation         = 252
SystemHandleCountInformation          = 253

ObjectBasicInformation = 0
ObjectTypeInformation  = 2

KeyWriteTimeInformation          = 0
KeyWow64FlagsInformation         = 1
KeyControlFlagsInformation       = 2
KeySetVirtualizationInformation  = 3
KeySetDebugInformation           = 4
KeySetHandleTagsInformation      = 5
MaxKeySetInfoClass               = 6

WinStationInformation = 8

; CodeIntegrityOptions bitmask values

CODEINTEGRITY_OPTION_ENABLED                      = 00000001h
CODEINTEGRITY_OPTION_TESTSIGN                     = 00000002h
CODEINTEGRITY_OPTION_UMCI_ENABLED                 = 00000004h
CODEINTEGRITY_OPTION_UMCI_AUDITMODE_ENABLED       = 00000008h
CODEINTEGRITY_OPTION_UMCI_EXCLUSIONPATHS_ENABLED  = 00000010h
CODEINTEGRITY_OPTION_TEST_BUILD                   = 00000020h
CODEINTEGRITY_OPTION_PREPRODUCTION_BUILD          = 00000040h
CODEINTEGRITY_OPTION_DEBUGMODE_ENABLED            = 00000080h
CODEINTEGRITY_OPTION_FLIGHT_BUILD                 = 00000100h
CODEINTEGRITY_OPTION_FLIGHTING_ENABLED            = 00000200h
CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED            = 00000400h
CODEINTEGRITY_OPTION_HVCI_KMCI_AUDITMODE_ENABLED  = 00000800h
CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED = 00001000h
CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED             = 00002000h

; Object attribute flags

OBJ_INHERIT                       = 00000002h
OBJ_PERMANENT                     = 00000010h
OBJ_EXCLUSIVE                     = 00000020h
OBJ_CASE_INSENSITIVE              = 00000040h
OBJ_OPENIF                        = 00000080h
OBJ_OPENLINK                      = 00000100h
OBJ_KERNEL_HANDLE                 = 00000200h
OBJ_FORCE_ACCESS_CHECK            = 00000400h
OBJ_IGNORE_IMPERSONATED_DEVICEMAP = 00000800h
OBJ_DONT_REPARSE                  = 00001000h
OBJ_VALID_ATTRIBUTES              = 00001FF2h

; NtCreateFile/NtOpenFile disposition and option flags

FILE_SUPERSEDE           = 00000000h
FILE_OPEN                = 00000001h
FILE_CREATE              = 00000002h
FILE_OPEN_IF             = 00000003h
FILE_OVERWRITE           = 00000004h
FILE_OVERWRITE_IF        = 00000005h
FILE_MAXIMUM_DISPOSITION = 00000005h

FILE_DIRECTORY_FILE            = 00000001h
FILE_WRITE_THROUGH             = 00000002h
FILE_SEQUENTIAL_ONLY           = 00000004h
FILE_NO_INTERMEDIATE_BUFFERING = 00000008h
FILE_SYNCHRONOUS_IO_ALERT      = 00000010h
FILE_SYNCHRONOUS_IO_NONALERT   = 00000020h
FILE_NON_DIRECTORY_FILE        = 00000040h
FILE_CREATE_TREE_CONNECTION    = 00000080h
FILE_COMPLETE_IF_OPLOCKED      = 00000100h
FILE_NO_EA_KNOWLEDGE           = 00000200h
FILE_OPEN_REMOTE_INSTANCE      = 00000400h
FILE_RANDOM_ACCESS             = 00000800h
FILE_DELETE_ON_CLOSE           = 00001000h
FILE_OPEN_BY_FILE_ID           = 00002000h
FILE_OPEN_FOR_BACKUP_INTENT    = 00004000h
FILE_NO_COMPRESSION            = 00008000h
FILE_OPEN_REQUIRING_OPLOCK     = 00010000h
FILE_RESERVE_OPFILTER          = 00100000h
FILE_OPEN_REPARSE_POINT        = 00200000h
FILE_OPEN_NO_RECALL            = 00400000h
FILE_OPEN_FOR_FREE_SPACE_QUERY = 00800000h

FILE_VALID_OPTION_FLAGS        = 00FFFFFFh
FILE_VALID_PIPE_OPTION_FLAGS   = 00000032h
FILE_VALID_MAILSLOT_OPTION_FLAGS = 00000032h
FILE_VALID_SET_FLAGS           = 00000036h

FILE_SUPERSEDED     = 00000000h
FILE_OPENED         = 00000001h
FILE_CREATED        = 00000002h
FILE_OVERWRITTEN    = 00000003h
FILE_EXISTS         = 00000004h
FILE_DOES_NOT_EXIST = 00000005h

LOGONID_CURRENT    = -1
SERVERNAME_CURRENT = 0

; RTL helper constants

RTL_RUN_ONCE_CHECK_ONLY       = 00000001h
RTL_RUN_ONCE_ASYNC            = 00000002h
RTL_RUN_ONCE_INIT_FAILED      = 00000004h
RTL_RUN_ONCE_CTX_RESERVED_BITS = 2

RUNTIME_FUNCTION_INDIRECT = 00000001h

UNW_FLAG_NHANDLER    = 00000000h
UNW_FLAG_EHANDLER    = 00000001h
UNW_FLAG_UHANDLER    = 00000002h
UNW_FLAG_CHAININFO   = 00000004h
UNW_FLAG_NO_EPILOGUE = 80000000h
UNWIND_CHAIN_LIMIT   = 32
UNWIND_HISTORY_TABLE_SIZE = 12

FLUSH_NV_MEMORY_IN_FLAG_NO_DRAIN = 00000001h
FLUSH_NV_MEMORY_DEFAULT_TOKEN    = -1
WRITE_NV_MEMORY_FLAG_FLUSH       = 00000001h
WRITE_NV_MEMORY_FLAG_NO_DRAIN    = 00000002h
FILL_NV_MEMORY_FLAG_FLUSH        = 00000001h
FILL_NV_MEMORY_FLAG_NO_DRAIN     = 00000002h

RTL_CORRELATION_VECTOR_STRING_LENGTH  = 129
RTL_CORRELATION_VECTOR_VERSION_1      = 1
RTL_CORRELATION_VECTOR_VERSION_2      = 2
RTL_CORRELATION_VECTOR_VERSION_CURRENT = RTL_CORRELATION_VECTOR_VERSION_2
RTL_CORRELATION_VECTOR_V1_PREFIX_LENGTH = 16
RTL_CORRELATION_VECTOR_V1_LENGTH      = 64
RTL_CORRELATION_VECTOR_V2_PREFIX_LENGTH = 22
RTL_CORRELATION_VECTOR_V2_LENGTH      = 128

OS_DEPLOYMENT_STANDARD = 1
OS_DEPLOYMENT_COMPACT  = 2

RTL_VIRTUAL_UNWIND2_VALIDATE_PAC = 00000001h

GlobalDataIdUnknown                 = 0
GlobalDataIdRngSeedVersion          = 1
GlobalDataIdInterruptTime           = 2
GlobalDataIdTimeZoneBias            = 3
GlobalDataIdImageNumberLow          = 4
GlobalDataIdImageNumberHigh         = 5
GlobalDataIdTimeZoneId              = 6
GlobalDataIdNtMajorVersion          = 7
GlobalDataIdNtMinorVersion          = 8
GlobalDataIdSystemExpirationDate    = 9
GlobalDataIdKdDebuggerEnabled       = 10
GlobalDataIdCyclesPerYield          = 11
GlobalDataIdSafeBootMode            = 12
GlobalDataIdLastSystemRITEventTickCount = 13
GlobalDataIdConsoleSharedDataFlags  = 14
GlobalDataIdNtSystemRootDrive       = 15
GlobalDataIdQpcBypassEnabled        = 16
GlobalDataIdQpcData                 = 17
GlobalDataIdQpcBias                 = 18
