# Modern fasm2 Win64 Example

`examples/modern` is an instructional GUI example trimmed to a study-sized program. It builds a Unicode Win64 text editor with UTF-8 file I/O and a persistent
recent-file menu backed by the comctl32 MRU ordinals.

## Build

Run from a Visual Studio developer prompt or any shell where the Windows SDK is
available:

```cmd
_build.cmd
```

The script calls the repository-local `..\..\fasm2.cmd`, which adds the repo
`include` directory to the assembler include path.

## File Layout

`modern.asm` is the entry point. It only includes modules and orchestrates
startup/shutdown. Each module registers its own data/BSS emitter with
`define __GLOBAL_DATA__ ...` or `define __GLOBAL_BSS__ ...`; `windows.inc`
collects those emitters when it finalizes the executable.

`windows.inc` is the local Win64 policy layer. It sets the PE format, includes
`win64wx.inc`, enables reusable inline strings, installs the static proc64
prologue, and emits module-registered data at the end of the build.

`common.inc` contains code-centric helpers that do not own app state: bounded
wide string copy, case-folded FNV-1a hashing, menu clearing, and current-path
helpers.

`app_state.inc` owns global app data and storage: class strings, title format,
`WNDCLASSEX`, handles, dialog buffers, and title/menu scratch buffers.

`mru_api.inc` binds the undocumented comctl32 MRU entry points by ordinal using
`LoadLibraryW` and `GetProcAddress`. The app uses local function-pointer slots
instead of import-table names.

`mru_recent.inc` is the recent-file feature module. It defines the binary blob,
comparator, save/update path, menu population, and open-by-menu-id behavior.

`dialogs.inc` isolates `OPENFILENAME` setup from file loading and saving.

`file_io.inc` implements bounded UTF-8 load/save with heap allocation and the
standard edit control.

`ui_main.inc` builds menus in code, routes `WM_COMMAND`, owns dirty-state
prompting, and implements the `WindowProc`.

## MRU Pattern

The recent-file cache uses `MRU_BINARY | MRU_CACHEWRITE`. Each record is:

```asm
struct RecentFileBlob
        path_hash dq ?
        sel_start dd ?
        sel_end   dd ?
        path      rw MAX_PATH
ends
```

The comparator first checks `path_hash`, then verifies identity with
`lstrcmpiW`. Updating a record uses the safe sequence:

```text
FindMRUData -> DelMRUString if found -> AddMRUData
```

This avoids the promote-only behavior of `AddMRUData` when a comparator match
already exists.

## Registry Data

State is stored under:

```text
HKCU\Software\Fasm2Examples\ModernMruEdit\Recent
```

The MRU API writes slot values named `a`, `b`, ... and a sibling `MRUList`
value. `MRU_CACHEWRITE` batches registry writes until `FreeMRUList`.

## Extension Points

Good exercises:

1. Add a second MRU module for recent search strings.
2. Add a `settings.inc` module for window size and word-wrap state.
3. Move menu creation to a resource file and compare resource-driven versus
   code-driven UI setup.
4. Add UTF-16 BOM detection before the UTF-8 fallback path.
5. Split dirty prompting into a document module if multiple documents are added.
