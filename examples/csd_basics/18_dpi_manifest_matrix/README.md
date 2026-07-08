# `18_dpi_manifest_matrix`

This rung turns the DPI behavior introduced in `05_dpi_frame.asm` into a small
evidence matrix. The same CSD source is assembled four times; only the embedded
manifest changes.

## Build

Run from a Visual Studio developer prompt:

```cmd
cd C:\git\fasm2\examples\csd_basics\18_dpi_manifest_matrix
build_matrix.cmd
```

The script:

- compiles the four manifest resources with `rc.exe`;
- assembles the four generated wrappers with `fasm2.cmd`;
- extracts embedded manifests with `mt.exe` when available;
- captures `dumpbin /headers` output when available;
- writes `reports\matrix.tsv`.

If `dumpbin.exe` is not on `PATH`, the build still succeeds and the report marks
PE-header inspection as skipped.

## Variants

| Variant | Manifest policy |
| --- | --- |
| `unaware` | Common-controls v6 only; no DPI-awareness entry. |
| `system` | 2005 namespace `dpiAware=true`. |
| `pmv1` | 2005 namespace `dpiAware=true/pm`. |
| `pmv2` | 2005 `true/pm` plus 2016 `PerMonitorV2, PerMonitor`. |

The generated executables live under `generated\`:

```cmd
generated\18_unaware.exe
generated\18_system.exe
generated\18_pmv1.exe
generated\18_pmv2.exe
```

Run them from this directory so runtime logs land in `reports\`.

## Runtime Logs

Each variant writes `reports\runtime_<variant>.tsv` while it runs. The window
status line also reports the active variant, current DPI, DPI-scaled title
height, button width, `WM_DPICHANGED` count, and `WM_SIZE` count.

To exercise the matrix manually:

1. Run one variant from this directory.
2. Move it between monitors with different scale factors, or change display
   scale while the window is open.
3. Close the window.
4. Run `build_matrix.cmd` again to refresh `reports\matrix.tsv` from the new
   runtime log.

Single-monitor machines still produce a useful report: startup DPI is recorded,
and `wm_dpichanged` remains `no` until a DPI transition is observed.

## Report

`reports\matrix.tsv` has one row per variant:

```text
variant	manifest_dpiAware	manifest_dpiAwareness	subsystem	startup_dpi	wm_dpichanged	suggested_rect_applied	notes
pmv2	true/pm	PerMonitorV2, PerMonitor	Windows GUI subsystem; 5.00 subsystem version	144	yes	yes	mt extracted manifest; dumpbin headers captured; runtime log captured
```

The sample row is illustrative. Local `startup_dpi` and `subsystem` values come
from the machine and toolchain used to run the matrix.
