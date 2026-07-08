# `16_multi_window` Implementation Brief

This is the implementation brief for the first Tier 3 roadmap item. It is not
the full framework endpoint. Its job is to define the per-window state,
owned-window, DPI/theme propagation, and teardown contract beside
`16_multi_window.asm`.

## Implementation Status

Implemented source: `16_multi_window.asm`.

The current implementation deliberately returns to the `09_embed_edit` visual
surface so the ownership refactor stays readable. It stores a heap-allocated
`CSD_WINDOW` pointer in `GWLP_USERDATA`, uses `WM_NCCREATE`/`WM_NCDESTROY` for
context lifetime, creates a `WS_EX_TOOLWINDOW` owned CSD frame, keeps both
windows in a small `csd_windows[]` registry, and broadcasts theme refreshes
across that registry.

To keep this rung compact, handlers load the heap context into one active
scratch `CSD_WINDOW`, run the existing 09-shaped helpers, and save the scratch
record back to the owning context after mutating messages. That is a teaching
step toward the helper boundary below, not the final reusable package shape.

## Goal

Turn the single-window CSD examples into a small multi-window topology:

- one main CSD frame;
- one owned tool frame or popup that also uses the CSD support layer;
- shared process policy for theme and fonts where appropriate;
- per-window caption, geometry, state, snap, DPI, and child-control state;
- explicit propagation for `WM_DPICHANGED`, theme refresh, and teardown.

The point is not to create a framework. The point is to show where the current
global state stops scaling.

## Non-Goals

- Do not create an MDI application.
- Do not implement cross-window tab drag or tool-window docking.
- Do not turn every helper into a heap-allocated object if a small state record
  is enough.
- Do not introduce threads.
- Do not keep hidden global window handles that make the second window depend on
  the first window's paint state.

## Current Constraint

The current examples keep most window state global:

| Current global shape | Why it does not scale |
| --- | --- |
| `hMain` | Helpers implicitly target the main window. |
| `current_dpi` | Each top-level window can be on a different monitor. |
| `current_theme` | Shared theme inputs are process-wide, but derived brushes and DWM attributes are per window. |
| `caption_geometry`, `caption_state`, `caption_input` | Each HWND needs independent hit-test and input state. |
| `snap_policy` | The OS capability is shared, but the hit row and state are per window. |
| `hSearchEdit` | Child HWNDs belong to one parent HWND. |

`16` should put those fields into a window context and store a pointer through
`GWLP_USERDATA`.

## State Shape

Suggested context:

```asm
struct CSD_WINDOW
  hwnd              dq ?
  owner             dq ?
  flags             dd ?
  dpi               dd ?
  themeGeneration   dd ?
  currentTheme      CSD_THEME
  snapPolicy        CSD_SNAP_POLICY
  captionInput      CSD_CAPTION_INPUT
  captionDragRect   RECT
  captionGeometry   rb sizeof.CSD_CAPTION_GEOMETRY * CSD_CAPTION_CONTROL_COUNT
  captionState      rb sizeof.CSD_CAPTION_STATE * CSD_CAPTION_CONTROL_COUNT
  hCaptionFont      dq ?
  hTextFont         dq ?
  hBodyBrush        dq ?
  hTitleBrush       dq ?
  hButtonBrush      dq ?
  hEdgeBrush        dq ?
  hSearchEdit       dq ?
  hUiFont           dq ?
ends
```

Although `13`-`15` exist, the implemented `16` keeps the first multi-window rung
close to `09` so the refactor stays readable. Tabs, responsive overflow, and RTL
are intentionally not layered into this first ownership example.

Process-wide state should remain outside the context only when it is truly
shared:

| Shared state | Reason |
| --- | --- |
| `hInstance` | Process/module identity. |
| class names | Window class identity. |
| immutable descriptor tables | Same policy for every instance, unless a tool frame needs its own descriptor table. |
| OS capability probe cache | Same OS build for all windows. |

## Context Lifetime

Recommended lifetime:

1. Allocate `CSD_WINDOW` before or during `WM_NCCREATE`.
2. Store the pointer in `GWLP_USERDATA`.
3. Initialize DPI, theme, fonts, brushes, caption state, and child controls in
   `WM_CREATE`.
4. Every handler loads the context once and passes it to helpers.
5. `WM_NCDESTROY` destroys child HWNDs, GDI objects, and heap state, clears
   `GWLP_USERDATA`, then frees the context.

Do not wait for `WM_DESTROY` alone to free the context. `WM_NCDESTROY` is the
last message tied to the HWND identity and is the better boundary for clearing
per-window storage.

## Owned Window Shape

The second window should be intentionally different enough to prove the
ownership model:

- create it with the main window as `hWndParent`/owner;
- use `WS_EX_TOOLWINDOW` or a documented tool-frame style;
- keep its own caption controls and DPI state;
- omit the Search child or use a smaller descriptor table if that clarifies the
  per-window policy split;
- close it without shutting down the main app;
- destroy it automatically when the owner is destroyed.

The owned window should not use the global `hMain` in paint, layout, or command
routing.

## DPI And Theme Propagation

Each top-level HWND receives its own `WM_DPICHANGED`.

Expected behavior:

- moving the main window across monitors updates only the main context;
- moving the tool window across monitors updates only the tool context;
- shared theme notifications refresh both live contexts;
- brush and font recreation happen per context;
- child controls are moved using the owning context's geometry and DPI.

A small registry of live `CSD_WINDOW` pointers is useful for theme broadcast:

```asm
csd_windows dq CSD_WINDOW_MAX dup ?
csd_window_count dd ?
```

That registry is not a framework; it is the minimum state needed to apply a
process-wide theme change to every live CSD frame.

## Helper Boundary

Refactor helpers only where state ownership requires it:

| Current helper shape | Multi-window shape |
| --- | --- |
| `LayoutCaptionControls` uses globals | `CsdWindowLayoutCaption windowp` |
| `PaintFrame hdc` reads `hMain` and global brushes | `CsdWindowPaint windowp, hdc` |
| `RefreshThemeAndFrame hwnd` writes global `current_theme` | `CsdWindowRefreshTheme windowp` |
| `HandleDpiChanged hwnd,wparam,lparam` writes global DPI | `CsdWindowHandleDpiChanged windowp,wparam,lparam` |

Keep leaf table helpers such as `CsdCaptionIndexFromPoint` generic. The
multi-window rung should reduce global coupling, not wrap every one-line API
call.

## Message Integration

Expected integration:

| Message/path | Multi-window responsibility |
| --- | --- |
| `WM_NCCREATE` | Allocate or attach `CSD_WINDOW`, store `GWLP_USERDATA`. |
| `WM_CREATE` | Initialize per-window DWM, DPI, theme, fonts, caption state, and child controls. |
| `WM_SIZE` | Relayout only this window's context. |
| `WM_DPICHANGED` | Apply suggested rect and rebuild this window's DPI-dependent resources. |
| `WM_SETTINGCHANGE` / `WM_DWMCOLORIZATIONCOLORCHANGED` | Refresh all live CSD windows or post a private refresh message to each. |
| `WM_COMMAND` | Route commands using the owning context and HWND. |
| `WM_DESTROY` | Main window posts quit; owned tool window does not. |
| `WM_NCDESTROY` | Remove context from registry and free per-window resources. |

## Verification

Build gate:

```cmd
cd C:\git\fasm2\examples\csd_basics
_build.cmd
```

Manual behavior gate:

- Main and owned tool windows can be moved independently.
- Both windows draw CSD captions without sharing hover/pressed state.
- Moving one window to a different DPI monitor does not resize fonts in the
  other window.
- Theme/accent changes update both windows.
- Closing the owned window does not quit the app.
- Closing the main window tears down the owned window and frees its resources.
- System menu, snap, and caption commands use the correct HWND.

Debug gate:

- Temporarily display each context pointer or window role in status text.
- Verify no handler reads `hMain` when operating on the tool window.

## Source Anchors

Current source points this brief now anchors:

| Source | Role |
| --- | --- |
| `include/equates/user64.inc:994` | `GWLP_USERDATA` for per-HWND context storage. |
| `16_multi_window.asm:58` | `CSD_WINDOW`, the per-HWND state record. |
| `16_multi_window.asm:183` | Active scratch context and live context registry. |
| `16_multi_window.asm:328` | Heap allocation for `CSD_WINDOW`. |
| `16_multi_window.asm:389` | Scratch load from the heap context. |
| `16_multi_window.asm:395` | Scratch save back to the heap context. |
| `16_multi_window.asm:404` | Theme refresh broadcast over `csd_windows[]`. |
| `16_multi_window.asm:423` | Owned-window teardown scan. |
| `16_multi_window.asm:1305` | Owned tool-frame creation. |
| `16_multi_window.asm:1348` | Context lookup before normal message dispatch. |
| `16_multi_window.asm:1398` | `WM_NCCREATE` context allocation and `GWLP_USERDATA` attach. |
| `16_multi_window.asm:1606` | `WM_NCDESTROY` resource/context teardown. |
