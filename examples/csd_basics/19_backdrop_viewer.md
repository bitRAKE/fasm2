# `19_backdrop_viewer` Implementation Brief

Implemented source: `19_backdrop_viewer.asm`.

## Purpose

Show backdrop material changes instead of only proving that
`DWMWA_SYSTEMBACKDROP_TYPE` accepts a value. This rung keeps the `12_backdrop`
caption surface and changes the window configuration plus paint surface:

- full-client `DwmExtendFrameIntoClientArea` margins;
- `DWMWA_USE_HOSTBACKDROPBRUSH`;
- `DWMWA_REDIRECTIONBITMAP_ALPHA` when the OS build is `26100` or newer;
- one bounded premultiplied-alpha viewport rewritten on every paint;
- a software color-grid probe rendered inside that viewport;
- an opaque requested/read-back header and mode-colored panel so every backdrop
  selection is visibly different even when redirected alpha is not honored.

## Design Boundary

`12_backdrop.asm` stays the deterministic opaque GDI example. It is the right
place to teach read-back diagnostics and why skipping broad fills causes resize
smear.

`19_backdrop_viewer.asm` owns the alpha-channel experiment. Redirection alpha is
a window-wide promise, so the example renders normal GDI content into a 32bpp
DIB and then explicitly sets alpha to opaque everywhere outside the material
viewer rectangle. Only the viewer rectangle receives transparent or translucent
premultiplied BGRA pixels.

The software probe separates this rung from `12_backdrop.asm`: it supplies
concrete pixels inside the transparent aperture. The clear band shows the raw
probe, while the mid and strong bands show the requested mode color blended over
that same probe.

## Mode Meaning

The Settings glyph cycles semantic `DWM_SYSTEMBACKDROP_TYPE` requests. The
names shown by the example are the current Windows material mapping, but the
code tracks the enum and the DWM read-back value because Microsoft owns the
final visual policy.

| Visible name | Enum | Intended use |
| --- | --- | --- |
| None | `DWMSBT_NONE` | Disable system backdrop material. |
| Mica | `DWMSBT_MAINWINDOW` | Long-lived main window surface. |
| Acrylic | `DWMSBT_TRANSIENTWINDOW` | Transient surface such as a flyout-style window. |
| Mica Alt | `DWMSBT_TABBEDWINDOW` | Tabbed/titlebar-oriented surface. |

`ViewerColorForType` returns a GDI `COLORREF`; `RenderViewerPanelPixels`
converts that value to the DIB pixel channel order before blending. The fallback
solid GDI fill keeps using the original `COLORREF`.

The viewport is intentionally self-diagnosing. It first paints a different GDI
tint for each requested mode over the probe, then the alpha repair pass changes
only alpha bytes. If the compositor path is unavailable, the user still sees the
selected mode and band strength change instead of staring at an apparently inert
sample.

## Support Surface

`include/addon/csd/backdrop.inc` adds:

| Item | Role |
| --- | --- |
| `DWMWA_USE_HOSTBACKDROPBRUSH = 17` | Enables host backdrop brush support for the window. |
| `DWMWA_REDIRECTIONBITMAP_ALPHA = 39` | Allows the redirected bitmap alpha channel to affect composition on supported builds. |
| `CSD_BACKDROP_ALPHA_BUILD = 26100` | Gates the redirection-alpha attribute to SDK/OS builds that define it. |
| `CSD_BACKDROP_VIEWER_CONFIG` | Stores host/alpha enable flags, alpha capability, and the last `HRESULT`s. |
| `CsdBackdropViewerApply` | Applies host backdrop support and alpha redirection separately so the status line can report both. |

The example-local DIB helpers and software probe renderer remain in
`19_backdrop_viewer.asm` because they are presentation policy, not a general CSD
package primitive.

## Verification

Build from a Visual Studio developer prompt:

```cmd
cd C:\git\fasm2\examples\csd_basics
_build.cmd
```

Manual checks:

- Settings cycles None, Mica, Acrylic, and Mica Alt while `req`/`rd` update.
- The status line shows separate `host` and `alpha` `HRESULT`s.
- Every Settings click visibly changes the viewer header/tint.
- On build `26100+`, the viewer rectangle also shows clear/mid/strong alpha
  bands. The RGB bands are already software-composited, and the alpha bytes are
  still available for DWM redirection behavior.
- Dragging, resizing, maximize/restore, Alt+Space, and the Search edit still
  behave like `12_backdrop`.
