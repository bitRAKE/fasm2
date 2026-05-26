# Modern RC fasm2 Win64 Example

`examples/modern_rc` is the resource-backed companion to
`examples/modern`. It keeps the same modular source shape and MRU-backed text
editor behavior, but moves user-interface identity into Win32 resources:

- `modern.rc` owns the menu, accelerator table, modeless Find dialog, manifest
  binding, and version information.
- `resource.h` is the shared ID file included by both the resource compiler
  and fasm2 source.
- `manifest.xml` requests common-controls v6 and normal user privileges.
- `modern.asm` uses an accelerator-aware, modeless-dialog-aware message loop.

## Why This Is Modular

Most small assembly examples are a single source file, and that is usually the
right shape for showing one isolated API call or instruction pattern. This
example is split because it is teaching a larger habit: each include file is a
reusable semantic unit with a stable responsibility.

The module boundary is not just file organization. A reader can learn the
meaning of `mru_recent.inc`, `dialogs.inc`, `file_io.inc`, or
`find_dialog.inc` as a large puzzle piece, then carry that piece into another
program and refine it over time. The code stays close to the machine, but the
application is assembled from recognizable parts:

- A policy layer that defines how this executable is built.
- State modules that own the data and BSS they need.
- Feature modules that expose command-sized procedures.
- UI routing that connects Windows messages to those feature procedures.

That gives the example a reason to be longer than the one-file examples: it
demonstrates how reusable Win32 assembly vocabulary can grow without hiding the
actual API calls or control flow.

## Build

Run from a Visual Studio developer prompt:

```cmd
_build.cmd
```

The build script first compiles `modern.rc` to `modern.res` with `rc.exe`,
then calls the repository-local `..\..\fasm2.cmd`.

The executable includes resources through the local policy file:

```asm
section '.rsrc' resource from 'modern.res' data readable
```

## File Layout

`modern.asm` is the entry point. It includes the shared ID file, pulls in the
modules, loads the resource menu, creates the main window, and runs the message
loop. It stays intentionally thin so the reusable modules remain visible.

`resource.h` is intentionally C-preprocessor style. The same constants drive
`modern.rc` and fasm2 command routing, so menu IDs, dialog IDs, control IDs, and
small policy constants stay in one place.

`modern.rc` contains four resource categories:

- `IDR_MAINMENU` defines File/Edit/Help and the fixed Recent Files submenu.
- `IDR_ACCELERATORS` defines Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S, Ctrl+F,
  Ctrl+A, and F3.
- `IDD_FIND` defines a modeless Find dialog.
- `VERSIONINFO` gives the binary normal Explorer/file-properties metadata.

`manifest.xml` is referenced by `modern.rc` with `RT_MANIFEST`. It keeps the
manifest source readable instead of embedding XML text directly into the
resource script.

`windows.inc` owns executable policy: PE format, imports, proc64 setup, module
data finalization, and the final `.rsrc` section. This is the part to copy when
starting another GUI example with the same local Win64 conventions.

`app_state.inc` owns process-wide handles and state, including `hAccel` and
`hFindDlg`. It keeps the global storage contract explicit instead of letting
state accumulate anonymously in the entry source.

`find_dialog.inc` owns the dialog procedure and edit-control search logic. It
is intentionally separate from `ui_main.inc` so the modeless dialog reads like a
feature module that can be reused or replaced.

`ui_main.inc` loads `IDR_MAINMENU`, captures submenu handles with `GetSubMenu`,
routes `WM_COMMAND` and `WM_DROPFILES`, and owns the main window procedure. It
is the adapter between Windows messages and feature-level procedures.

The remaining modules are inherited from the non-resource example:
`common.inc`, `mru_api.inc`, `mru_recent.inc`, `dialogs.inc`, and
`file_io.inc`. They are deliberately ordinary include files so the same pieces
can be lifted into another fasm2 program without a framework.

## Modeless Dialog Loop

Accelerators and modeless dialogs both need first chance at messages. The main
loop translates the accelerator table first, then offers remaining messages to
the dialog before the normal translate/dispatch path:

```asm
invoke  GetMessageW,addr msg,0,0,0
test    eax,eax
jle     .shutdown
invoke  TranslateAcceleratorW,[hMain],[hAccel],addr msg
test    eax,eax
jnz     .message_loop
cmp     qword [hFindDlg],0
je      .translate
invoke  IsDialogMessage,[hFindDlg],addr msg
test    eax,eax
jnz     .message_loop
.translate:
invoke  TranslateMessage,addr msg
invoke  DispatchMessageW,addr msg
```

That is the key structural difference from `examples/modern`. Without
`TranslateAcceleratorW`, menu shortcuts do not dispatch. Without
`IsDialogMessage`, Tab, Enter, Esc, and default-button behavior in the modeless
dialog are incomplete.

## Accelerators

The accelerator table is declared in `modern.rc`, but the command IDs still
come from `resource.h`:

```rc
IDR_ACCELERATORS ACCELERATORS
BEGIN
    "N", ID_FILE_NEW, VIRTKEY, CONTROL
    "O", ID_FILE_OPEN, VIRTKEY, CONTROL
    "S", ID_FILE_SAVE, VIRTKEY, CONTROL
    "S", ID_FILE_SAVE_AS, VIRTKEY, CONTROL, SHIFT
    "F", ID_EDIT_FIND, VIRTKEY, CONTROL
    "A", ID_EDIT_SELECT_ALL, VIRTKEY, CONTROL
    VK_F3, ID_FIND_NEXT, VIRTKEY
END
```

`LoadAccelerators` returns `hAccel`, and `TranslateAcceleratorW` turns matching
keystrokes into normal `WM_COMMAND` traffic. This keeps keyboard shortcuts on
the same code paths as menu clicks.

## Find Dialog

The Find dialog is created with:

```asm
invoke CreateDialogParam,[hInstance],IDD_FIND,[hMain],FindDlgProc,0
```

`ShowFindDialog` reuses an existing dialog window if one is already open. The
dialog procedure handles `WM_INITDIALOG`, `WM_COMMAND`, `WM_CLOSE`, and
`WM_DESTROY`; `WM_DESTROY` clears `hFindDlg` so the main loop stops routing
messages to a dead window.

The search implementation demonstrates a bounded, local feature module:

- `find_buffer` stores the current query.
- `FindNextInEditor` copies edit-control text into a heap buffer.
- `SearchRange` scans forward, wraps once, supports ASCII case folding, and
  can require simple whole-word boundaries.
- On success, the edit control selection is updated with `EM_SETSEL`.

F3 uses the same command ID as the dialog's Find Next button. If the dialog has
already stored a query in `find_buffer`, `FindNextFromCommand` searches again;
otherwise it opens the modeless Find dialog.

## Drag And Drop

The main window opts into shell drops during `WM_CREATE`:

```asm
invoke DragAcceptFiles,[hwnd],1
```

`WM_DROPFILES` is routed to `OpenDroppedFile`. The handler calls
`DragQueryFile` with `-1` to get the file count, accepts exactly one path into
`dialog_path`, runs the same dirty-document prompt as File/Open, and then calls
`OpenPathIntoEditor`. It always calls `DragFinish` before returning.

Multi-file drops are rejected on purpose. This keeps the example focused on the
single-document editor model while still showing the shell drop API lifecycle.

## MRU Pattern

The recent-file cache still uses `MRU_BINARY | MRU_CACHEWRITE`. Each record is:

```asm
struct RecentFileBlob
        path_hash dq ?
        sel_start dd ?
        sel_end   dd ?
        path      rw MAX_PATH
ends
```

Updating a record keeps the safe sequence:

```text
FindMRUData -> DelMRUString if found -> AddMRUData
```

This avoids the promote-only behavior of `AddMRUData` when a comparator match
already exists.

## Registry Data

State is stored under:

```text
HKCU\Software\Fasm2Examples\ModernRcMruEdit\Recent
```

The MRU API writes slot values named `a`, `b`, ... and a sibling `MRUList`
value. `MRU_CACHEWRITE` batches registry writes until `FreeMRUList`.

## Extension Points

Good exercises:

1. Add a second modeless dialog and route both with the same message-loop
   pattern.
2. Move the dialog search history into a second MRU module.
3. Add localized string-table resources for menu and message text.
4. Split document state from window state if multiple editor windows are added.
5. Add multi-file drop handling after introducing tabs or multiple documents.
