# `17_custom_shadow` Implementation Brief

This is the implementation brief for the Tier 3 custom shadow roadmap item. It
defines the layered-window, alpha, geometry, and verification contract beside
`17_custom_shadow.asm`.

## Implementation Status

Implemented source: `17_custom_shadow.asm`.

The current implementation deliberately returns to the `09_embed_edit` visual
surface so the shadow topology stays readable. It removes the
`DwmExtendFrameIntoClientArea` margin call, creates an owned borderless
`WS_EX_LAYERED` popup for the shadow, renders a top-down 32bpp DIB with
premultiplied BGRA pixels, and presents the surface with
`UpdateLayeredWindow(ULW_ALPHA)`.

The main HWND remains a normal overlapped window with custom caption painting,
hit-testing, snap-layout bridging, a real system menu, and the caption-hosted
Search edit. The shadow HWND is a visual companion only; it uses
`WS_EX_TRANSPARENT` and does not own move, resize, menu, or command behavior.

## Goal

Demonstrate what DWM normally gives the examples for free by removing the
one-pixel DWM frame trick and drawing an app-owned shadow.

The rung should show:

- a shadow surface owned by the app;
- `UpdateLayeredWindow` with per-pixel alpha;
- premultiplied alpha generation for a 32bpp DIB;
- positioning the shadow around the real CSD window;
- the behavioral trade-off versus `DwmExtendFrameIntoClientArea`;
- why this path is niche compared with the DWM shadow path.

## Non-Goals

- Do not replace the main CSD window with a layered top-level window.
- Do not implement acrylic, blur, or backdrop materials.
- Do not chase Windows 11 rounded-corner fidelity perfectly.
- Do not add a general vector graphics library.
- Do not keep the DWM margin trick active while claiming the shadow is custom.

## Include And Equate Gate

Local support already exists for the main calls and flags:

| Surface | Local status |
| --- | --- |
| `UpdateLayeredWindow` | `include/api/user32.inc` |
| `WS_EX_LAYERED` | `include/equates/user64.inc` |
| `ULW_ALPHA` | `include/equates/user64.inc` |
| `CreateCompatibleDC`, `CreateDIBSection` | `include/api/gdi32.inc` |
| `BITMAPINFOHEADER`, `BI_RGB` | `include/equates/gdi64.inc` |

The local include set does not currently expose a complete `BLENDFUNCTION`
record or the `AC_SRC_ALPHA` / `AC_SRC_OVER` constants. The rung should add a
small local definition first, or promote it if another example needs it:

```asm
struct BLENDFUNCTION
  BlendOp             db ?
  BlendFlags          db ?
  SourceConstantAlpha db ?
  AlphaFormat         db ?
ends

AC_SRC_OVER  = 0
AC_SRC_ALPHA = 1
```

`UpdateLayeredWindow` expects premultiplied BGRA pixels when `AC_SRC_ALPHA` is
used. That is the central correctness point.

## Window Topology

Use two HWNDs:

| HWND | Role |
| --- | --- |
| Main CSD window | Normal top-level window with custom caption, hit-testing, and content. |
| Shadow window | Borderless layered owned window, positioned behind or around the main window. |

The shadow window should be owned by the main window and use styles such as:

```asm
WS_EX_LAYERED or WS_EX_TRANSPARENT or WS_EX_TOOLWINDOW
```

The exact style set should be documented in the source. `WS_EX_TRANSPARENT`
means hit-testing should pass through the shadow to windows underneath; it does
not make pixels transparent by itself.

## Shadow Bitmap Contract

Generate a 32bpp top-down DIB:

- width = main window width + left/right shadow margins;
- height = main window height + top/bottom shadow margins;
- RGB channels premultiplied by alpha;
- alpha falls off from the window edge outward;
- inner window area is fully transparent so it does not cover the main window.

Simple first algorithm:

1. Clear the DIB to transparent.
2. For each pixel, compute distance to the nearest window rect edge.
3. If the pixel is outside the window rect and inside the shadow radius, compute
   alpha from a small falloff curve.
4. Store BGRA as premultiplied color.

The first rung can use a box or rounded-rectangle distance approximation. The
teaching value is alpha ownership and window topology, not perfect shadow art.

## Geometry And Z Order

The shadow must track the main window:

| Event | Shadow responsibility |
| --- | --- |
| `WM_CREATE` | Create the shadow owner window and initial DIB. |
| `WM_WINDOWPOSCHANGED` or `WM_MOVE` / `WM_SIZE` | Reposition and resize the shadow around the main window. |
| `WM_DPICHANGED` | Recompute shadow radius and margins in physical pixels. |
| `WM_SHOWWINDOW` | Show/hide the shadow with the main window. |
| Minimize | Hide shadow while minimized. |
| Restore | Recreate or show shadow at the restored bounds. |
| Destroy | Destroy shadow before the main window context is freed. |

Keep hit-testing on the main window. The shadow window should not return resize
codes or intercept commands.

## DWM Trade-Offs

The rung should explicitly document what is lost compared with `03`:

| DWM frame path | Custom shadow path |
| --- | --- |
| DWM supplies shadow and compositor animations. | App owns shadow pixels and invalidation. |
| Window manager handles occlusion and monitor changes. | App must move and update the shadow window. |
| Rounded corners follow OS policy. | App must approximate corners. |
| Snap/resize animations integrate naturally. | Shadow can lag or pop unless carefully updated. |
| Minimal code. | Layered-window lifetime and alpha correctness. |

This is a counterexample rung. Its value is understanding why the DWM trick is
usually the right answer.

## Message Integration

Expected integration:

| Message/path | Shadow responsibility |
| --- | --- |
| `WM_CREATE` | Do not call `DwmExtendFrameIntoClientArea`; create shadow window. |
| `WM_NCCALCSIZE` / `WM_NCHITTEST` | Main CSD behavior remains app-owned. |
| `WM_WINDOWPOSCHANGED` | Update shadow position after main position/size changes. |
| `WM_SIZE` | Hide on minimize, update on restore/maximize. |
| `WM_DPICHANGED` | Apply suggested rect to main window, rebuild shadow bitmap at new DPI. |
| Theme/backdrop change | Optionally adjust shadow alpha/color, then call `UpdateLayeredWindow`. |
| `WM_DESTROY` / `WM_NCDESTROY` | Destroy shadow DIB, DC, bitmap, and HWND. |

## Verification

Build gate:

```cmd
cd C:\git\fasm2\examples\csd_basics
_build.cmd
```

Manual behavior gate:

- The main window has a visible app-owned shadow without the DWM margin call.
- Shadow follows move, resize, maximize, restore, and DPI changes.
- Shadow does not intercept mouse input.
- Resize edges and caption drag still work on the main window.
- Snap layouts still work if the snap bridge is included.
- Minimize hides the shadow and restore brings it back.
- Moving across monitors rebuilds shadow margins at the correct DPI.
- No GDI handles leak when repeatedly opening and closing the window.

Pixel/alpha gate:

- Confirm DIB pixels are premultiplied BGRA before `UpdateLayeredWindow`.
- Confirm `BLENDFUNCTION.AlphaFormat = AC_SRC_ALPHA`.
- Confirm `ULW_ALPHA` is used.

## Source Anchors

| Source | Role |
| --- | --- |
| `17_custom_shadow.asm:66` | Local `BLENDFUNCTION` record and `AC_SRC_*` constants. |
| `17_custom_shadow.asm:342` | `CreateShadowSurface`, the 32bpp top-down DIB and compatible DC setup. |
| `17_custom_shadow.asm:423` | `RenderShadowPixels`, the premultiplied BGRA falloff loop. |
| `17_custom_shadow.asm:560` | `CreateShadowWindow`, the owned layered popup style decision. |
| `17_custom_shadow.asm:583` | `UpdateShadowWindow`, geometry sync plus `UpdateLayeredWindow(ULW_ALPHA)`. |
| `17_custom_shadow.asm:1500` | `HandleDpiChanged` rebuilds DPI-scaled shadow margins and updates the popup. |
| `17_custom_shadow.asm:1605` | `WM_CREATE` initializes the shadow path without calling `DwmExtendFrameIntoClientArea`. |
| `17_custom_shadow.asm:1635` | `WM_WINDOWPOSCHANGED` keeps the shadow tracking moves. |
| `17_custom_shadow.asm:1639` | `WM_SIZE` hides the shadow while minimized and refreshes it on visible sizes. |
| `17_custom_shadow.asm:1767` | `WM_DESTROY` tears down the shadow HWND, DC, bitmap, and DIB bits. |
| `include/api/user32.inc:655` | `UpdateLayeredWindow` import. |
| `include/equates/user64.inc:432` | `WS_EX_LAYERED`. |
| `include/equates/user64.inc:1940` | `ULW_*` layered-window flags. |
| `include/api/gdi32.inc:44` | `CreateDIBSection` import for 32bpp shadow bitmap. |
| `examples/font_icons/ideas.md` | Existing note distinguishing straight alpha for icons from premultiplied alpha for `UpdateLayeredWindow`. |
