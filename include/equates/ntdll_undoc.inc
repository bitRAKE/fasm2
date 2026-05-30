
; NTDLL.DLL PHNT structures and constants
; Standalone PHNT-derived native surface from systeminformer\phnt\include.
; Do not include this together with equates/ntdll.inc; overlapping names are
; repeated here so this file can be used as its own source model.

iterate <PTR,PTRWORD,PTRBYTES>,\
    dd,dword,4,\
    dq,qword,8

  if x86.mode = 64
    indx 2
  end if

  NTDLL_UNDOC_PTR_SIZE = PTRBYTES

  ; Native pseudo handles

  NtCurrentProcess               = -1
  ZwCurrentProcess               = -1
  NtCurrentThread                = -2
  ZwCurrentThread                = -2
  NtCurrentSession               = -3
  ZwCurrentSession               = -3
  NtCurrentProcessToken          = -4
  NtCurrentThreadToken           = -5
  NtCurrentThreadEffectiveToken  = -6
  NtCurrentSilo                  = -1

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
    ExitStatus                    dd ?
    if PTRBYTES = 8
                                  dd ?        ; align PebBaseAddress
    end if
    PebBaseAddress                PTR ?
    AffinityMask                  PTR ?
    BasePriority                  dd ?
    if PTRBYTES = 8
                                  dd ?        ; align UniqueProcessId
    end if
    UniqueProcessId               PTR ?
    InheritedFromUniqueProcessId  PTR ?
  ends

  struct THREAD_BASIC_INFORMATION
    ExitStatus     dd ?
    if PTRBYTES = 8
                   dd ?        ; align TebBaseAddress
    end if
    TebBaseAddress PTR ?
    ClientId       CLIENT_ID
    AffinityMask   PTR ?
    Priority       dd ?
    BasePriority   dd ?
  ends

  struct SYSTEM_THREAD_INFORMATION
    KernelTime      dq ?
    UserTime        dq ?
    CreateTime      dq ?
    WaitTime        dd ?
    if PTRBYTES = 8
                    dd ?        ; align StartAddress
    end if
    StartAddress    PTR ?
    ClientId        CLIENT_ID
    Priority        dd ?
    BasePriority    dd ?
    ContextSwitches dd ?
    ThreadState     dd ?
    WaitReason      dd ?
                    dd ?        ; tail padding, sizeof aligned to 8
  ends

  struct SYSTEM_PROCESS_INFORMATION
    NextEntryOffset              dd ?
    NumberOfThreads              dd ?
    WorkingSetPrivateSize        dq ?
    HardFaultCount               dd ?
    NumberOfThreadsHighWatermark dd ?
    CycleTime                    dq ?
    CreateTime                   dq ?
    UserTime                     dq ?
    KernelTime                   dq ?
    ImageName                    UNICODE_STRING
    BasePriority                 dd ?
    if PTRBYTES = 8
                                 dd ?        ; align UniqueProcessId
    end if
    UniqueProcessId              PTR ?
    InheritedFromUniqueProcessId PTR ?
    HandleCount                  dd ?
    SessionId                    dd ?
    UniqueProcessKey             PTR ?
    PeakVirtualSize              PTR ?
    VirtualSize                  PTR ?
    PageFaultCount               dd ?
    if PTRBYTES = 8
                                 dd ?        ; align PeakWorkingSetSize
    end if
    PeakWorkingSetSize           PTR ?
    WorkingSetSize               PTR ?
    QuotaPeakPagedPoolUsage      PTR ?
    QuotaPagedPoolUsage          PTR ?
    QuotaPeakNonPagedPoolUsage   PTR ?
    QuotaNonPagedPoolUsage       PTR ?
    PagefileUsage                PTR ?
    PeakPagefileUsage            PTR ?
    PrivatePageCount             PTR ?
    ReadOperationCount           dq ?
    WriteOperationCount          dq ?
    OtherOperationCount          dq ?
    ReadTransferCount            dq ?
    WriteTransferCount           dq ?
    OtherTransferCount           dq ?
    Threads                      SYSTEM_THREAD_INFORMATION
  ends

  struct SYSTEM_EXTENDED_THREAD_INFORMATION
    ThreadInfo        SYSTEM_THREAD_INFORMATION
    StackBase         PTR ?
    StackLimit        PTR ?
    Win32StartAddress PTR ?
    TebBaseAddress    PTR ?
    Reserved2         PTR ?
    Reserved3         PTR ?
    Reserved4         PTR ?
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

  struct FILE_BASIC_INFORMATION
    CreationTime   dq ?
    LastAccessTime dq ?
    LastWriteTime  dq ?
    ChangeTime     dq ?
    FileAttributes dd ?
                  dd ?        ; tail padding, sizeof aligned to 8
  ends

  struct FILE_STANDARD_INFORMATION
    AllocationSize dq ?
    EndOfFile      dq ?
    NumberOfLinks  dd ?
    DeletePending  db ?
    Directory      db ?
                   rb 2       ; tail padding
  ends

  struct FILE_POSITION_INFORMATION
    CurrentByteOffset dq ?
  ends

  struct FILE_NAME_INFORMATION
    FileNameLength dd ?
    FileName       dw 1 dup (?)
                   dw ?        ; tail padding, sizeof aligned to 4
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

  break
end iterate

; Object attribute flags

OBJ_PROTECT_CLOSE                 = 00000001h
OBJ_INHERIT                       = 00000002h
OBJ_AUDIT_OBJECT_CLOSE            = 00000004h
OBJ_NO_RIGHTS_UPGRADE             = 00000008h
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
FILE_DISALLOW_EXCLUSIVE        = 00020000h
FILE_SESSION_AWARE             = 00040000h
FILE_RESERVE_OPFILTER          = 00100000h
FILE_OPEN_REPARSE_POINT        = 00200000h
FILE_OPEN_NO_RECALL            = 00400000h
FILE_OPEN_FOR_FREE_SPACE_QUERY = 00800000h
FILE_CONTAINS_EXTENDED_CREATE_INFORMATION = 10000000h
FILE_VALID_EXTENDED_OPTION_FLAGS = 10000000h

FILE_VALID_OPTION_FLAGS          = 00FFFFFFh
FILE_VALID_PIPE_OPTION_FLAGS     = 00000032h
FILE_VALID_MAILSLOT_OPTION_FLAGS = 00000032h
FILE_VALID_SET_FLAGS             = 00000036h

FILE_SUPERSEDED     = 00000000h
FILE_OPENED         = 00000001h
FILE_CREATED        = 00000002h
FILE_OVERWRITTEN    = 00000003h
FILE_EXISTS         = 00000004h
FILE_DOES_NOT_EXIST = 00000005h

FILE_BYTE_ALIGNMENT     = 00000000h
FILE_WORD_ALIGNMENT     = 00000001h
FILE_LONG_ALIGNMENT     = 00000003h
FILE_QUAD_ALIGNMENT     = 00000007h
FILE_OCTA_ALIGNMENT     = 0000000Fh
FILE_32_BYTE_ALIGNMENT  = 0000001Fh
FILE_64_BYTE_ALIGNMENT  = 0000003Fh
FILE_128_BYTE_ALIGNMENT = 0000007Fh
FILE_256_BYTE_ALIGNMENT = 000000FFh
FILE_512_BYTE_ALIGNMENT = 000001FFh

; Process and thread access/create flags from PHNT

PROCESS_TERMINATE                 = 00000001h
PROCESS_CREATE_THREAD             = 00000002h
PROCESS_SET_SESSIONID             = 00000004h
PROCESS_VM_OPERATION              = 00000008h
PROCESS_VM_READ                   = 00000010h
PROCESS_VM_WRITE                  = 00000020h
PROCESS_DUP_HANDLE                = 00000040h
PROCESS_CREATE_PROCESS            = 00000080h
PROCESS_SET_QUOTA                 = 00000100h
PROCESS_SET_INFORMATION           = 00000200h
PROCESS_QUERY_INFORMATION         = 00000400h
PROCESS_SET_PORT                  = 00000800h
PROCESS_SUSPEND_RESUME            = 00000800h
PROCESS_QUERY_LIMITED_INFORMATION = 00001000h
PROCESS_SET_LIMITED_INFORMATION   = 00002000h

THREAD_TERMINATE                 = 00000001h
THREAD_SUSPEND_RESUME            = 00000002h
THREAD_ALERT                     = 00000004h
THREAD_GET_CONTEXT               = 00000008h
THREAD_SET_CONTEXT               = 00000010h
THREAD_SET_INFORMATION           = 00000020h
THREAD_QUERY_INFORMATION         = 00000040h
THREAD_SET_THREAD_TOKEN          = 00000080h
THREAD_IMPERSONATE               = 00000100h
THREAD_DIRECT_IMPERSONATION      = 00000200h
THREAD_SET_LIMITED_INFORMATION   = 00000400h
THREAD_QUERY_LIMITED_INFORMATION = 00000800h
THREAD_RESUME                    = 00001000h

PROCESS_CREATE_FLAGS_NONE                       = 00000000h
PROCESS_CREATE_FLAGS_BREAKAWAY                  = 00000001h
PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT           = 00000002h
PROCESS_CREATE_FLAGS_INHERIT_HANDLES            = 00000004h
PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE     = 00000008h
PROCESS_CREATE_FLAGS_LARGE_PAGES                = 00000010h
PROCESS_CREATE_FLAGS_LARGE_PAGE_SYSTEM_DLL      = 00000020h
PROCESS_CREATE_FLAGS_PROTECTED_PROCESS          = 00000040h
PROCESS_CREATE_FLAGS_CREATE_SESSION             = 00000080h
PROCESS_CREATE_FLAGS_INHERIT_FROM_PARENT        = 00000100h
PROCESS_CREATE_FLAGS_CREATE_SUSPENDED           = 00000200h
PROCESS_CREATE_FLAGS_FORCE_BREAKAWAY            = 00000400h
PROCESS_CREATE_FLAGS_MINIMAL_PROCESS            = 00000800h
PROCESS_CREATE_FLAGS_RELEASE_SECTION            = 00001000h
PROCESS_CREATE_FLAGS_CLONE_MINIMAL              = 00002000h
PROCESS_CREATE_FLAGS_CLONE_MINIMAL_REDUCED_COMMIT = 00004000h
PROCESS_CREATE_FLAGS_AUXILIARY_PROCESS          = 00008000h
PROCESS_CREATE_FLAGS_CREATE_STORE               = 00020000h
PROCESS_CREATE_FLAGS_USE_PROTECTED_ENVIRONMENT  = 00040000h
PROCESS_CREATE_FLAGS_IMAGE_EXPANSION_MITIGATION_DISABLE = 00080000h
PROCESS_CREATE_FLAGS_PARTITION_CREATE_SLAB_IDENTITY = 00400000h

THREAD_CREATE_FLAGS_NONE                  = 00000000h
THREAD_CREATE_FLAGS_CREATE_SUSPENDED      = 00000001h
THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH    = 00000002h
THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER    = 00000004h
THREAD_CREATE_FLAGS_LOADER_WORKER         = 00000010h
THREAD_CREATE_FLAGS_SKIP_LOADER_INIT      = 00000020h
THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE = 00000040h

; Enum constants generated from PHNT

  ; WAIT_TYPE (phnt_ntdef.h)
  WaitAll = 0
  WaitAny = 1
  WaitNotification = 2
  WaitDequeue = 3
  WaitDpc = 4

  ; KTHREAD_STATE (ntkeapi.h)
  Initialized = 0
  Ready = 1
  Running = 2
  Standby = 3
  Terminated = 4
  Waiting = 5
  Transition = 6
  DeferredReady = 7
  GateWaitObsolete = 8
  WaitingForProcessInSwap = 9
  MaximumThreadState = 10

  ; KWAIT_REASON (ntkeapi.h)
  Executive = 0
  FreePage = 1
  PageIn = 2
  PoolAllocation = 3
  DelayExecution = 4
  Suspended = 5
  UserRequest = 6
  WrExecutive = 7
  WrFreePage = 8
  WrPageIn = 9
  WrPoolAllocation = 10
  WrDelayExecution = 11
  WrSuspended = 12
  WrUserRequest = 13
  WrEventPair = 14
  WrQueue = 15
  WrLpcReceive = 16
  WrLpcReply = 17
  WrVirtualMemory = 18
  WrPageOut = 19
  WrRendezvous = 20
  WrKeyedEvent = 21
  WrTerminated = 22
  WrProcessInSwap = 23
  WrCpuRateControl = 24
  WrCalloutStack = 25
  WrKernel = 26
  WrResource = 27
  WrPushLock = 28
  WrMutex = 29
  WrQuantumEnd = 30
  WrDispatchInt = 31
  WrPreempted = 32
  WrYieldExecution = 33
  WrFastMutex = 34
  WrGuardedMutex = 35
  WrRundown = 36
  WrAlertByThreadId = 37
  WrDeferredPreempt = 38
  WrPhysicalFault = 39
  WrIoRing = 40
  WrMdlCache = 41
  WrRcu = 42
  MaximumWaitReason = 43

  ; PROCESSINFOCLASS (ntpsapi.h)
  ProcessBasicInformation = 0
  ProcessQuotaLimits = 1
  ProcessIoCounters = 2
  ProcessVmCounters = 3
  ProcessTimes = 4
  ProcessBasePriority = 5
  ProcessRaisePriority = 6
  ProcessDebugPort = 7
  ProcessExceptionPort = 8
  ProcessAccessToken = 9
  ProcessLdtInformation = 10
  ProcessLdtSize = 11
  ProcessDefaultHardErrorMode = 12
  ProcessIoPortHandlers = 13
  ProcessPooledUsageAndLimits = 14
  ProcessWorkingSetWatch = 15
  ProcessUserModeIOPL = 16
  ProcessEnableAlignmentFaultFixup = 17
  ProcessPriorityClass = 18
  ProcessWx86Information = 19
  ProcessHandleCount = 20
  ProcessAffinityMask = 21
  ProcessPriorityBoost = 22
  ProcessDeviceMap = 23
  ProcessSessionInformation = 24
  ProcessForegroundInformation = 25
  ProcessWow64Information = 26
  ProcessImageFileName = 27
  ProcessLUIDDeviceMapsEnabled = 28
  ProcessBreakOnTermination = 29
  ProcessDebugObjectHandle = 30
  ProcessDebugFlags = 31
  ProcessHandleTracing = 32
  ProcessIoPriority = 33
  ProcessExecuteFlags = 34
  ProcessTlsInformation = 35
  ProcessCookie = 36
  ProcessImageInformation = 37
  ProcessCycleTime = 38
  ProcessPagePriority = 39
  ProcessInstrumentationCallback = 40
  ProcessThreadStackAllocation = 41
  ProcessWorkingSetWatchEx = 42
  ProcessImageFileNameWin32 = 43
  ProcessImageFileMapping = 44
  ProcessAffinityUpdateMode = 45
  ProcessMemoryAllocationMode = 46
  ProcessGroupInformation = 47
  ProcessTokenVirtualizationEnabled = 48
  ProcessConsoleHostProcess = 49
  ProcessWindowInformation = 50
  ProcessHandleInformation = 51
  ProcessMitigationPolicy = 52
  ProcessDynamicFunctionTableInformation = 53
  ProcessHandleCheckingMode = 54
  ProcessKeepAliveCount = 55
  ProcessRevokeFileHandles = 56
  ProcessWorkingSetControl = 57
  ProcessHandleTable = 58
  ProcessCheckStackExtentsMode = 59
  ProcessCommandLineInformation = 60
  ProcessProtectionInformation = 61
  ProcessMemoryExhaustion = 62
  ProcessFaultInformation = 63
  ProcessTelemetryIdInformation = 64
  ProcessCommitReleaseInformation = 65
  ProcessDefaultCpuSetsInformation = 66
  ProcessAllowedCpuSetsInformation = 67
  ProcessSubsystemProcess = 68
  ProcessJobMemoryInformation = 69
  ProcessInPrivate = 70
  ProcessRaiseUMExceptionOnInvalidHandleClose = 71
  ProcessIumChallengeResponse = 72
  ProcessChildProcessInformation = 73
  ProcessHighGraphicsPriorityInformation = 74
  ProcessSubsystemInformation = 75
  ProcessEnergyValues = 76
  ProcessPowerThrottlingState = 77
  ProcessActivityThrottlePolicy = 78
  ProcessWin32kSyscallFilterInformation = 79
  ProcessDisableSystemAllowedCpuSets = 80
  ProcessWakeInformation = 81
  ProcessEnergyTrackingState = 82
  ProcessManageWritesToExecutableMemory = 83
  ProcessCaptureTrustletLiveDump = 84
  ProcessTelemetryCoverage = 85
  ProcessEnclaveInformation = 86
  ProcessEnableReadWriteVmLogging = 87
  ProcessUptimeInformation = 88
  ProcessImageSection = 89
  ProcessDebugAuthInformation = 90
  ProcessSystemResourceManagement = 91
  ProcessSequenceNumber = 92
  ProcessLoaderDetour = 93
  ProcessSecurityDomainInformation = 94
  ProcessCombineSecurityDomainsInformation = 95
  ProcessEnableLogging = 96
  ProcessLeapSecondInformation = 97
  ProcessFiberShadowStackAllocation = 98
  ProcessFreeFiberShadowStackAllocation = 99
  ProcessAltSystemCallInformation = 100
  ProcessDynamicEHContinuationTargets = 101
  ProcessDynamicEnforcedCetCompatibleRanges = 102
  ProcessCreateStateChange = 103
  ProcessApplyStateChange = 104
  ProcessEnableOptionalXStateFeatures = 105
  ProcessAltPrefetchParam = 106
  ProcessAssignCpuPartitions = 107
  ProcessPriorityClassEx = 108
  ProcessMembershipInformation = 109
  ProcessEffectiveIoPriority = 110
  ProcessEffectivePagePriority = 111
  ProcessSchedulerSharedData = 112
  ProcessSlistRollbackInformation = 113
  ProcessNetworkIoCounters = 114
  ProcessFindFirstThreadByTebValue = 115
  ProcessEnclaveAddressSpaceRestriction = 116
  ProcessAvailableCpus = 117
  MaxProcessInfoClass = 118

  ; THREADINFOCLASS (ntpsapi.h)
  ThreadBasicInformation = 0
  ThreadTimes = 1
  ThreadPriority = 2
  ThreadBasePriority = 3
  ThreadAffinityMask = 4
  ThreadImpersonationToken = 5
  ThreadDescriptorTableEntry = 6
  ThreadEnableAlignmentFaultFixup = 7
  ThreadEventPair = 8
  ThreadQuerySetWin32StartAddress = 9
  ThreadZeroTlsCell = 10
  ThreadPerformanceCount = 11
  ThreadAmILastThread = 12
  ThreadIdealProcessor = 13
  ThreadPriorityBoost = 14
  ThreadSetTlsArrayAddress = 15
  ThreadIsIoPending = 16
  ThreadHideFromDebugger = 17
  ThreadBreakOnTermination = 18
  ThreadSwitchLegacyState = 19
  ThreadIsTerminated = 20
  ThreadLastSystemCall = 21
  ThreadIoPriority = 22
  ThreadCycleTime = 23
  ThreadPagePriority = 24
  ThreadActualBasePriority = 25
  ThreadTebInformation = 26
  ThreadCSwitchMon = 27
  ThreadCSwitchPmu = 28
  ThreadWow64Context = 29
  ThreadGroupInformation = 30
  ThreadUmsInformation = 31
  ThreadCounterProfiling = 32
  ThreadIdealProcessorEx = 33
  ThreadCpuAccountingInformation = 34
  ThreadSuspendCount = 35
  ThreadHeterogeneousCpuPolicy = 36
  ThreadContainerId = 37
  ThreadNameInformation = 38
  ThreadSelectedCpuSets = 39
  ThreadSystemThreadInformation = 40
  ThreadActualGroupAffinity = 41
  ThreadDynamicCodePolicyInfo = 42
  ThreadExplicitCaseSensitivity = 43
  ThreadWorkOnBehalfTicket = 44
  ThreadSubsystemInformation = 45
  ThreadDbgkWerReportActive = 46
  ThreadAttachContainer = 47
  ThreadManageWritesToExecutableMemory = 48
  ThreadPowerThrottlingState = 49
  ThreadWorkloadClass = 50
  ThreadCreateStateChange = 51
  ThreadApplyStateChange = 52
  ThreadStrongerBadHandleChecks = 53
  ThreadEffectiveIoPriority = 54
  ThreadEffectivePagePriority = 55
  ThreadUpdateLockOwnership = 56
  ThreadSchedulerSharedDataSlot = 57
  ThreadTebInformationAtomic = 58
  ThreadIndexInformation = 59
  MaxThreadInfoClass = 60

  ; SYSTEM_INFORMATION_CLASS (ntexapi.h)
  SystemBasicInformation = 0
  SystemProcessorInformation = 1
  SystemPerformanceInformation = 2
  SystemTimeOfDayInformation = 3
  SystemPathInformation = 4
  SystemProcessInformation = 5
  SystemCallCountInformation = 6
  SystemDeviceInformation = 7
  SystemProcessorPerformanceInformation = 8
  SystemFlagsInformation = 9
  SystemCallTimeInformation = 10
  SystemModuleInformation = 11
  SystemLocksInformation = 12
  SystemStackTraceInformation = 13
  SystemPagedPoolInformation = 14
  SystemNonPagedPoolInformation = 15
  SystemHandleInformation = 16
  SystemObjectInformation = 17
  SystemPageFileInformation = 18
  SystemVdmInstemulInformation = 19
  SystemVdmBopInformation = 20
  SystemFileCacheInformation = 21
  SystemPoolTagInformation = 22
  SystemInterruptInformation = 23
  SystemDpcBehaviorInformation = 24
  SystemFullMemoryInformation = 25
  SystemLoadGdiDriverInformation = 26
  SystemUnloadGdiDriverInformation = 27
  SystemTimeAdjustmentInformation = 28
  SystemSummaryMemoryInformation = 29
  SystemMirrorMemoryInformation = 30
  SystemPerformanceTraceInformation = 31
  SystemObsolete0 = 32
  SystemExceptionInformation = 33
  SystemCrashDumpStateInformation = 34
  SystemKernelDebuggerInformation = 35
  SystemContextSwitchInformation = 36
  SystemRegistryQuotaInformation = 37
  SystemExtendServiceTableInformation = 38
  SystemPrioritySeparation = 39
  SystemVerifierAddDriverInformation = 40
  SystemVerifierRemoveDriverInformation = 41
  SystemProcessorIdleInformation = 42
  SystemLegacyDriverInformation = 43
  SystemCurrentTimeZoneInformation = 44
  SystemLookasideInformation = 45
  SystemTimeSlipNotification = 46
  SystemSessionCreate = 47
  SystemSessionDetach = 48
  SystemSessionInformation = 49
  SystemRangeStartInformation = 50
  SystemVerifierInformation = 51
  SystemVerifierThunkExtend = 52
  SystemSessionProcessInformation = 53
  SystemLoadGdiDriverInSystemSpace = 54
  SystemNumaProcessorMap = 55
  SystemPrefetcherInformation = 56
  SystemExtendedProcessInformation = 57
  SystemRecommendedSharedDataAlignment = 58
  SystemComPlusPackage = 59
  SystemNumaAvailableMemory = 60
  SystemProcessorPowerInformation = 61
  SystemEmulationBasicInformation = 62
  SystemEmulationProcessorInformation = 63
  SystemExtendedHandleInformation = 64
  SystemLostDelayedWriteInformation = 65
  SystemBigPoolInformation = 66
  SystemSessionPoolTagInformation = 67
  SystemSessionMappedViewInformation = 68
  SystemHotpatchInformation = 69
  SystemObjectSecurityMode = 70
  SystemWatchdogTimerHandler = 71
  SystemWatchdogTimerInformation = 72
  SystemLogicalProcessorInformation = 73
  SystemWow64SharedInformationObsolete = 74
  SystemRegisterFirmwareTableInformationHandler = 75
  SystemFirmwareTableInformation = 76
  SystemModuleInformationEx = 77
  SystemVerifierTriageInformation = 78
  SystemSuperfetchInformation = 79
  SystemMemoryListInformation = 80
  SystemFileCacheInformationEx = 81
  SystemThreadPriorityClientIdInformation = 82
  SystemProcessorIdleCycleTimeInformation = 83
  SystemVerifierCancellationInformation = 84
  SystemProcessorPowerInformationEx = 85
  SystemRefTraceInformation = 86
  SystemSpecialPoolInformation = 87
  SystemProcessIdInformation = 88
  SystemErrorPortInformation = 89
  SystemBootEnvironmentInformation = 90
  SystemHypervisorInformation = 91
  SystemVerifierInformationEx = 92
  SystemTimeZoneInformation = 93
  SystemImageFileExecutionOptionsInformation = 94
  SystemCoverageInformation = 95
  SystemPrefetchPatchInformation = 96
  SystemVerifierFaultsInformation = 97
  SystemSystemPartitionInformation = 98
  SystemSystemDiskInformation = 99
  SystemProcessorPerformanceDistribution = 100
  SystemNumaProximityNodeInformation = 101
  SystemDynamicTimeZoneInformation = 102
  SystemCodeIntegrityInformation = 103
  SystemProcessorMicrocodeUpdateInformation = 104
  SystemProcessorBrandString = 105
  SystemVirtualAddressInformation = 106
  SystemLogicalProcessorAndGroupInformation = 107
  SystemProcessorCycleTimeInformation = 108
  SystemStoreInformation = 109
  SystemRegistryAppendString = 110
  SystemAitSamplingValue = 111
  SystemVhdBootInformation = 112
  SystemCpuQuotaInformation = 113
  SystemNativeBasicInformation = 114
  SystemErrorPortTimeouts = 115
  SystemLowPriorityIoInformation = 116
  SystemTpmBootEntropyInformation = 117
  SystemVerifierCountersInformation = 118
  SystemPagedPoolInformationEx = 119
  SystemSystemPtesInformationEx = 120
  SystemNodeDistanceInformation = 121
  SystemAcpiAuditInformation = 122
  SystemBasicPerformanceInformation = 123
  SystemQueryPerformanceCounterInformation = 124
  SystemSessionBigPoolInformation = 125
  SystemBootGraphicsInformation = 126
  SystemScrubPhysicalMemoryInformation = 127
  SystemBadPageInformation = 128
  SystemProcessorProfileControlArea = 129
  SystemCombinePhysicalMemoryInformation = 130
  SystemEntropyInterruptTimingInformation = 131
  SystemConsoleInformation = 132
  SystemPlatformBinaryInformation = 133
  SystemPolicyInformation = 134
  SystemHypervisorProcessorCountInformation = 135
  SystemDeviceDataInformation = 136
  SystemDeviceDataEnumerationInformation = 137
  SystemMemoryTopologyInformation = 138
  SystemMemoryChannelInformation = 139
  SystemBootLogoInformation = 140
  SystemProcessorPerformanceInformationEx = 141
  SystemCriticalProcessErrorLogInformation = 142
  SystemSecureBootPolicyInformation = 143
  SystemPageFileInformationEx = 144
  SystemSecureBootInformation = 145
  SystemEntropyInterruptTimingRawInformation = 146
  SystemPortableWorkspaceEfiLauncherInformation = 147
  SystemFullProcessInformation = 148
  SystemKernelDebuggerInformationEx = 149
  SystemBootMetadataInformation = 150
  SystemSoftRebootInformation = 151
  SystemElamCertificateInformation = 152
  SystemOfflineDumpConfigInformation = 153
  SystemProcessorFeaturesInformation = 154
  SystemRegistryReconciliationInformation = 155
  SystemEdidInformation = 156
  SystemManufacturingInformation = 157
  SystemEnergyEstimationConfigInformation = 158
  SystemHypervisorDetailInformation = 159
  SystemProcessorCycleStatsInformation = 160
  SystemVmGenerationCountInformation = 161
  SystemTrustedPlatformModuleInformation = 162
  SystemKernelDebuggerFlags = 163
  SystemCodeIntegrityPolicyInformation = 164
  SystemIsolatedUserModeInformation = 165
  SystemHardwareSecurityTestInterfaceResultsInformation = 166
  SystemSingleModuleInformation = 167
  SystemAllowedCpuSetsInformation = 168
  SystemVsmProtectionInformation = 169
  SystemInterruptCpuSetsInformation = 170
  SystemSecureBootPolicyFullInformation = 171
  SystemCodeIntegrityPolicyFullInformation = 172
  SystemAffinitizedInterruptProcessorInformation = 173
  SystemRootSiloInformation = 174
  SystemCpuSetInformation = 175
  SystemCpuSetTagInformation = 176
  SystemWin32WerStartCallout = 177
  SystemSecureKernelProfileInformation = 178
  SystemCodeIntegrityPlatformManifestInformation = 179
  SystemInterruptSteeringInformation = 180
  SystemSupportedProcessorArchitectures = 181
  SystemMemoryUsageInformation = 182
  SystemCodeIntegrityCertificateInformation = 183
  SystemPhysicalMemoryInformation = 184
  SystemControlFlowTransition = 185
  SystemKernelDebuggingAllowed = 186
  SystemActivityModerationExeState = 187
  SystemActivityModerationUserSettings = 188
  SystemCodeIntegrityPoliciesFullInformation = 189
  SystemCodeIntegrityUnlockInformation = 190
  SystemIntegrityQuotaInformation = 191
  SystemFlushInformation = 192
  SystemProcessorIdleMaskInformation = 193
  SystemSecureDumpEncryptionInformation = 194
  SystemWriteConstraintInformation = 195
  SystemKernelVaShadowInformation = 196
  SystemHypervisorSharedPageInformation = 197
  SystemFirmwareBootPerformanceInformation = 198
  SystemCodeIntegrityVerificationInformation = 199
  SystemFirmwarePartitionInformation = 200
  SystemSpeculationControlInformation = 201
  SystemDmaGuardPolicyInformation = 202
  SystemEnclaveLaunchControlInformation = 203
  SystemWorkloadAllowedCpuSetsInformation = 204
  SystemCodeIntegrityUnlockModeInformation = 205
  SystemLeapSecondInformation = 206
  SystemFlags2Information = 207
  SystemSecurityModelInformation = 208
  SystemCodeIntegritySyntheticCacheInformation = 209
  SystemFeatureConfigurationInformation = 210
  SystemFeatureConfigurationSectionInformation = 211
  SystemFeatureUsageSubscriptionInformation = 212
  SystemSecureSpeculationControlInformation = 213
  SystemSpacesBootInformation = 214
  SystemFwRamdiskInformation = 215
  SystemWheaIpmiHardwareInformation = 216
  SystemDifSetRuleClassInformation = 217
  SystemDifClearRuleClassInformation = 218
  SystemDifApplyPluginVerificationOnDriver = 219
  SystemDifRemovePluginVerificationOnDriver = 220
  SystemShadowStackInformation = 221
  SystemBuildVersionInformation = 222
  SystemPoolLimitInformation = 223
  SystemCodeIntegrityAddDynamicStore = 224
  SystemCodeIntegrityClearDynamicStores = 225
  SystemDifPoolTrackingInformation = 226
  SystemPoolZeroingInformation = 227
  SystemDpcWatchdogInformation = 228
  SystemDpcWatchdogInformation2 = 229
  SystemSupportedProcessorArchitectures2 = 230
  SystemSingleProcessorRelationshipInformation = 231
  SystemXfgCheckFailureInformation = 232
  SystemIommuStateInformation = 233
  SystemHypervisorMinrootInformation = 234
  SystemHypervisorBootPagesInformation = 235
  SystemPointerAuthInformation = 236
  SystemSecureKernelDebuggerInformation = 237
  SystemOriginalImageFeatureInformation = 238
  SystemMemoryNumaInformation = 239
  SystemMemoryNumaPerformanceInformation = 240
  SystemCodeIntegritySignedPoliciesFullInformation = 241
  SystemSecureCoreInformation = 242
  SystemTrustedAppsRuntimeInformation = 243
  SystemBadPageInformationEx = 244
  SystemResourceDeadlockTimeout = 245
  SystemBreakOnContextUnwindFailureInformation = 246
  SystemOslRamdiskInformation = 247
  SystemCodeIntegrityPolicyManagementInformation = 248
  SystemMemoryNumaCacheInformation = 249
  SystemProcessorFeaturesBitMapInformation = 250
  SystemRefTraceInformationEx = 251
  SystemBasicProcessInformation = 252
  SystemHandleCountInformation = 253
  SystemRuntimeAttestationReport = 254
  SystemPoolTagInformation2 = 255
  SystemCodeIntegrityEndpointSecurityInformation = 256
  MaxSystemInfoClass = 257

  ; FILE_INFORMATION_CLASS (ntioapi.h)
  FileDirectoryInformation = 1
  FileFullDirectoryInformation = 2
  FileBothDirectoryInformation = 3
  FileBasicInformation = 4
  FileStandardInformation = 5
  FileInternalInformation = 6
  FileEaInformation = 7
  FileAccessInformation = 8
  FileNameInformation = 9
  FileRenameInformation = 10
  FileLinkInformation = 11
  FileNamesInformation = 12
  FileDispositionInformation = 13
  FilePositionInformation = 14
  FileFullEaInformation = 15
  FileModeInformation = 16
  FileAlignmentInformation = 17
  FileAllInformation = 18
  FileAllocationInformation = 19
  FileEndOfFileInformation = 20
  FileAlternateNameInformation = 21
  FileStreamInformation = 22
  FilePipeInformation = 23
  FilePipeLocalInformation = 24
  FilePipeRemoteInformation = 25
  FileMailslotQueryInformation = 26
  FileMailslotSetInformation = 27
  FileCompressionInformation = 28
  FileObjectIdInformation = 29
  FileCompletionInformation = 30
  FileMoveClusterInformation = 31
  FileQuotaInformation = 32
  FileReparsePointInformation = 33
  FileNetworkOpenInformation = 34
  FileAttributeTagInformation = 35
  FileTrackingInformation = 36
  FileIdBothDirectoryInformation = 37
  FileIdFullDirectoryInformation = 38
  FileValidDataLengthInformation = 39
  FileShortNameInformation = 40
  FileIoCompletionNotificationInformation = 41
  FileIoStatusBlockRangeInformation = 42
  FileIoPriorityHintInformation = 43
  FileSfioReserveInformation = 44
  FileSfioVolumeInformation = 45
  FileHardLinkInformation = 46
  FileProcessIdsUsingFileInformation = 47
  FileNormalizedNameInformation = 48
  FileNetworkPhysicalNameInformation = 49
  FileIdGlobalTxDirectoryInformation = 50
  FileIsRemoteDeviceInformation = 51
  FileUnusedInformation = 52
  FileNumaNodeInformation = 53
  FileStandardLinkInformation = 54
  FileRemoteProtocolInformation = 55
  FileRenameInformationBypassAccessCheck = 56
  FileLinkInformationBypassAccessCheck = 57
  FileVolumeNameInformation = 58
  FileIdInformation = 59
  FileIdExtdDirectoryInformation = 60
  FileReplaceCompletionInformation = 61
  FileHardLinkFullIdInformation = 62
  FileIdExtdBothDirectoryInformation = 63
  FileDispositionInformationEx = 64
  FileRenameInformationEx = 65
  FileRenameInformationExBypassAccessCheck = 66
  FileDesiredStorageClassInformation = 67
  FileStatInformation = 68
  FileMemoryPartitionInformation = 69
  FileStatLxInformation = 70
  FileCaseSensitiveInformation = 71
  FileLinkInformationEx = 72
  FileLinkInformationExBypassAccessCheck = 73
  FileStorageReserveIdInformation = 74
  FileCaseSensitiveInformationForceAccessCheck = 75
  FileKnownFolderInformation = 76
  FileStatBasicInformation = 77
  FileId64ExtdDirectoryInformation = 78
  FileId64ExtdBothDirectoryInformation = 79
  FileIdAllExtdDirectoryInformation = 80
  FileIdAllExtdBothDirectoryInformation = 81
  FileStreamReservationInformation = 82
  FileMupProviderInfo = 83
  FileMaximumInformation = 84

  ; FSINFOCLASS (ntioapi.h)
  FileFsVolumeInformation = 1
  FileFsLabelInformation = 2
  FileFsSizeInformation = 3
  FileFsDeviceInformation = 4
  FileFsAttributeInformation = 5
  FileFsControlInformation = 6
  FileFsFullSizeInformation = 7
  FileFsObjectIdInformation = 8
  FileFsDriverPathInformation = 9
  FileFsVolumeFlagsInformation = 10
  FileFsSectorSizeInformation = 11
  FileFsDataCopyInformation = 12
  FileFsMetadataSizeInformation = 13
  FileFsFullSizeInformationEx = 14
  FileFsGuidInformation = 15
  FileFsMaximumInformation = 16

  ; DIRECTORY_NOTIFY_INFORMATION_CLASS (ntioapi.h)
  DirectoryNotifyInformation = 1
  DirectoryNotifyExtendedInformation = 2
  DirectoryNotifyFullInformation = 3
  DirectoryNotifyMaximumInformation = 4

  ; IO_COMPLETION_INFORMATION_CLASS (ntioapi.h)
  IoCompletionBasicInformation = 0

  ; OBJECT_INFORMATION_CLASS (ntobapi.h)
  ObjectBasicInformation = 0
  ObjectNameInformation = 1
  ObjectTypeInformation = 2
  ObjectTypesInformation = 3
  ObjectHandleFlagInformation = 4
  ObjectSessionInformation = 5
  ObjectSessionObjectInformation = 6
  ObjectSetRefTraceInformation = 7
  MaxObjectInfoClass = 8

  ; MEMORY_INFORMATION_CLASS (ntmmapi.h)
  MemoryBasicInformation = 0
  MemoryWorkingSetInformation = 1
  MemoryMappedFilenameInformation = 2
  MemoryRegionInformation = 3
  MemoryWorkingSetExInformation = 4
  MemorySharedCommitInformation = 5
  MemoryImageInformation = 6
  MemoryRegionInformationEx = 7
  MemoryPrivilegedBasicInformation = 8
  MemoryEnclaveImageInformation = 9
  MemoryBasicInformationCapped = 10
  MemoryPhysicalContiguityInformation = 11
  MemoryBadInformation = 12
  MemoryBadInformationAllProcesses = 13
  MemoryImageExtensionInformation = 14
  MaxMemoryInfoClass = 15

  ; VIRTUAL_MEMORY_INFORMATION_CLASS (ntmmapi.h)
  VmPrefetchInformation = 0
  VmPagePriorityInformation = 1
  VmCfgCallTargetInformation = 2
  VmPageDirtyStateInformation = 3
  VmImageHotPatchInformation = 4
  VmPhysicalContiguityInformation = 5
  VmVirtualMachinePrepopulateInformation = 6
  VmRemoveFromWorkingSetInformation = 7
  MaxVmInfoClass = 8

  ; SECTION_INFORMATION_CLASS (ntmmapi.h)
  SectionBasicInformation = 0
  SectionImageInformation = 1
  SectionRelocationInformation = 2
  SectionOriginalBaseInformation = 3
  SectionInternalImageInformation = 4
  MaxSectionInfoClass = 5

  ; PARTITION_INFORMATION_CLASS (ntmmapi.h)
  SystemMemoryPartitionInformation = 0
  SystemMemoryPartitionMoveMemory = 1
  SystemMemoryPartitionAddPagefile = 2
  SystemMemoryPartitionCombineMemory = 3
  SystemMemoryPartitionInitialAddMemory = 4
  SystemMemoryPartitionGetMemoryEvents = 5
  SystemMemoryPartitionSetAttributes = 6
  SystemMemoryPartitionNodeInformation = 7
  SystemMemoryPartitionCreateLargePages = 8
  SystemMemoryPartitionDedicatedMemoryInformation = 9
  SystemMemoryPartitionOpenDedicatedMemory = 10
  SystemMemoryPartitionMemoryChargeAttributes = 11
  SystemMemoryPartitionClearAttributes = 12
  SystemMemoryPartitionSetMemoryThresholds = 13
  SystemMemoryPartitionMemoryListCommand = 14
  SystemMemoryPartitionMax = 15

  ; KEY_INFORMATION_CLASS (ntregapi.h)
  KeyBasicInformation = 0
  KeyNodeInformation = 1
  KeyFullInformation = 2
  KeyNameInformation = 3
  KeyCachedInformation = 4
  KeyFlagsInformation = 5
  KeyVirtualizationInformation = 6
  KeyHandleTagsInformation = 7
  KeyTrustInformation = 8
  KeyLayerInformation = 9
  MaxKeyInfoClass = 10

  ; KEY_SET_INFORMATION_CLASS (ntregapi.h)
  KeyWriteTimeInformation = 0
  KeyWow64FlagsInformation = 1
  KeyControlFlagsInformation = 2
  KeySetVirtualizationInformation = 3
  KeySetDebugInformation = 4
  KeySetHandleTagsInformation = 5
  KeySetLayerInformation = 6
  MaxKeySetInfoClass = 7

  ; KEY_VALUE_INFORMATION_CLASS (ntregapi.h)
  KeyValueBasicInformation = 0
  KeyValueFullInformation = 1
  KeyValuePartialInformation = 2
  KeyValueFullInformationAlign64 = 3
  KeyValuePartialInformationAlign64 = 4
  KeyValueLayerInformation = 5
  MaxKeyValueInfoClass = 6

  ; TOKEN_INFORMATION_CLASS (ntseapi.h)
  TokenUser = 1
  TokenGroups = 2
  TokenPrivileges = 3
  TokenOwner = 4
  TokenPrimaryGroup = 5
  TokenDefaultDacl = 6
  TokenSource = 7
  TokenType = 8
  TokenImpersonationLevel = 9
  TokenStatistics = 10
  TokenRestrictedSids = 11
  TokenSessionId = 12
  TokenGroupsAndPrivileges = 13
  TokenSessionReference = 14
  TokenSandBoxInert = 15
  TokenAuditPolicy = 16
  TokenOrigin = 17
  TokenElevationType = 18
  TokenLinkedToken = 19
  TokenElevation = 20
  TokenHasRestrictions = 21
  TokenAccessInformation = 22
  TokenVirtualizationAllowed = 23
  TokenVirtualizationEnabled = 24
  TokenIntegrityLevel = 25
  TokenUIAccess = 26
  TokenMandatoryPolicy = 27
  TokenLogonSid = 28
  TokenIsAppContainer = 29
  TokenCapabilities = 30
  TokenAppContainerSid = 31
  TokenAppContainerNumber = 32
  TokenUserClaimAttributes = 33
  TokenDeviceClaimAttributes = 34
  TokenRestrictedUserClaimAttributes = 35
  TokenRestrictedDeviceClaimAttributes = 36
  TokenDeviceGroups = 37
  TokenRestrictedDeviceGroups = 38
  TokenSecurityAttributes = 39
  TokenIsRestricted = 40
  TokenProcessTrustLevel = 41
  TokenPrivateNameSpace = 42
  TokenSingletonAttributes = 43
  TokenBnoIsolation = 44
  TokenChildProcessFlags = 45
  TokenIsLessPrivilegedAppContainer = 46
  TokenIsSandboxed = 47
  TokenIsAppSilo = 48
  TokenLoggingInformation = 49
  TokenLearningMode = 50
  MaxTokenInfoClass = 51

  ; PORT_INFORMATION_CLASS (ntlpcapi.h)
  PortBasicInformation = 0
  PortDumpInformation = 1

  ; ALPC_PORT_INFORMATION_CLASS (ntlpcapi.h)
  AlpcBasicInformation = 0
  AlpcPortInformation = 1
  AlpcAssociateCompletionPortInformation = 2
  AlpcConnectedSIDInformation = 3
  AlpcServerInformation = 4
  AlpcMessageZoneInformation = 5
  AlpcRegisterCompletionListInformation = 6
  AlpcUnregisterCompletionListInformation = 7
  AlpcAdjustCompletionListConcurrencyCountInformation = 8
  AlpcRegisterCallbackInformation = 9
  AlpcCompletionListRundownInformation = 10
  AlpcWaitForPortReferences = 11
  AlpcServerSessionInformation = 12

  ; ALPC_MESSAGE_INFORMATION_CLASS (ntlpcapi.h)
  AlpcMessageSidInformation = 0
  AlpcMessageTokenModifiedIdInformation = 1
  AlpcMessageDirectStatusInformation = 2
  AlpcMessageHandleInformation = 3
  MaxAlpcMessageInfoClass = 4

  ; DEBUGOBJECTINFOCLASS (ntdbg.h)
  DebugObjectUnusedInformation = 0
  DebugObjectKillProcessOnExitInformation = 1
  MaxDebugObjectInfoClass = 2

  ; EVENT_INFORMATION_CLASS (ntexapi.h)
  EventBasicInformation = 0

  ; MUTANT_INFORMATION_CLASS (ntexapi.h)
  MutantBasicInformation = 0
  MutantOwnerInformation = 1

  ; SEMAPHORE_INFORMATION_CLASS (ntexapi.h)
  SemaphoreBasicInformation = 0

  ; TIMER_INFORMATION_CLASS (ntexapi.h)
  TimerBasicInformation = 0

  ; TIMER_SET_INFORMATION_CLASS (ntexapi.h)
  TimerSetCoalescableTimer = 0
  MaxTimerInfoClass = 1

  ; WORKERFACTORYINFOCLASS (ntexapi.h)
  WorkerFactoryTimeout = 0
  WorkerFactoryRetryTimeout = 1
  WorkerFactoryIdleTimeout = 2
  WorkerFactoryBindingCount = 3
  WorkerFactoryThreadMinimum = 4
  WorkerFactoryThreadMaximum = 5
  WorkerFactoryPaused = 6
  WorkerFactoryBasicInformation = 7
  WorkerFactoryAdjustThreadGoal = 8
  WorkerFactoryCallbackType = 9
  WorkerFactoryStackInformation = 10
  WorkerFactoryThreadBasePriority = 11
  WorkerFactoryTimeoutWaiters = 12
  WorkerFactoryFlags = 13
  WorkerFactoryThreadSoftMaximum = 14
  WorkerFactoryThreadCpuSets = 15
  MaxWorkerFactoryInfoClass = 16

  ; EVENT_TRACE_INFORMATION_CLASS (ntexapi.h)
  EventTraceKernelVersionInformation = 0
  EventTraceGroupMaskInformation = 1
  EventTracePerformanceInformation = 2
  EventTraceTimeProfileInformation = 3
  EventTraceSessionSecurityInformation = 4
  EventTraceSpinlockInformation = 5
  EventTraceStackTracingInformation = 6
  EventTraceExecutiveResourceInformation = 7
  EventTraceHeapTracingInformation = 8
  EventTraceHeapSummaryTracingInformation = 9
  EventTracePoolTagFilterInformation = 10
  EventTracePebsTracingInformation = 11
  EventTraceProfileConfigInformation = 12
  EventTraceProfileSourceListInformation = 13
  EventTraceProfileEventListInformation = 14
  EventTraceProfileCounterListInformation = 15
  EventTraceStackCachingInformation = 16
  EventTraceObjectTypeFilterInformation = 17
  EventTraceSoftRestartInformation = 18
  EventTraceLastBranchConfigurationInformation = 19
  EventTraceLastBranchEventListInformation = 20
  EventTraceProfileSourceAddInformation = 21
  EventTraceProfileSourceRemoveInformation = 22
  EventTraceProcessorTraceConfigurationInformation = 23
  EventTraceProcessorTraceEventListInformation = 24
  EventTraceCoverageSamplerInformation = 25
  EventTraceUnifiedStackCachingInformation = 26
  EventTraceContextRegisterTraceInformation = 27
  MaxEventTraceInfoClass = 28

  ; WATCHDOG_INFORMATION_CLASS (ntexapi.h)
  WdInfoTimeoutValue = 0
  WdInfoResetTimer = 1
  WdInfoStopTimer = 2
  WdInfoStartTimer = 3
  WdInfoTriggerAction = 4
  WdInfoState = 5
  WdInfoTriggerReset = 6
  WdInfoNop = 7
  WdInfoGeneratedLastReset = 8
  WdInfoInvalid = 9

  ; STORE_INFORMATION_CLASS (ntexapi.h)
  StorePageRequest = 1
  StoreStatsRequest = 2
  StoreCreateRequest = 3
  StoreDeleteRequest = 4
  StoreListRequest = 5
  StoreEmptyRequest = 7
  CacheListRequest = 8
  CacheCreateRequest = 9
  CacheDeleteRequest = 10
  CacheStoreCreateRequest = 11
  CacheStoreDeleteRequest = 12
  CacheStatsRequest = 13
  RegistrationRequest = 15
  GlobalCacheStatsRequest = 16
  StoreResizeRequest = 17
  CacheStoreResizeRequest = 18
  SmConfigRequest = 19
  StoreHighMemoryPriorityRequest = 20
  SystemStoreTrimRequest = 21
  MemCompressionInfoRequest = 22
  StoreExistsForProcess = 23
  CompressionReadStatsRequest = 24
  CompressionAcceleratorRequest = 25
  StoreInformationMax = 26

  ; ATOM_INFORMATION_CLASS (ntexapi.h)
  AtomBasicInformation = 0
  AtomTableInformation = 1

  ; HOT_PATCH_INFORMATION_CLASS (ntexapi.h)
  ManageHotPatchLoadPatch = 0
  ManageHotPatchUnloadPatch = 1
  ManageHotPatchQueryPatches = 2
  ManageHotPatchLoadPatchForUser = 3
  ManageHotPatchUnloadPatchForUser = 4
  ManageHotPatchQueryPatchesForUser = 5
  ManageHotPatchQueryActivePatches = 6
  ManageHotPatchApplyImagePatch = 7
  ManageHotPatchQuerySinglePatch = 8
  ManageHotPatchCheckEnabled = 9
  ManageHotPatchCreatePatchSection = 10
  ManageHotPatchMax = 11

  ; PREFETCHER_INFORMATION_CLASS (ntpfapi.h)
  PrefetcherRetrieveTrace = 1
  PrefetcherSystemParameters = 2
  PrefetcherBootPhase = 3
  PrefetcherSpare1 = 4
  PrefetcherOperationProcess = 5
  PrefetcherCacheEntryUpdate = 6
  PrefetcherSpare2 = 7
  PrefetcherAppLaunchScenarioControl = 8
  PrefetcherInformationMax = 9

  ; SUPERFETCH_INFORMATION_CLASS (ntpfapi.h)
  SuperfetchRetrieveTrace = 1
  SuperfetchSystemParameters = 2
  SuperfetchLogEvent = 3
  SuperfetchGenerateTrace = 4
  SuperfetchPrefetch = 5
  SuperfetchPfnQuery = 6
  SuperfetchPfnSetPriority = 7
  SuperfetchPrivSourceQuery = 8
  SuperfetchSequenceNumberQuery = 9
  SuperfetchScenarioPhase = 10
  SuperfetchWorkerPriority = 11
  SuperfetchScenarioQuery = 12
  SuperfetchScenarioPrefetch = 13
  SuperfetchRobustnessControl = 14
  SuperfetchTimeControl = 15
  SuperfetchMemoryListQuery = 16
  SuperfetchMemoryRangesQuery = 17
  SuperfetchTracingControl = 18
  SuperfetchTrimWhileAgingControl = 19
  SuperfetchRepurposedByPrefetch = 20
  SuperfetchChannelPowerRequest = 21
  SuperfetchMovePages = 22
  SuperfetchVirtualQuery = 23
  SuperfetchCombineStatsQuery = 24
  SuperfetchSetMinWsAgeRate = 25
  SuperfetchDeprioritizeOldPagesInWs = 26
  SuperfetchFileExtentsQuery = 27
  SuperfetchGpuUtilizationQuery = 28
  SuperfetchPfnSet = 29
  SuperfetchInformationMax = 30

  ; PSSNT_QUERY_INFORMATION_CLASS (ntpsapi.h)
  PSSNT_QUERY_PROCESS_INFORMATION = 0
  PSSNT_QUERY_VA_CLONE_INFORMATION = 1
  PSSNT_QUERY_AUXILIARY_PAGES_INFORMATION = 2
  PSSNT_QUERY_VA_SPACE_INFORMATION = 3
  PSSNT_QUERY_HANDLE_INFORMATION = 4
  PSSNT_QUERY_THREAD_INFORMATION = 5
  PSSNT_QUERY_HANDLE_TRACE_INFORMATION = 6
  PSSNT_QUERY_PERFORMANCE_COUNTERS = 7

  ; PS_PROTECTED_TYPE (ntpsapi.h)
  PsProtectedTypeNone = 0
  PsProtectedTypeProtectedLight = 1
  PsProtectedTypeProtected = 2
  PsProtectedTypeMax = 3

  ; PS_PROTECTED_SIGNER (ntpsapi.h)
  PsProtectedSignerNone = 0
  PsProtectedSignerAuthenticode = 1
  PsProtectedSignerCodeGen = 2
  PsProtectedSignerAntimalware = 3
  PsProtectedSignerLsa = 4
  PsProtectedSignerWindows = 5
  PsProtectedSignerWinTcb = 6
  PsProtectedSignerWinSystem = 7
  PsProtectedSignerApp = 8
  PsProtectedSignerMax = 9
