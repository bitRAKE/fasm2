# addon/csd

`include/addon/csd` holds the reusable mechanics that grew out of
`examples/csd_basics`. It is still a small Win64/fasm2 package, not a windowing
framework: the caller owns the caption policy, glyph choices, colors, command
IDs, child controls, and message routing.

The package keeps the core split used by the teaching examples:

- `caption.inc`: descriptor, geometry, and state rows, DIP conversion, caption
  layout, fixed-control hit-testing, and real system-menu state dispatch.
- `dpi.inc`: per-monitor DPI helpers, resize-border metrics, suggested
  `WM_DPICHANGED` rectangle application, and Segoe UI text font creation.
- `theme.inc`: DWM color/theme refresh, accent ARGB to `COLORREF` conversion,
  and DWM frame-attribute application.
- `state.inc`: hot/pressed/inactive caption input state plus fade-phase helpers.
- `snap.inc`: Windows 11 snap-layout capability probe and maximize hit-test
  policy.
- `backdrop.inc`: Windows 11 system-backdrop capability, read-back state, and
  optional backdrop-viewer window configuration.
- `child.inc`: helper for moving a child HWND into a caption geometry row.
- `tabstrip.inc`: tab item, geometry, state, and strip input rows used by the
  tabbed-caption examples.
- `uia.inc`: policy-free UI Automation constants, COM IDs, provider storage,
  API binding, reference counting, `VARIANT` helpers, and runtime-ID creation.
- `verify.inc`: optional assemble-time size and offset probes for public CSD
  structures.

## Include Model

Include after the normal Win64 Windows setup include, usually
`addon/windows.inc`. Most modules assume the Windows structures and imports are
already available. `dpi.inc` uses the standard GDI `CLEARTYPE_QUALITY` equate;
programs that render icon glyphs still include `examples/font_icons/font_icons.inc`
or another glyph renderer by policy.

Typical full package order:

```asm
include 'addon/windows.inc'
include '..\font_icons\font_icons.inc'
include 'addon/csd/caption.inc'
include 'addon/csd/dpi.inc'
include 'addon/csd/theme.inc'
include 'addon/csd/state.inc'
include 'addon/csd/snap.inc'
include 'addon/csd/backdrop.inc'
include 'addon/csd/tabstrip.inc'
include 'addon/csd/child.inc'
include 'addon/csd/uia.inc'
include 'addon/csd/verify.inc'
```

`verify.inc` can be included after any subset of CSD modules. It verifies only
the structures already defined by earlier includes.

`uia.inc` deliberately stops at reusable mechanics. Consumers still provide the
provider tree, property policy, navigation policy, focus behavior, element
names, and command invocation.

The original `examples/csd_basics/csd_*.inc` names are compatibility wrappers
around these package files. New shared code should include `addon/csd/*.inc`
directly; examples that are meant to preserve old source shape can keep the
wrappers.

## Contract Boundary

The package is mechanism, not application policy. Keep these decisions in the
consumer:

- which caption rows exist and in what visual order;
- whether a row is system, app-owned, danger, child-HWND, or snap-capable;
- icon font or bitmap rendering policy;
- brush/color choices, including high-contrast overrides;
- UI Automation element policy, keyboard focus, overflow, localization, and
  multi-window ownership policy.

This boundary keeps the reusable layer small enough to carry into another
program while leaving the richer CSD examples free to demonstrate one policy at
a time.
