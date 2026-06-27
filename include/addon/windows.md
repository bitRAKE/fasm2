# addon/windows.inc

`addon/windows.inc` is a small Win64 GUI executable policy include for modular
examples and small apps.

It owns:

- PE64 GUI format selection
- `win64wx.inc` inclusion
- global string pools
- static proc64 prologue policy
- inline A/W string materialisation
- `.text` and `.data` section emission
- optional resource section emission

## Include Model

Use it as the first include in the source:

```asm
include 'addon/windows.inc'
```

For a resource file:

```asm
ADDON_WINDOWS_RESOURCE equ 'program.res'
include 'addon/windows.inc'
```

For custom resource section emission:

```asm
ADDON_WINDOWS_RESOURCE_CUSTOM = 1
macro ADDON_WINDOWS_RESOURCE
        section '.rsrc' resource from 'program.res' data readable
        ; emit any additional resource or metadata sections here
end macro
include 'addon/windows.inc'
```

Other API-specific equate files should be included normally by the program that
needs them.

## Data Model

Feature modules contribute data with:

```asm
define __GLOBAL_DATA__ module_data
define __GLOBAL_BSS__ module_bss
```

`addon/windows.inc` expands those vectors into the writable `.data` section
after the string pools.

## Entry Point

The source must provide `proc start ... endp`. The include emits `.end start`
after postponed data/resource sections.

## Checked Calls

`addon/windows.inc` provides a `✓` macro for lightweight failure diagnostics:

```asm
✓       invoke  RegisterClassExW,addr wc
        test    rax,rax
        jz      .fatal_msgbox

  .fatal_msgbox:
        invoke  MessageBox,0,rdx,'Program',MB_ICONERROR
        invoke  ExitProcess,1
```

The macro executes the line that follows it, builds a TCHAR diagnostic string
from that source line plus the source line number, and leaves the message
address in `rdx`. This makes `MessageBox` a natural sink because `rdx` is the
second Win64 argument register and no extra move is needed for `lpText`.

The same `rdx` message pointer can be routed to other diagnostics such as
`OutputDebugString`, a log-file writer, or a shared fatal-error procedure. Keep
the reporting path close to the checked call or copy `rdx` before intervening
calls, since `rdx` is volatile by ABI. Do not use `✓` before a line whose
successful result must remain live in `rdx`; the macro intentionally reuses it
for the diagnostic pointer.
