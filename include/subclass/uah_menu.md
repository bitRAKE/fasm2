# uah_menu.inc

`uah_menu.inc` is the reusable CBT hook layer for UserApiHook popup-menu work.
It does not draw menus. It discovers the short-lived system popup menu windows
created with class name `#32768` and attaches a caller-provided subclass.

The UAH message constants, menu structures, `UahIsMenuWindow`, and tolerant
private `uxtheme.dll` wrappers live in `include/addon/uah.inc`.

## Include Model

```asm
include 'windows.inc'
include 'addon/uah.inc'
include 'subclass/uah_menu.inc'
```

The include contributes writable globals with:

```asm
define __GLOBAL_BSS__ uah_menu_bss
```

This matches the data/BSS aggregation model used by the modular examples. A
source with a different storage model must expand `uah_menu_bss` or provide
equivalent storage.

## Public Interface

### `UahMenuHook_Install`

```asm
fastcall UahMenuHook_Install, hwndOwner, popupSubclassProc, subclassId, dark
```

Installs a thread-local `WH_CBT` hook. On `HCBT_CREATEWND`, the hook filters the
new window with `UahIsMenuWindow`; matching menu windows receive:

```asm
invoke SetWindowSubclass, hwndMenu, popupSubclassProc, subclassId, hwndOwner
```

`dark` is cached and passed to `UahAllowDarkModeForWindow` for each detected
popup menu window. The wrapper is tolerant when the private export is missing.

Returns nonzero on success.

### `UahMenuHook_Uninstall`

```asm
fastcall UahMenuHook_Uninstall
```

Unhooks the CBT hook and clears cached state. Call during application shutdown.

### `UahMenuHook_SetDark`

```asm
fastcall UahMenuHook_SetDark, dark
```

Updates the cached dark-mode flag used for future popup menu windows.

## Caller Subclass

The caller supplies a normal comctl32 subclass procedure:

```asm
proc MenuPopupSubclassProc hwnd,wmsg,wparam,lparam,subclass_id,refdata
```

Typical responsibilities:

- answer `WM_UAHDRAWMENU`, `WM_UAHDRAWMENUITEM`, and
  `WM_UAHMEASUREMENUITEM` when custom menu painting is desired
- optionally frame the popup on `WM_UAHNCPAINTMENUPOPUP`
- call `RemoveWindowSubclass` on `WM_NCDESTROY`
- delegate unhandled messages to `DefSubclassProc`

The `refdata` value is the `hwndOwner` passed to `UahMenuHook_Install`.

## Why A Hook

Application code normally owns `HMENU` data. It does not create or retain the
transient `HWND` used by popup tracking. The CBT hook supplies that missing
creation notification, allowing the application to subclass the popup window
before UAH draw and measure messages arrive.
