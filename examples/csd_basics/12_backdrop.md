# `12_backdrop` Implementation Brief

This is the implementation brief for the Tier 1 roadmap item. The corresponding
source now exists as `12_backdrop.asm`; this document remains the include,
version, paint, theme, and verification contract for the rung.

## Goal

Teach how a client-side-decorated window cooperates with a system backdrop
material:

- apply `DWMWA_SYSTEMBACKDROP_TYPE` through `DwmSetWindowAttribute`;
- expose `DWMSBT_MAINWINDOW`, `DWMSBT_TRANSIENTWINDOW`, and
  `DWMSBT_TABBEDWINDOW` as selectable policies;
- show why a normal GDI paint path still needs opaque repaint coverage even when
  a backdrop policy is active;
- keep the existing caption controls, system menu, snap bridge, DPI behavior,
  and child Search edit working;
- document the Windows build gate separately from the existing Windows 11 snap
  gate.

For Windows 11, the named backdrop policies map to Mica, Desktop Acrylic, and
Mica Alt today, but the example should teach the DWM enum names rather than
hard-code material marketing names as the API contract.

## Non-Goals

- Do not use undocumented attributes such as old Mica effect values.
- Do not switch to DirectComposition, WinUI, or a layered-window rendering
  model.
- Do not make the whole app transparent or alpha-blended.
- Do not replace the theme row with a general material system.
- Do not bundle tabbed-caption layout into this rung; `13_caption_tabs` owns
  real tabs.

## Include And Version Gate

The repo imports the DWM window-attribute calls and defines the DWM attributes
used by `06`:

| Surface | Current local state |
| --- | --- |
| `DwmSetWindowAttribute` | `include/api/dwmapi.inc` |
| `DwmGetWindowAttribute` | `include/api/dwmapi.inc` |
| `DWMWA_USE_IMMERSIVE_DARK_MODE` | `include/equates/dwm64.inc` and `dwm32.inc` |
| `DWMWA_WINDOW_CORNER_PREFERENCE` | `include/equates/dwm64.inc` and `dwm32.inc` |
| `DWMWA_BORDER_COLOR`, `DWMWA_CAPTION_COLOR`, `DWMWA_TEXT_COLOR` | `include/equates/dwm64.inc` and `dwm32.inc` |

The backdrop rung needs the newer enum values. Windows SDK `10.0.26100.0`
defines:

```asm
DWMWA_VISIBLE_FRAME_BORDER_THICKNESS = 37
DWMWA_SYSTEMBACKDROP_TYPE            = 38

DWMSBT_AUTO            = 0
DWMSBT_NONE            = 1
DWMSBT_MAINWINDOW      = 2
DWMSBT_TRANSIENTWINDOW = 3
DWMSBT_TABBEDWINDOW    = 4
```

`DWMWA_SYSTEMBACKDROP_TYPE` is documented as supported starting with Windows 11
build `22621`. That is a different threshold from the existing
`CSD_SNAP_WINDOWS11_BUILD = 22000` policy. Do not reuse the snap gate for
backdrops.

Suggested support:

```asm
CSD_BACKDROP_WINDOWS11_BUILD = 22621
```

The promoted `include/addon/csd/backdrop.inc` owns these CSD-specific backdrop
equates so examples can assemble without changing global DWM include policy.

## Backdrop Policy State

Keep backdrop state small and separate from hover/pressed caption state:

```asm
struct CSD_BACKDROP_POLICY
  capable       dd ?
  osMajor       dd ?
  osMinor       dd ?
  osBuild       dd ?
  requestedType dd ?
  appliedType   dd ?
  readbackType  dd ?
  lastResult    dd ?
  readbackResult dd ?
ends
```

`requestedType` is the app's current choice. `appliedType` records what was last
sent successfully to DWM. On down-level systems, `capable = 0` and
`appliedType = DWMSBT_NONE`. `readbackType` records the last
`DwmGetWindowAttribute` value after a successful set. `lastResult` and
`readbackResult` carry the exact `HRESULT` values for status reporting.

Suggested helper surface:

| Helper | Responsibility |
| --- | --- |
| `CsdBackdropProbePolicy` | Use `RtlGetVersion` like the snap helper, but gate on build `22621`. |
| `CsdBackdropSetRequested` | Validate a requested `DWMSBT_*` value and mark it pending. |
| `CsdBackdropApply` | Call `DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, addr value, 4)` only when capable, then read the same attribute back with `DwmGetWindowAttribute`. |
| `CsdBackdropName` | Return a status/menu label for the current requested, applied, or read-back type. |

`CsdBackdropApply` should preserve the exact set `HRESULT` in `eax` for callers
that want to show failure in the status text. Do not silently treat a failed
attribute call as success. The policy row should also retain the read-back
`HRESULT` so the demo can distinguish a successful set from a DWM-selected
effective value.

## Paint Policy

The main teaching point is not the single `DwmSetWindowAttribute` call. It is
the paint contract around that call.

Current `PaintFrame` in `06` and `09` starts with fully opaque fills:

```asm
invoke  GetClientRect,[hMain],addr client_rect
invoke  FillRect,rbx,addr client_rect,[hBodyBrush]
...
invoke  FillRect,rbx,addr title_rect,[hTitleBrush]
```

Those fills cover the material, but this example is still a normal GDI HWND.
Leaving broad GDI regions uncovered makes the redirection surface preserve stale
pixels during resize and move. `12` therefore keeps deterministic opaque coverage
and treats a transparent/composition surface as a later, separate rendering
model:

| Region | Recommended behavior |
| --- | --- |
| Caption strip | Fill the whole `title_rect` in the GDI path before drawing text and owner-drawn controls. |
| Body background | Fill the full client body before transparent text output so old glyph pixels cannot smear. |
| App-drawn edges | Keep the edge fill unless the rung explicitly demonstrates `DWMWA_COLOR_NONE` or a no-border policy. |
| Child Search edit | Keep it opaque at first; native edit transparency is a separate control-hosting problem. |

`WM_ERASEBKGND` should continue to return handled. Letting default erase run can
paint an opaque class brush and hide or flash over the material before
`WM_PAINT`.

The backdrop helper should only track OS capability, requested type, applied
type, and the last `HRESULT`. Paint coverage is intentionally independent of the
DWM enum in this GDI version.

## Theme Interaction

`CSD_THEME` currently owns dark mode, accent colors, DWM caption/border/text
attributes, body/text/button colors, and corner preference. Backdrop support can
extend that row, but the cleaner teaching split is a small `CSD_BACKDROP_POLICY`
beside `CSD_THEME`.

The interaction rules:

- `CsdThemeApplyDwm` still applies dark mode, corner preference, caption color,
  border color, and text color.
- `CsdBackdropApply` applies `DWMWA_SYSTEMBACKDROP_TYPE` after the normal DWM
  theme attributes, then reads the same attribute back for diagnostics.
- On `WM_SETTINGCHANGE` and `WM_DWMCOLORIZATIONCOLORCHANGED`, refresh the theme
  and reapply the backdrop choice.
- If high contrast is active in the future accessibility rung, force
  `DWMSBT_NONE` or system-color fills; material effects should not be needed for
  legibility.

Do not assume `DWMWA_CAPTION_COLOR` is the visible caption fill. These examples
reclaim the caption into the client area; the visible caption fill is whatever
`PaintFrame` draws.

## UI Shape

Keep the UI local and inspectable:

- use the existing Settings glyph to cycle backdrop mode, or add a small
  app-owned menu under it;
- show current mode and OS build/capability in the status text;
- keep the real system menu on the System glyph and Alt+Space path;
- do not add a toolbar or routing framework.

Suggested modes:

| Label | DWM value | Notes |
| --- | --- | --- |
| `None` | `DWMSBT_NONE` | Opaque fills can match `09` behavior. |
| `Mica` | `DWMSBT_MAINWINDOW` | Long-lived main window material. |
| `Acrylic` | `DWMSBT_TRANSIENTWINDOW` | Brighter transient-window material; verify text contrast. |
| `Mica Alt` | `DWMSBT_TABBEDWINDOW` | Prepares the visual story for `13_caption_tabs`. |

If the system is below build `22621`, leave the selector visible but disabled or
show a status message explaining that the current OS cannot apply the backdrop.

## Message Integration

`12_backdrop.asm` starts from `11_caption_fade.asm`, so it keeps the hardest
already-existing surfaces in the test: owner-drawn controls, timer-driven state,
the snap bridge, and a child edit in the caption.

Expected integration:

| Message/path | Backdrop responsibility |
| --- | --- |
| `WM_CREATE` | Probe OS policy and apply the requested backdrop after normal DWM frame setup. |
| `WM_SETTINGCHANGE` | Refresh theme, re-evaluate high contrast if available, reapply backdrop, repaint. |
| `WM_DWMCOLORIZATIONCOLORCHANGED` | Refresh theme, reapply backdrop, repaint. |
| `WM_COMMAND` or Settings click | Change requested backdrop, call `CsdBackdropApply`, update status, invalidate. |
| `WM_ERASEBKGND` | Return handled; do not let the class brush flash before the paint pass. |
| `WM_PAINT` | Fill the full GDI client/caption surface, then draw edges, controls, text, and child-host boundaries. |
| `WM_DESTROY` | No special DWM teardown is required, but setting `DWMSBT_NONE` before destroy is acceptable for symmetry. |

## Verification

Build gate:

```cmd
cd C:\git\fasm2\examples\csd_basics
_build.cmd
```

Manual behavior gate on Windows 11 build `22621` or newer:

- Backdrop mode cycles through None, Mica, Acrylic, and Mica Alt.
- The mode switch updates the DWM attribute and status text without introducing
  stale pixels during move or resize.
- Caption controls, snap hover on maximize, system menu, child Search edit, and
  DPI relayout still work.
- `WM_ERASEBKGND` does not flash a solid class background over the material.
- Text remains readable in light and dark app modes.
- Theme/accent changes do not reset the user's selected backdrop mode.

Down-level gate:

- On builds below `22621`, the example assembles and runs.
- Backdrop application is skipped or reports a clear failure status.
- The window falls back to the opaque `09` paint path.

Inspection gate:

- Confirm the implementation uses `DWMWA_SYSTEMBACKDROP_TYPE = 38`, not an
  old undocumented Mica attribute.
- Confirm `DwmSetWindowAttribute` and `DwmGetWindowAttribute` receive pointers
  to 4-byte `DWM_SYSTEMBACKDROP_TYPE` values and `cbAttribute = 4`.
- Confirm opaque body and caption `FillRect` calls stay unconditional in this
  GDI example.

## Source Anchors

Implemented source points for this brief:

| Source | Role |
| --- | --- |
| `include/addon/csd/backdrop.inc:3` | Local `DWMWA_SYSTEMBACKDROP_TYPE = 38` definition keeps global DWM include policy unchanged. |
| `include/addon/csd/backdrop.inc:26` | `CsdBackdropProbePolicy` gates support on Windows build `22621`. |
| `include/addon/csd/backdrop.inc:128` | `CsdBackdropApply` sets and reads back a 4-byte backdrop enum and preserves both `HRESULT` values. |
| `12_backdrop.asm:292` | `RefreshThemeAndFrame` reapplies the backdrop after normal theme attributes. |
| `12_backdrop.asm:587` | `PaintFrame` keeps full body and caption fills unconditional so repaint coverage remains deterministic. |
| `12_backdrop.asm:878` | `CycleBackdropMode` connects the Settings glyph to backdrop selection. |
| `12_backdrop.asm:1415` | `WM_CREATE` probes snap and backdrop policy before creating paint objects. |
| `12_backdrop.asm:1548` | `WM_ERASEBKGND` remains handled, so default erase does not flash a class brush before the paint pass. |

## References

- Microsoft Learn: [`DWM_SYSTEMBACKDROP_TYPE`](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwm_systembackdrop_type)
- Microsoft Learn: [`DWMWINDOWATTRIBUTE`](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/ne-dwmapi-dwmwindowattribute)
- Microsoft Learn: [`DwmSetWindowAttribute`](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmsetwindowattribute)
