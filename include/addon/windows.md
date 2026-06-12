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
