# `18_dpi_manifest_matrix` Implementation Brief

This is the implementation brief for the Tier 3 PE/manifest matrix roadmap
item. It defines the reproducible build, inspection, runtime-observation, and
reporting contract beside `18_dpi_manifest_matrix/`.

## Implementation Status

Implemented directory: `18_dpi_manifest_matrix/`.

The implementation uses one shared CSD source body, `matrix_app.inc`, seeded
from the `05_dpi_frame.asm` behavior. Four tiny generated wrappers define only
the variant strings and resource paths, then include the same source body. The
manifest resources differ by variant; the caption layout, hit-testing,
`WM_DPICHANGED` handling, and status/logging code remain identical.

`build_matrix.cmd` compiles the resources, assembles the four binaries,
extracts embedded manifests with `mt.exe` when available, captures `dumpbin`
headers when available, and writes `reports/matrix.tsv`. Each runtime binary
also writes `reports/runtime_<variant>.tsv` when run from the matrix directory.

## Goal

Turn the DPI manifest behavior in `05` into a small reproducible study:

- assemble the same CSD source under several DPI-awareness manifests;
- inspect the embedded manifest and PE metadata;
- run each variant and record observed DPI behavior;
- compare what the manifest says with what the window actually receives.

The point is evidence. A reader should learn to inspect the binary and observe
messages instead of guessing which DPI mode is active.

## Variants

Suggested matrix:

| Variant | Manifest policy | Expected teaching point |
| --- | --- | --- |
| `unaware` | no DPI manifest entries | Windows can scale the process; CSD geometry is not per-monitor native. |
| `system` | system-DPI aware | DPI chosen at process startup; monitor moves do not act like PMv2. |
| `pmv1` | per-monitor v1 | `WM_DPICHANGED` exists, but behavior differs from PMv2 details. |
| `pmv2` | `PerMonitorV2, PerMonitor` | Current `05`-`09` policy. |

Keep common-controls v6 in every manifest so visual/control differences are not
confused with DPI-awareness differences.

## Source Shape

Use one caption source as the body of the test, ideally a reduced variant of
`05_dpi_frame.asm` or the current latest caption source. The matrix should not
copy the full application four times.

Recommended layout:

```text
18_dpi_manifest_matrix/
  README.md
  build_matrix.cmd
  manifests/
    unaware.manifest
    system.manifest
    pmv1.manifest
    pmv2.manifest
  generated/
    18_unaware.rc
    18_system.rc
    18_pmv1.rc
    18_pmv2.rc
  reports/
    matrix.tsv
```

The generated `.rc` files can all embed the same icon/resource policy plus one
manifest. The `.asm` source can use assemble-time symbols for output names only;
the behavior should otherwise be the same.

The implemented layout keeps `matrix_app.inc` as the shared source body and
places both the tiny wrapper sources and the resource scripts under
`generated/`.

## Build And Inspection Tools

The existing `_build.cmd` already finds `rc.exe`, discovers the latest Windows
SDK include path, compiles manifests through `.rc`, and assembles sources. The
matrix script can reuse that pattern.

Tool roles:

| Tool | Role |
| --- | --- |
| `rc.exe` | Compile the per-variant manifest resource. |
| `fasm2.cmd` | Assemble the caption variant. |
| `mt.exe` | Extract or validate embedded manifests from the built `.exe`. |
| `dumpbin` | Inspect PE headers when available from the Visual Studio developer prompt. |
| `sigcheck` or PowerShell fallback | Optional, only if already available locally. |

Do not require a nonstandard external tool. If `dumpbin` is unavailable, the
script should report that PE-header inspection was skipped and still run manifest
extraction.

## Runtime Observation

Each variant should make its DPI state visible:

- startup DPI from `GetDpiForWindow`;
- each `WM_DPICHANGED` value;
- suggested rectangle size;
- current monitor/work area if monitor helpers are added;
- caption title height and button width after DIP conversion.

Two acceptable observation modes:

1. display status text in the window and require manual movement across monitors;
2. write a small TSV log when messages arrive.

The TSV log is better for the matrix:

```text
variant	event	dpi	x	y	w	h	note
pmv2	create	144	...
pmv2	WM_DPICHANGED	192	...
```

If file logging would distract from the CSD source, keep it as a narrow helper
module or use status text plus a manual report template.

## Report Contract

`reports/matrix.tsv` should capture:

| Column | Meaning |
| --- | --- |
| `variant` | `unaware`, `system`, `pmv1`, or `pmv2`. |
| `manifest_dpiAware` | Extracted 2005 namespace value, if present. |
| `manifest_dpiAwareness` | Extracted 2016 namespace value, if present. |
| `subsystem` | PE subsystem and version from `dumpbin`, if available. |
| `startup_dpi` | First `GetDpiForWindow` value observed. |
| `wm_dpichanged` | Whether the variant received `WM_DPICHANGED` during monitor move. |
| `suggested_rect_applied` | Whether the app applied the suggested rect. |
| `notes` | Manual observation or skipped-tool notes. |

The README should include one filled sample row so readers understand the
expected output.

## Message Integration

Expected source hooks:

| Message/path | Matrix responsibility |
| --- | --- |
| `WM_CREATE` | Record startup DPI and manifest variant label. |
| `WM_DPICHANGED` | Record DPI, suggested rect, and whether the rect was applied. |
| `WM_SIZE` | Optionally record client size after scaling. |
| `WM_DESTROY` | Flush log/report if runtime logging is used. |

Do not change the core CSD layout algorithm per variant. The experiment is the
manifest and resulting Windows behavior, not four different code paths.

## Verification

Build gate:

```cmd
cd C:\git\fasm2\examples\csd_basics
18_dpi_manifest_matrix\build_matrix.cmd
```

Inspection gate:

- `mt.exe` extracts the expected manifest from every variant.
- `dumpbin` results are captured when available, or the skip is explicit.
- The report records which tools were used and which were unavailable.

Manual behavior gate:

- Run each variant on a machine with two DPI scales if available.
- Move windows between monitors.
- Record whether `WM_DPICHANGED` fires.
- Compare title height, icon size, and child placement against the reported DPI.
- Confirm PMv2 matches the current `05`-style behavior.

Single-monitor fallback:

- The matrix still builds and extracts manifests.
- Runtime report records startup DPI and notes that cross-monitor behavior was
  not exercised.

## Source Anchors

| Source | Role |
| --- | --- |
| `18_dpi_manifest_matrix/build_matrix.cmd:1` | SDK/`rc.exe` discovery, resource compilation, assembly, extraction, and report generation. |
| `18_dpi_manifest_matrix/write_report.ps1:1` | Manifest/runtime/dumpbin summarizer for `reports/matrix.tsv`. |
| `18_dpi_manifest_matrix/generated/18_pmv2.asm:2` | Variant wrapper shape: one label set, one resource, shared source include. |
| `18_dpi_manifest_matrix/manifests/pmv2.manifest:12` | Current 2005 `dpiAware` PM entry. |
| `18_dpi_manifest_matrix/manifests/pmv2.manifest:13` | Current 2016 `dpiAwareness` PMv2 entry. |
| `18_dpi_manifest_matrix/generated/18_pmv2.rc:5` | Manifest resource include pattern. |
| `18_dpi_manifest_matrix/matrix_app.inc:189` | Status text with variant, DPI, derived pixels, and event counters. |
| `18_dpi_manifest_matrix/matrix_app.inc:211` | Runtime TSV log creation. |
| `18_dpi_manifest_matrix/matrix_app.inc:236` | Runtime event row writer. |
| `18_dpi_manifest_matrix/matrix_app.inc:721` | `WM_DPICHANGED` routing and suggested-rect logging. |
| `18_dpi_manifest_matrix/matrix_app.inc:809` | `WM_CREATE` startup DPI observation. |
| `18_dpi_manifest_matrix/matrix_app.inc:824` | `WM_SIZE` size observation. |
| `18_dpi_manifest_matrix/matrix_app.inc:891` | `WM_DESTROY` log flush. |
| `include/addon/csd/dpi.inc:5` | `GetDpiForWindow` helper. |
| `include/addon/csd/dpi.inc:28` | `AdjustWindowRectExForDpi` helper. |
| `examples/roadmap.md:173` | Parent PE metadata and loader behavior direction. |
