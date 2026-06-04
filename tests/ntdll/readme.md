# NTDLL smoke tests

This directory keeps small Win64 console smoke tests for the repo's NTDLL include
surface. They are intentionally narrow: each sample assembles through the normal
`win64a.inc` path, imports from `NTDLL.DLL`, performs one low-risk runtime check,
and exits with `0` on PASS or SKIP and nonzero on FAIL.

## Surface split

There are two NTDLL source models in `include`:

- `include/equates/ntdll.inc` and `include/api/ntdll.inc` mirror the Windows SDK
  user-mode declarations. Use these for SDK-declared exports, SDK-visible
  structures, and reserved-layout structures from `winternl.h`.
- `include/equates/ntdll_undoc.inc` and `include/api/ntdll_undoc.inc` mirror the
  PHNT native surface. Use these for native heap, virtual memory syscalls,
  pseudo handles such as `NtCurrentProcess`, and private structure fields such
  as `SYSTEM_PROCESS_INFORMATION.Threads`.

Do not include both equate files in one test. Several names overlap, and some
of those names intentionally have different layouts between the SDK-reserved
view and the PHNT-native view.

## Samples

| Sample | Include surface | Reason |
| --- | --- | --- |
| `timer_resolution.asm` | SDK | Uses SDK-declared `NtQueryTimerResolution`. |
| `system_basic_process.asm` | SDK | Uses SDK `SystemBasicProcessInformation` and avoids PHNT allocation APIs. |
| `native_heap.asm` | PHNT | Uses native `RtlCreateHeap`/`RtlAllocateHeap`/`RtlDestroyHeap`. |
| `nt_path.asm` | PHNT | Uses `RtlDosPathNameToNtPathName_U_WithStatus`. |
| `system_process.asm` | PHNT | Uses PHNT `SYSTEM_PROCESS_INFORMATION.Threads` layout. |
| `version_process_thread.asm` | PHNT | Uses `RtlGetVersion`, `THREAD_BASIC_INFORMATION`, and NT pseudo handles. |
| `virtual_memory.asm` | PHNT | Uses `NtAllocateVirtualMemory` and `NtFreeVirtualMemory`. |

## Running

Start from a Visual Studio developer prompt, then run:

```cmd
tests\ntdll\_build.cmd
tests\ntdll\_build.cmd run
```

`_build.cmd` assembles every `.asm` file with the repository-local `fasm2.cmd`.
Passing `run` or `/run` runs each generated `.exe` immediately after it builds.
Use `tests\ntdll\_build.cmd clean` to delete generated executables.

## Further verification

The current tests are smoke coverage, not a full ABI conformance suite. Useful
next checks are 32-bit PE coverage, structure size/offset assertions for both
SDK and PHNT layouts, and import-list audits against newer Windows SDK and PHNT
snapshots.
