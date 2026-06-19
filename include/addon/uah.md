# uah.inc

UAH stands for UserApiHook. `uah.inc` is a small add-on support layer for code
that needs to recognize the undocumented `WM_UAH*` menu messages and optionally
call the private `uxtheme.dll` ordinals used by dark-mode menu experiments.

It does not install hooks, attach subclasses, or draw menus by itself. The
caller owns the CBT hook or creation hook, subclass procedure, colors, metrics,
and any `WM_UAH*` return values. This include only supplies the message
constants, structure layouts, menu-window detection helper, and tolerant runtime
wrappers for the private exports.

## Include Model

Include the helper after the Win64 Windows include used by the examples:

```asm
include 'windows.inc'
include 'addon/uah.inc'
```

The helper contributes writable globals with:

```asm
define __GLOBAL_BSS__ uah_bss
```

This requires a data model that expands `__GLOBAL_BSS__` into writable storage,
as the example `windows.inc` files do with `irpv xbss,__GLOBAL_BSS__`. A source
that uses a different format/data model must provide equivalent storage for
`uah_bss`, or call the `UahRuntime*` functions with its own `UAH_RUNTIME`
storage.

The include depends on normal Win64 imports for `kernel32` and `user32`:

- `LoadLibraryExW`
- `GetProcAddress`
- `FreeLibrary`
- `RtlZeroMemory`
- `GetClassNameW`
- `lstrcmpW`

## Messages

The include defines the verified undocumented message IDs:

```asm
WM_UAHDESTROYWINDOW     = 0090h
WM_UAHDRAWMENU          = 0091h
WM_UAHDRAWMENUITEM      = 0092h
WM_UAHINITMENU          = 0093h
WM_UAHMEASUREMENUITEM   = 0094h
WM_UAHNCPAINTMENUPOPUP  = 0095h
WM_UAHUPDATE            = 0096h
```

`WM_UAHDRAWMENUITEM` passes a `UAHDRAWMENUITEM` pointer in `lParam`.
`WM_UAHMEASUREMENUITEM` passes a `UAHMEASUREMENUITEM` pointer in `lParam`.

These are not public PSDK definitions. Treat them as trace and subclass support
for code that deliberately opts into this seam.

## Structures

The include translates the support structures from `uah.h`:

- `UAHMENUITEMMETRICS`
- `UAHMENUPOPUPMETRICS`
- `UAHMENU`
- `UAHMENUITEM`
- `UAHDRAWMENUITEM`
- `UAHMEASUREMENUITEM`
- `UAH_RUNTIME`

`UAHMENUITEMMETRICS` stores the largest C union view, `SIZE rgsizePopup[4]`.
The aliases `UAHMENUITEMMETRICS.rgsizeBar` and
`UAHMENUITEMMETRICS.rgsizePopup` both point at offset zero.

## Runtime API

### `UahOpen`

```asm
fastcall UahOpen
```

Opens the process-singleton `uah_runtime` using `LoadLibraryExW` against
`uxtheme.dll` in the system directory and resolves the private ordinals. Returns
nonzero in `eax` when the library was loaded.

The wrapper is tolerant: missing ordinals leave the corresponding function
pointers as zero.

### `UahClose`

```asm
fastcall UahClose
```

Frees the singleton `uah_runtime.Module` when present and clears the runtime
block.

### `UahSetPreferredAppMode`

```asm
fastcall UahSetPreferredAppMode, UAH_APPMODE_ALLOWDARK
```

Calls private `uxtheme` ordinal 135 when available. Returns the ordinal's
`eax` result, or zero when the export is missing.

Available mode constants:

- `UAH_APPMODE_DEFAULT`
- `UAH_APPMODE_ALLOWDARK`
- `UAH_APPMODE_FORCEDARK`
- `UAH_APPMODE_FORCELIGHT`

### `UahFlushMenuThemes`

```asm
fastcall UahFlushMenuThemes
```

Calls private ordinal 136 when available. Returns nonzero when the call was
made, zero when the export is missing.

### `UahAllowDarkModeForWindow`

```asm
fastcall UahAllowDarkModeForWindow, hwnd, TRUE
```

Calls private ordinal 133 when available. Returns the ordinal's `BOOL` result,
or zero when the export is missing.

### `UahRefreshImmersiveColorPolicyState`

```asm
fastcall UahRefreshImmersiveColorPolicyState
```

Calls private ordinal 104 when available. Returns nonzero when the call was
made, zero when the export is missing.

### `UahIsMenuWindow`

```asm
fastcall UahIsMenuWindow, hwnd
```

Returns nonzero when `hwnd` has class name `#32768`, the system popup-menu
window class that receives the popup `WM_UAH*` offers.

## Caller-Owned Runtime

Use the `UahRuntime*` functions when an application wants to own storage instead
of using the singleton:

```asm
my_uah_runtime UAH_RUNTIME

fastcall UahRuntimeOpen,my_uah_runtime
fastcall UahRuntimeSetPreferredAppMode,my_uah_runtime,UAH_APPMODE_ALLOWDARK
fastcall UahRuntimeAllowDarkModeForWindow,my_uah_runtime,[hwnd],1
fastcall UahRuntimeFlushMenuThemes,my_uah_runtime
fastcall UahRuntimeRefreshImmersiveColorPolicyState,my_uah_runtime
fastcall UahRuntimeClose,my_uah_runtime
```

The caller-owned functions follow the same no-op behavior for missing private
exports.

## Subclass Use

A typical menu-window subclass flow is:

1. Open the UAH runtime during startup if dark-mode private wrappers are needed.
2. Detect popup menu windows with a CBT hook or another creation hook.
3. Filter candidate windows with `UahIsMenuWindow`.
4. Attach your own subclass with `SetWindowSubclass`.
5. In the subclass, handle the `WM_UAH*` messages relevant to your menu policy.
6. Remove the subclass on `WM_NCDESTROY` and close the runtime at shutdown.

This include intentionally does not install hooks or subclasses. It is a shared
add-on support surface for applications that already own those policy decisions.
A reusable CBT hook or creation-hook implementation would belong under
`include/subclass`; the UAH message and runtime definitions live here because
they are useful outside one specific subclass implementation.
