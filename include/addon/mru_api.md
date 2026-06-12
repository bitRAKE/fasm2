# addon/mru_api.inc

`addon/mru_api.inc` centralizes the comctl32 MRU ordinal binding and small
descriptor helpers used by fasm2 examples and apps.

This is an addon-level helper, not a subsystem directory. Feature modules such
as recent files, settings, themes, or profiles should live with the app or
example that owns their blob shape and UI policy.

## Include Model

Include after the local Windows setup include:

```asm
include 'addon/windows.inc'
include 'addon/mru_api.inc'
```

The include contributes writable state with:

```asm
define __GLOBAL_BSS__ mru_api_bss
```

The host source must use a data model that expands `__GLOBAL_BSS__` into
writable storage. `addon/windows.inc` does this with
`irpv xbss,__GLOBAL_BSS__`.

Call `LoadMRUAPI` before opening any MRU descriptors. It loads `comctl32.dll`
and resolves every MRU ordinal used by the program.

## Public Surface

The include exposes all known MRU ordinals through `pMRU_*` slots:

- ANSI string list: `CreateMRUListA`, `AddMRUStringA`, `FindMRUStringA`,
  `EnumMRUListA`, `CreateMRUListLazyA`
- Unicode string list: `CreateMRUListW`, `AddMRUStringW`, `FindMRUStringW`,
  `EnumMRUListW`, `CreateMRUListLazyW`
- shared/binary helpers: `FreeMRUList`, `DelMRUString`, `AddMRUData`,
  `FindMRUData`

Function pointers are resolved by ordinal, not by import-table name.

## Descriptor Helpers

`MRUINFO` matches the comctl32 A/W structure shape:

```asm
struct MRUINFO
        cbSize      dd ?
        uMax        dd ?
        fFlags      dd ?
                    dd ?
        hKey        dq ?
        lpszSubKey  dq ?
        lpfnCompare dq ?
ends
```

`MRUDESC` wraps one MRU instance:

```asm
struct MRUDESC
        info_ptr  dq ?
        handle    dq ?
        blob_size dq ?
        is_wide   dq ?
ends
```

Set `is_wide: 1` for descriptors whose `lpszSubKey` is a `du` string. The
existing examples use wide descriptors.

`MRUOpen descp` opens one descriptor and stores the returned handle.
`MRUClose descp` frees the handle and clears it.

## Update Pattern

Use the find/delete/add pattern when updating binary blobs:

```asm
invoke pMRU_FindMRUData,[recent_files.handle],addr blob,\
        sizeof.RecentFileBlob,addr slot
cmp     eax,0
jl      .add
invoke pMRU_DelMRUString,[recent_files.handle],eax
.add:
invoke pMRU_AddMRUData,[recent_files.handle],addr blob,\
        sizeof.RecentFileBlob
```

The return value from `FindMRUData` is the recency position used by
`EnumMRUListW` and `DelMRUString`. The out parameter is the backing slot index
and should not be used for deletion.

For fixed-size descriptors, `MRU_FIND_DEL_ADD inst, ptr` expands this pattern
with include-owned scratch storage.

## Migration Boundary

The richer historical MRU work includes feature modules for colors, fonts,
find/replace, templates, settings, topology, terminal profiles, and command
history. Those modules are pattern references. Promote only stable,
cross-application contracts into addon helpers; keep app-specific menu, dialog,
document, and profile behavior beside the owning application.
