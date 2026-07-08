# CSD Roadmap: Direction Vectors

Future directions for the Client-Side Decoration example set, beyond the core
caption rungs and the first complete `10_a11y_keyboard` accessibility step.

This roadmap follows the selection criteria of the parent
`examples/roadmap.md`: prefer additions that
demonstrate a reusable module, a message shape that is easy to get wrong,
resource/manifest behavior hard to infer from docs, or a technique better studied
in isolation. CSD is fertile for all four.

The current set teaches how to *build* a custom caption. The vectors below extend
it toward how to *finish* one — the costs real applications pay after the frame
works — and toward turning the `csd_*.inc` files into a reusable package.

---

## Tier 1 — Complete the responsible-caption story

These finish the arc the existing rungs start. They have the highest teaching
value because each one names a cost that the "borderless window" tutorials on the
internet silently skip.

### `10_a11y_keyboard` — accessibility and keyboard

The missing thesis of the whole set: UIA provider over the descriptor table,
keyboard navigation, high-contrast response, Alt+Space to the system menu. The
descriptor table already reserves the structural room; this rung is where that
foresight pays off. The implementation brief in `10_a11y_keyboard.md` names the
PSDK/include gate, provider model, keyboard state, and verification
requirements.

Implemented source: `10_a11y_keyboard.asm`. The current implementation delivers
keyboard navigation, real-system-menu access, shared command dispatch, focus
cleanup, high-contrast system-color rendering, and a local UIA provider for the
09-level owner-drawn caption buttons. Shared policy-free UIA mechanics now live
in `include/addon/csd/uia.inc` for the richer tabbed-caption rungs, while this
fixed-button provider policy remains local.

### `11_caption_fade` — timer-driven state animation

`CSD_CAPTION_STATE` carries compact animation bytes that earlier rungs initialize
but do not visibly animate. This rung gives them meaning: a timer-driven
hover/press fade.

- demonstrates `WM_TIMER` vs. a fixed-step accumulator (cross-links to the
  `gdi_snake` timing-variant idea in the parent roadmap);
- teaches the generation-counter pattern in `CSD_THEME` as a redraw gate so the
  fade does not fight live theme changes;
- shows how to drive animation without a full render thread.

Reusable payoff: the state animation helpers are now visible in
`include/addon/csd/state.inc`;
`11_caption_fade.asm` applies them with `WM_TIMER` and transient `DC_BRUSH`
fills. The implementation brief in `11_caption_fade.md` defines the broader
state transition, timer, rendering, theme-generation, and verification
contracts.

### `12_backdrop` — Mica / Acrylic / system backdrop

`06` covers DWM caption/border/text color but stops at solid fills. This rung
adds `DWMWA_SYSTEMBACKDROP_TYPE` (Mica, Acrylic, Tabbed) and the interaction
between a backdrop material and an owner-drawn caption.

- teaches the boundary between applying `DWMWA_SYSTEMBACKDROP_TYPE` and actually
  revealing material: this GDI rung keeps opaque repaint coverage, while
  `19_backdrop_viewer` owns the bounded alpha-surface experiment;
- documents the build-gating story (backdrop types arrived across several
  Windows 11 builds) — a natural companion to the CSD snap helper's version
  probe;
- the visually striking rung, useful as the "why bother with CSD" hook.

Implemented source: `12_backdrop.asm`. The implementation brief in
`12_backdrop.md` defines the broader include/version gate, paint policy, theme
interaction, UI shape, and verification matrix.

---

## Tier 2 — The real use cases CSD exists for

The current set demonstrates the *mechanism* with a toy caption. These rungs
demonstrate the *applications* — what Chrome, Explorer, and VS actually do with
the reclaimed pixels.

### `13_caption_tabs` — tabbed title bar

The canonical CSD payoff (Chrome, Windows 11 Explorer/Notepad/Terminal). Promote
the single child slot of `09` into a strip of tabs sharing the caption with the
window controls.

- exercises responsive layout: tabs, new-tab button, and window buttons
  competing for one row;
- teaches drag-region computation when the draggable area is *the gaps between
  tabs* — the hardest part of real CSD and the thing `SetDragRectangles`
  abstracts in WinUI;
- reusable `include/addon/csd/tabstrip.inc`.

Implemented source: `13_caption_tabs.asm`. The implementation brief in
`13_caption_tabs.md` defines the broader tab-strip data model, multi-rectangle
drag map, hit-test precedence, input routing, responsive fit policy, and
verification gates. The source now also includes F6 tab keyboard mode for
fixed caption buttons, visible tab bodies, and close glyphs plus a local UIA
provider for those owner-drawn caption targets.

### `14_responsive_caption` — overflow and collapse

When the window narrows, real captions collapse leading controls into an overflow
menu rather than letting them clip. This rung adds a layout pass that hides/merges
descriptor rows by priority.

- teaches a priority/overflow algorithm over the existing geometry column;
- shows minimum-width enforcement via `WM_GETMINMAXINFO`;
- a small, self-contained algorithm rung — fits the "compact technique" criterion.

Implemented source: `14_responsive_caption.asm`. The implementation brief in
`14_responsive_caption.md` defines collapse priorities, overflow UI, child-HWND
visibility, minimum track-size handling, and verification gates. The source now
also carries the tab keyboard/UIA model through responsive layout and reports
the visible overflow glyph as `caption.overflow` with `ItemStatus` naming hidden
caption commands.

### `15_rtl_caption` — mirrored / right-to-left layout

`CsdCaptionLayout` already separates leading/trailing alignment; RTL is the test
of whether that abstraction is real.

- `WS_EX_LAYOUTRTL` and `WS_EX_RTLREADING` interaction with custom hit-testing;
- proves (or fixes) the leading/trailing split under mirroring;
- low LOC, high "did the abstraction actually hold up" value.

Implemented source: `15_rtl_caption.asm`. The implementation brief in
`15_rtl_caption.md` defines the mirroring policy, include/equate gate,
text/menu rendering rules, hit-test invariants, and verification gates. The
source now also carries the dynamic tab/overflow keyboard and UIA reporting
through RTL, with provider bounds following mirrored geometry and
`caption.overflow` remaining reachable by keyboard in narrow RTL layouts.

---

## Tier 3 — Window-topology and integration breadth

### `16_multi_window` — owned windows and tool frames

CSD on a single top-level window is the easy case. This rung adds an owned tool
window / popup that must share the theme and DPI state without re-implementing
the frame.

- teaches per-window vs. shared state ownership (the `CSD_THEME` row goes from
  global to per-window);
- `WM_DPICHANGED` propagation to owned windows;
- the refactor that turns the includes from "one global window" into "a window
  class you can instantiate."

Implemented source: `16_multi_window.asm`. The implementation brief in
`16_multi_window.md` defines the per-HWND context, owned-window lifetime,
theme/DPI propagation, helper boundaries, and verification gates.

### `17_custom_shadow` — shadow and rounded corners without the DWM trick

A counterpoint rung: what if you *cannot* use the `DwmExtendFrameIntoClientArea`
one-pixel trick (layered window, custom shape)? Demonstrates a hand-drawn shadow
via a layered child or region, and the trade-offs versus `03`'s DWM path.

- teaches `UpdateLayeredWindow` / per-pixel alpha;
- documents exactly what DWM gives you for free in `03`, by removing it;
- niche but illuminating — best studied in isolation.

The implementation brief in `17_custom_shadow.md` defines the layered-window
topology, premultiplied-alpha bitmap contract, DWM trade-offs, and verification
gates.

Implemented source: `17_custom_shadow.asm`. The implementation keeps the
`09_embed_edit` caption surface, creates an owned layered popup for the shadow,
renders a premultiplied 32bpp DIB, and tracks owner move/size/show/DPI messages
without reintroducing the DWM margin call.

### `18_dpi_manifest_matrix` — PE/manifest behavior study

Distills the DPI-awareness and manifest behavior into a reproducible matrix,
matching the parent roadmap's "PE Metadata And Loader Behavior" direction.

- assemble the same caption under unaware / system-aware / PMv1 / PMv2 manifests;
- a script that runs `dumpbin`/`mt.exe` and reports the observed scaling
  behavior per variant;
- value is the reproducible comparison, not the binaries — teaches inspection
  over guessing.

The implementation brief in `18_dpi_manifest_matrix.md` defines the manifest
variants, build/inspection tools, runtime observation, report schema, and
verification gates.

Implemented directory: `18_dpi_manifest_matrix/`. The implementation assembles
one shared `05`-style CSD source through four manifest wrappers, extracts the
embedded manifests, writes `reports/matrix.tsv`, and records optional runtime
TSV logs from the variants.

### `19_backdrop_viewer` — visible backdrop surface

`12_backdrop` proves DWM accepts and reads back backdrop policy; this rung shows
what extra window configuration and alpha ownership are needed before material
changes are visible through a GDI CSD window.

- applies `DWMWA_USE_HOSTBACKDROPBRUSH` beside the backdrop type;
- applies `DWMWA_REDIRECTIONBITMAP_ALPHA` only on build `26100+`;
- uses a full-client DWM sheet and a bounded premultiplied-alpha viewport;
- renders a color-grid software probe inside the viewport and blends the
  requested mode tint over it in clear/mid/strong bands;
- paints an opaque requested/read-back header so backdrop selections remain
  visually distinct if the compositor path is inert;
- repairs the redirected bitmap alpha to opaque outside that viewport so normal
  caption, text, controls, and status pixels do not become see-through.

Implemented source: `19_backdrop_viewer.asm`. The implementation brief in
`19_backdrop_viewer.md` defines the extra DWM config row, alpha-surface policy,
and verification gates.

---

## Cross-cutting vector — promote `csd_*.inc` to a reusable package

Status: initial promotion is implemented. The reusable mechanics now live under
`include/addon/csd/`, and the original `examples/csd_basics/csd_*.inc` names are
compatibility wrappers for earlier rungs that keep their local include shape.
`13_caption_tabs.asm` is the first direct package consumer; `14` and `15`
continue the same direct include path.

The package now contains:

- stable interfaces in `include/addon/csd/` for caption, DPI, theme, state,
  snap, backdrop, child-HWND, tab-strip, and UIA support mechanics;
- separate *policy* (which buttons, what colors) from *mechanism* (layout,
  hit-test, state machine) more sharply, the way `uah_menu` split painter policy
  from hook policy;
- assemble-time offset probes for the public structs (per the parent
  roadmap's "generated include verification" idea), so a consumer catches a
  struct-shape change at build time;
- one canonical direct consumer while compatibility wrappers preserve the
  earlier example source shape.

The tabbed-caption family also has an example-local support include:
`examples/csd_basics/csd_example_support.inc`. It factors shared demo plumbing
from `13`-`15` without promoting those routines to package API, because they
still bind to example globals and call back into rung-owned policy.

This is the step that lets a CSD frame be *carried into another program* — the
bar the parent roadmap sets for an example earning its size.

---

## Cross-cutting vector — build and resize verification

The single `_build.cmd` script was useful while the directory had only a few
rungs, but it becomes less useful as the set grows and as some rungs need
resources, manifests, generated wrappers, extraction tools, or runtime reports.
Move the directory to a small build system that can name targets, preserve
generated/report outputs deliberately, and run focused verification.

Requirements for the replacement:

- target individual examples (`13`, `18:pmv2`, `all`) without editing the
  script;
- keep resource compilation, assembly, manifest extraction, and report writing
  as separate named steps;
- print the current source target before invoking `fasm2.cmd`, so pass-limit or
  include failures identify the file immediately;
- treat `18_dpi_manifest_matrix/reports` as explicit outputs, not incidental
  side effects of every build;
- add smoke-test hooks for live resize, caption drag, snap hover, and window
  close without requiring every manual test to start from the full set.

The same migration should carry a live-resize repaint audit. Current manual
testing shows different behavior classes: `09`-`10` can show frame-only during
resize, `12`-`15` can show frame/background-only during resize, `16` still needs
topology repair, and `17` shows content but flickers heavily. The audit should
separate final repaint correctness from live sizing feedback and document which
messages each rung must handle while USER owns the sizing loop.

---

## Connections to the existing examples tree

- **`msgflood`** is the natural observability partner for every CSD rung; a
  documented pairing is near-zero cost.
- **`font_icons`** is already a dependency from `04` on; the planned "icon
  button strip" in the parent roadmap and `include/addon/csd/tabstrip.inc`
  should share glyph code.
- **DPI-aware window topology** (parent roadmap) overlaps `05` and `16`; CSD is
  the concrete application that prevents DPI handling from being scattered.
- **PE/manifest matrix** (parent roadmap) is realized concretely by `18`.

---

## Priority cut

Highest leverage, in order:

Completed: `14_responsive_caption` now has keyboard/UIA reporting for overflow
and hidden-command state.
Completed: `15_rtl_caption` now carries that tab/overflow reporting model
through mirrored geometry.
Completed: `13_caption_tabs` now exposes fixed new-tab/settings/window buttons
in the same local UIA provider as the dynamic tab targets.
Completed: UIA support mechanics for `13`, `14`, and `15` now live in
`include/addon/csd/uia.inc`; element policy remains in the examples.
Completed: common `13`-`15` demo support plumbing now lives in
`examples/csd_basics/csd_example_support.inc`, with `FUNCTION_MAP.md`
documenting the current extraction boundary.

1. Promote reusable keyboard traversal or element-tree policy only when a later
   rung proves it is common mechanism rather than example-specific UI policy.

These fill the gaps that custom-frame tutorials elsewhere leave vague:
accessibility cost, real tabbed-caption layout, state animation, a reusable
frame module, and modern backdrop materials.
