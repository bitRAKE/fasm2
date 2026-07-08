# Client-Side Decoration Basics

This directory turns the local CSD survey into small Win64 GUI examples. Each
example isolates one mechanism before the next one adds behavior back.

The reusable CSD mechanics now live in `include/addon/csd/`. The local
`csd_*.inc` files are compatibility wrappers so earlier rungs can keep their
source shape while new shared code includes `addon/csd/*.inc` directly. The
package includes policy-free UIA support mechanics; each example still owns the
actual element tree and command policy it exposes.

`include/addon/csd/caption.inc` keeps the table split that later examples build
on:

- descriptor rows: immutable ID, glyph, command, hit-test code, base flags, and
  intrinsic width in DIPs;
- geometry rows: physical `RECT` values recomputed by `CsdCaptionLayout`;
- state rows: mutable input/rendering state reserved for hover, pressed,
  disabled, inactive, and fade phases.

`13_caption_tabs.asm` is the first canonical direct package consumer, and
`14_responsive_caption.asm` plus `15_rtl_caption.asm` continue that path. These
sources include `addon/csd/verify.inc` so public CSD row sizes and offsets are
checked at assembly time.

For the line-level "why" behind the unusual choices, keep `CONCEPTS.md` open
beside the source. It contains the table-spine diagram, concept-to-source index,
silent-decision notes, constants notes, and an observability recipe.
`METRICS.md` expands the constants and magic-number notes without moving source
values.
`FUNCTION_MAP.md` maps the `13`-`15` tabbed-caption function layers and explains
why `csd_example_support.inc` is local example support rather than reusable
package API.

## Reader Contract

These are teaching examples, not a production-ready frame template. They assume
the reader is comfortable with the Win64 calling convention, fasmg
`struct`/`iterate`/`proc` style, GDI object lifetime, and the basic Win32
message loop.

The examples intentionally keep some failure handling light so the CSD message
and state contracts stay readable. Production code must check object creation,
font creation, DWM/theme calls, class registration, and child window creation.

Accessibility is still a cost each richer caption surface must pay.
`10_a11y_keyboard.asm` covers the 09-level caption with keyboard access,
high-contrast rendering, and a local UI Automation provider for the owner-drawn
caption buttons. `13_caption_tabs.asm` carries the same responsibility into the
tabbed caption by exposing fixed caption buttons, visible tab bodies, and close
glyphs through keyboard mode and UIA while leaving the Search edit on its native
child-HWND provider. Later overflow, RTL, and multi-window rungs remain
mechanism studies until their fixed caption controls receive equivalent
reporting.

Build from a Visual Studio developer prompt:

```cmd
cd C:\git\fasm2\examples\csd_basics
_build.cmd
```

## Observability

To learn the message vocabulary live, build and run `examples/msgflood` beside
these rungs. It logs its own HWND rather than spying on another process, but it
lets you watch `WM_NCHITTEST`, `WM_NCCALCSIZE`, and `WM_NC*` traffic while you
move, resize, maximize, and right-click a normal window. Compare that traffic
against the message contract listed under each CSD rung. See `CONCEPTS.md` for
the step-by-step recipe and the future debug-overlay direction.

## `01_nccalc_frame.asm`

The first example keeps a normal `WS_OVERLAPPEDWINDOW` top-level window but
handles `WM_NCCALCSIZE` directly. The handler accepts the rectangle USER offered
as the client rectangle instead of letting `DefWindowProcW` shrink it for the
standard caption and sizing border. On the full `wParam != 0` form it returns
`WVR_REDRAW` (`0300h`) so USER invalidates the full surface instead of
preserving stale client pixels.

This is intentionally not a complete borderless window. `WM_NCHITTEST` is not
implemented yet, so the client-filled window does not provide normal mouse drag
or resize affordances. That missing behavior is the next CSD step.

Message contract:

| Message | Behavior |
| --- | --- |
| `WM_NCCALCSIZE` | Accept the client rect; return `0300h` (`WVR_REDRAW`) on the full form to force repaint. |
| `WM_SIZE` | Invalidate the full client surface. |
| `WM_ERASEBKGND` | Return handled; `WM_PAINT` owns all visible pixels. |
| `WM_PAINT` | Draw a simple custom top band, content, and self-drawn edge. |
| `WM_KEYDOWN` | `VK_ESCAPE` destroys the window for this first-step demo. |

DPI note: this example is intentionally 96-DPI only; `05_dpi_frame.asm` owns
per-monitor DPI behavior.

## `02_nchittest_frame.asm`

The second example keeps the same `WM_NCCALCSIZE` frame reclamation and adds
`WM_NCHITTEST`. It sign-extends the packed screen coordinates from `lParam`,
compares them against `GetWindowRect`, and returns the standard `HT*` codes:

| Region | Return |
| --- | --- |
| Top client band | `HTCAPTION` |
| Outer edge strip | `HTLEFT`, `HTRIGHT`, `HTTOP`, `HTBOTTOM` |
| Outer corners | `HTTOPLEFT`, `HTTOPRIGHT`, `HTBOTTOMLEFT`, `HTBOTTOMRIGHT` |
| Body | `HTCLIENT` |

Windows then owns the interactive move/size loop, cursors, drag-to-snap, and
caption double-click behavior even though the application draws the pixels.

DPI note: this example is intentionally 96-DPI only; `05_dpi_frame.asm` owns
per-monitor DPI behavior.

## `03_dwm_frame.asm`

The third example keeps the `WM_NCCALCSIZE` and `WM_NCHITTEST` behavior from
step 2 and calls `DwmExtendFrameIntoClientArea` with one-pixel margins in
`WM_CREATE`. The visible frame is still app-drawn, but the window participates
in the DWM frame path used for compositor-managed shadows and window-manager
animations.

DPI note: this example is intentionally 96-DPI only; `05_dpi_frame.asm` owns
per-monitor DPI behavior.

## `04_caption_controls.asm`

The fourth example uses the reclaimed caption pixels for real UI. It imports
`examples/font_icons/font_icons.inc` and declares immutable
`CSD_CAPTION_DESCRIPTOR` rows. Runtime rectangles live in `caption_geometry`,
and `caption_state` is present but still normal/zeroed until the dynamic-state
example starts writing it.

This is the steepest rung in the set. Read it in four passes: first the
descriptor/geometry/state table, then icon-font rendering, then hit-testing and
the reduced drag strip, then the system-menu and command-routing path.
`CONCEPTS.md` expands those four passes with source anchors.

Current source ranges for those passes:

| Pass | Current anchors |
| --- | --- |
| Table shape | `04_caption_controls.asm:59-115`, `include/addon/csd/caption.inc:50` |
| Rendering | `04_caption_controls.asm:181-209`, `04_caption_controls.asm:215` |
| Hit-testing | `04_caption_controls.asm:165-173`, `04_caption_controls.asm:347-450` |
| Commands | `04_caption_controls.asm:303-341`, `04_caption_controls.asm:475-541`, `include/addon/csd/caption.inc:159-223` |

The same table drives drawing and hit-testing:

| Region | Return/behavior |
| --- | --- |
| Menu glyph | `HTCLIENT`; click opens the real system menu. |
| Search and Settings glyphs | `HTCLIENT`; `WM_LBUTTONUP` routes to app-owned commands. |
| Minimize, maximize/restore, close glyphs | `HTCLIENT`; `WM_LBUTTONUP` dispatches the matching `WM_SYSCOMMAND`. |
| Remaining caption strip | `HTCAPTION`; this is the reduced movement area. |
| Body | `HTCLIENT`. |

This keeps the native system menu and window-management commands while making
the caption surface available to the application. The window buttons are not
reported as `HTMINBUTTON`, `HTMAXBUTTON`, or `HTCLOSE`, because those native
hit-test results ask USER/DWM to manage and paint standard caption-button
states. The maximize glyph dispatches `SC_RESTORE` while the window is zoomed,
and the system menu item state is refreshed after size/state changes, after
caption/system-menu commands, and again when the menu is initialized or shown.

DPI note: this example still passes `96` to `CsdCaptionLayout`; the descriptor
widths are DIPs so `05_dpi_frame.asm` can make geometry DPI-dependent without
rewriting the table.

## `05_dpi_frame.asm`

The fifth example opts into per-monitor DPI awareness v2 with
`05_dpi_frame.manifest`. It keeps the 04 descriptor/geometry/state split, but
the geometry column now depends on the current DPI:

| Concern | Behavior |
| --- | --- |
| Manifest | `05_dpi_frame.rc` embeds a PMv2 manifest resource. |
| Initial size | A desired client size in DIPs is converted to pixels and passed through `AdjustWindowRectExForDpi`. |
| Current DPI | `GetDpiForWindow` initializes the window DPI with a 96-DPI fallback. |
| Resize border | `GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi)` feeds the resize hit-test strip. |
| Caption layout | `CsdCaptionLayout` receives the current DPI and scales descriptor widths from DIPs to physical `RECT`s. |
| DPI transition | `WM_DPICHANGED` applies USER's suggested `RECT` with `SetWindowPos`, rebuilds the icon and Segoe UI text fonts, relayouts the caption, and repaints. |

This file deliberately leaves color/theme response and hover/pressed state flat;
those are the next two rungs.

## `06_dwm_theme.asm`

The sixth example keeps 05's PMv2 geometry and adds live DWM/theme inputs
through the CSD theme helper. This covers the DWM attribute path not
demonstrated by the one-pixel frame extension in 03:

| Concern | Behavior |
| --- | --- |
| Theme row | `CSD_THEME` stores dark-mode state, accent color, derived frame colors, corner preference, and a generation counter. |
| Accent color | `DwmGetColorizationColor` reads the current colorization ARGB value and converts it to a GDI `COLORREF`. |
| Dark mode | `WindowsAppsUseDarkMode` reads the user app-theme registry value through `RegGetValue`. |
| DWM attributes | `DwmSetWindowAttribute` applies immersive dark mode, caption color, border color, text color, and rounded-corner preference. |
| Live refresh | `WM_SETTINGCHANGE` and `WM_DWMCOLORIZATIONCOLORCHANGED` rebuild the theme row, recreate brushes, and repaint. |

The theme generation advances on startup and when the live accent/dark-mode
inputs change, so later rendering caches can key on it without treating every
redundant notification as a content change. Hover and pressed caption state are
still intentionally absent; 07 owns that mechanism.

## `07_caption_state.asm`

The seventh example keeps 06's DPI and theme rows and starts mutating
`caption_state`. The CSD state helper owns the small state machine over the
descriptor, geometry, and state columns:

| Concern | Behavior |
| --- | --- |
| Mouse tracking | `WM_MOUSEMOVE` arms `TrackMouseEvent` with `TME_LEAVE` for the client-owned caption glyphs. |
| Hover state | The current client point maps through the geometry column and writes `CSD_STATE_HOVER` to the matching state row. |
| Pressed state | `WM_LBUTTONDOWN` captures the mouse and writes `CSD_STATE_PRESSED` only while the pressed control is also hot; `WM_LBUTTONUP` releases capture and routes the existing command path only when press and release match. |
| Inactive state | `WM_ACTIVATE/WA_INACTIVE` clears hot/pressed state and marks every caption control inactive. |
| Rendering | The caption renderer resolves descriptor flags plus state: normal buttons get hover/pressed/inactive fills, and the close button keeps special red hover/pressed colors. |

This file deliberately does not add the Windows 11 snap-layout hit-test policy,
embedded child controls, or accessibility. The state row already reserves the
animation bytes that `11_caption_fade.asm` later makes active without changing
the state column shape.

## `08_snap_layouts.asm`

The eighth example revisits 04's decision to keep the maximize glyph as
`HTCLIENT`. The CSD snap helper probes the real OS build through `RtlGetVersion`
loaded from `ntdll.dll`; Windows 11 build 22000 or newer enables the snap
policy:

| Concern | Behavior |
| --- | --- |
| Down-level path | The maximize glyph still returns `HTCLIENT`, so the 07 client mouse and command path is unchanged. |
| Snap-capable path | `WM_NCHITTEST` returns `HTMAXBUTTON` only for the maximize glyph, allowing the shell to recognize the snap-layout affordance. |
| Owner drawing | The renderer is still the 07 owner-drawn state renderer; the descriptor stays in the same table. |
| NC click routing | `WM_NCLBUTTONDOWN`/`WM_NCLBUTTONUP` for `HTMAXBUTTON` are translated back to client coordinates and routed through the same pressed/release command logic. |
| NC hover bridge | `WM_NCMOUSEMOVE` updates the max row's hot state and arms `TrackMouseEvent` with `TME_NONCLIENT`; `WM_NCMOUSELEAVE` clears it. |

This file deliberately does not embed a live child control or add accessibility.
Those are separate costs once a caption slot becomes real content.

## `09_embed_edit.asm`

The ninth example keeps 08's snap policy and promotes the Search descriptor slot
into a live child `EDIT` control. The descriptor table still owns the slot width
and hit-test result, but the `CSD_CAPTION_CHILD` flag tells the parent renderer
to skip that row so the child HWND owns its pixels.

| Concern | Behavior |
| --- | --- |
| Child placement | `include/addon/csd/child.inc` derives an inset child rectangle from the Search row in `caption_geometry` and moves the edit on create, size, and DPI changes. |
| Input partition | The Search row remains `HTCLIENT`; focus, caret, text input, selection, and IME belong to the child edit. |
| Parent painting | The top-level window uses `WS_CLIPCHILDREN`, and `DrawCaptionControls` skips rows flagged `CSD_CAPTION_CHILD`. |
| Edit notifications | `WM_COMMAND/EN_CHANGE` updates the status text without changing the caption-button state machine. |
| Edit colors | `WM_CTLCOLOREDIT` returns the current body brush and text colors, keeping the child aligned with the active theme. |

This file deliberately does not add accessibility or keyboard navigation beyond
the native edit behavior. That is the next cost once caption content is a mix of
owner-drawn rows and real child windows.

## `10_a11y_keyboard.asm`

The tenth example keeps 09's live Search edit and snap-layout bridge, then adds
keyboard access, high-contrast rendering, and UI Automation reporting for the
owner-drawn caption rows. The child `EDIT` keeps its native HWND/provider
behavior; keyboard mode intentionally skips it and moves only across
owner-drawn caption buttons.

| Concern | Behavior |
| --- | --- |
| Keyboard entry | F10 or Alt enters caption mode and focuses the first owner-drawn row. |
| Focus order | Left/Right wrap through System menu, Settings, Minimize, Maximize/Restore, and Close, matching visual order rather than descriptor order. |
| Invocation | Enter/Space dispatch through the same real system-menu, `WM_SYSCOMMAND`, and app-command paths used by mouse activation. |
| System menu | Alt+Space opens the real system menu; the example does not introduce an app-owned replacement. |
| Cleanup | Escape, focus loss, and deactivation clear caption keyboard mode and repaint. |
| High contrast | `SPI_GETHIGHCONTRAST` switches caption colors to system colors and `DrawFocusRect` keeps a visible focus primitive. |
| UI Automation | `WM_GETOBJECT` returns a local provider root; the five owner-drawn caption buttons report name, automation ID, Button control type, bounds, enabled/focus state, and Invoke. |
| Native child split | The Search edit remains a native child provider and is not duplicated as an owner-drawn UIA button. |

The provider policy is deliberately local to this rung and dynamically loads
`UIAutomationCore.dll`/`oleaut32.dll` through existing `kernel32` imports. The
later tabbed-caption rungs now share policy-free UIA mechanics through
`include/addon/csd/uia.inc`, but the 09-level owner-drawn caption rows remain
local to this simpler accessibility example.

## `11_caption_fade.asm`

The eleventh example keeps 09's child edit and snap behavior, then gives
caption-state transitions a small `WM_TIMER` animation. State rows retain the
same size, but `fromValue`, `value`, `fadePhase`, and `animFlags` now describe
an in-flight visual transition.

| Concern | Behavior |
| --- | --- |
| State transition | `CsdCaptionStateApply` stores the previous visual state in `fromValue`, writes the new `value`, resets `fadePhase`, and marks the row active. |
| Timer lifetime | Input handlers start `CSD_FADE_TIMER_ID` only when active animation rows exist; `WM_TIMER` advances phases and stops when all rows are idle. |
| Rendering | `DrawCaptionControls` resolves old and new states through the current theme, blends `COLORREF` channels, and fills through `DC_BRUSH`. |
| Theme changes | Because rows store state IDs rather than cached colors, live theme changes redraw through the latest `CSD_THEME` colors. |

This file deliberately predates the accessibility rung. The native edit remains
accessible, and `10_a11y_keyboard.asm` shows keyboard, high-contrast, and UIA
provider coverage for the simpler 09-level owner-drawn buttons.

## `12_backdrop.asm`

The twelfth example keeps 11's fades, child edit, snap bridge, and system-menu
path, then adds a small DWM system-backdrop policy beside the theme row. The
Settings glyph cycles `DWMSBT_NONE`, `DWMSBT_MAINWINDOW`,
`DWMSBT_TRANSIENTWINDOW`, and `DWMSBT_TABBEDWINDOW` on capable Windows 11
builds.

| Concern | Behavior |
| --- | --- |
| OS gate | `include/addon/csd/backdrop.inc` probes `RtlGetVersion` and enables backdrop calls only on build `22621` or newer. |
| DWM attribute | `CsdBackdropApply` calls `DwmSetWindowAttribute` with `DWMWA_SYSTEMBACKDROP_TYPE = 38`, then reads the same 4-byte enum back with `DwmGetWindowAttribute`. |
| Paint policy | `PaintFrame` still fills the full client and caption before drawing edges, controls, text, and the opaque child edit; leaving normal GDI pixels uncovered produces stale redraw artifacts. |
| Selector | The Settings caption glyph cycles backdrop modes and updates the status line with requested/read-back modes, set/get `HRESULT`s, OS build, capability, edit HWND, and snap hit-test policy. |

This file deliberately keeps native edit transparency and high-contrast material
policy out of scope. Those belong with the later accessibility and control-host
work.

## `13_caption_tabs.asm`

The thirteenth example keeps 12's backdrop, fade, snap, system-menu, DPI, and
child-edit behavior, then adds an owner-drawn tab strip in the reclaimed caption
band. Fixed caption controls still use `caption_descriptors`; tabs use a
separate `CSD_TAB_ITEM` / `CSD_TAB_GEOMETRY` / `CSD_TAB_STATE` row set.

| Concern | Behavior |
| --- | --- |
| Dynamic rows | `InitTabs`, `AddDemoTab`, and `CloseDemoTab` keep item, geometry, and state rows compact as the tab count changes. |
| Layout | `LayoutTabs` uses the old drag band as the tab band, shrinks visible tabs toward a minimum width, keeps the active tab visible, and writes draggable gap rectangles. |
| Hit-testing | `HitTestCsdFrame` checks fixed caption controls first, then tab bodies/close glyphs as `HTCLIENT`, then tab gaps as `HTCAPTION`. |
| Input | Tab bodies select on matched press/release; close glyphs compact rows on matched press/release; dragging off cancels. |
| Keyboard | F6 enters tab keyboard mode; Left/Right walk fixed caption buttons, visible tab bodies, and close glyphs; Ctrl+Tab jumps between tab bodies; Enter/Space invokes, Delete closes tabs only, and Escape leaves the mode. |
| UI Automation | `WM_GETOBJECT` returns a local caption/tab provider root; fixed caption buttons report Button and Invoke, visible tab bodies report TabItem, name, selected state, bounds, Invoke, and SelectionItem, and visible close glyphs report Button, bounds, and Invoke. |
| Rendering | `DrawTabs` uses GDI fills, `DrawTextW` ellipsis, and `font_icons` close glyphs on the caption surface. |

`10_a11y_keyboard.asm` remains the simpler descriptor-table provider example.
`13_caption_tabs.asm` demonstrates the next step: one caption provider surface
that includes fixed owner-drawn buttons and the dynamic tab targets, while the
Search edit remains a native child provider.

## `14_responsive_caption.asm`

The fourteenth example keeps 13's tab strip, backdrop, snap bridge, system-menu
path, and Search child edit, then adds a responsive visibility pass over the
fixed caption descriptors. A sidecar policy table names required rows,
overflowed commands, and child-HWND rows without overloading the descriptor
flags used for rendering and hit-testing.

| Concern | Behavior |
| --- | --- |
| Collapse policy | `ComputeResponsivePolicy` converts descriptor widths from DIPs to pixels, hides Search first, then Settings, then New Tab, and shows the overflow glyph only while it represents hidden commands. |
| Hidden geometry | `LayoutCaptionControls` clears every geometry row before writing visible rectangles, so collapsed rows do not hit-test, hover, press, or block the tab band. |
| Overflow | The overflow glyph opens an app-owned popup menu for hidden Search/New Tab/Settings commands; the real system menu remains on the system glyph and caption context-menu path. |
| Child HWND visibility | The Search `EDIT` is hidden with `ShowWindow(SW_HIDE)` when its row collapses, focus is returned to the main HWND if needed, and the edit receives the same per-DPI text font used by owner-drawn body text. |
| Minimum size | `WM_GETMINMAXINFO` writes `MINMAXINFO.ptMinTrackSize` after converting the irreducible client width and body height through `AdjustWindowRectExForDpi`. |
| Keyboard/UIA | F6 enters the tab/overflow keyboard mode; Left/Right include the overflow glyph when visible; `WM_GETOBJECT` reports visible tab targets plus `caption.overflow` with `ItemStatus` naming hidden commands. |

This file still keeps the accessibility warning scoped: the dynamic tab strip
and overflow glyph now have keyboard/UIA reporting, but the fixed
system/search/new-tab/settings/window buttons are not all re-exposed by a
14-specific descriptor provider. `10_a11y_keyboard.asm` remains the simpler
descriptor-table provider example for fixed caption buttons.

## `15_rtl_caption.asm`

The fifteenth example keeps 14's responsive caption, tab strip, overflow menu,
Search child policy, snap bridge, and system-menu path, then adds a runtime
LTR/RTL layout toggle with `F2`. It deliberately mirrors geometry itself rather
than relying on `WS_EX_LAYOUTRTL` to fix owner-drawn rectangles.

| Concern | Behavior |
| --- | --- |
| Direction state | `layout_flags` carries `CSD_LAYOUT_RTL`; the status line reports `LTR` or `RTL`. |
| Fixed controls | `LayoutCaptionControls` keeps descriptor order stable, but in RTL leading rows consume the physical right side and trailing rows consume the physical left side. |
| Tabs | `LayoutTabs` flows visible tabs right-to-left in RTL, moves each tab close glyph to the mirrored leading side, and writes mirrored drag-gap rectangles. |
| Text and menus | Tab titles use `DT_RIGHT or DT_RTLREADING` in RTL; both real system menu and app overflow popup add `TPM_LAYOUTRTL` while RTL is active. |
| Search child | The edit remains a neutral search field, created with local `WS_EX_NOINHERITLAYOUT`; its parent caption slot mirrors around it. |
| Hit-testing | Resize edges remain physical `HTLEFT`/`HTRIGHT`; caption controls, tabs, overflow, and snap maximize use the mirrored geometry rows. |
| Keyboard/UIA | The tab/overflow provider from 14 is carried forward; `rtl.root`, visible tab targets, and `caption.overflow` report mirrored bounds, and Left/Right follows the mirrored tab flow. |

This file still keeps full bidirectional localization out of scope. The dynamic
tab strip and overflow glyph are keyboard/UIA reachable in LTR and RTL, but the
fixed system/search/new-tab/settings/window buttons are not all re-exposed by a
15-specific descriptor provider.

## `16_multi_window.asm`

The sixteenth example steps back to the `09_embed_edit` caption surface so the
ownership refactor is readable. It stores per-HWND state in a heap-allocated
`CSD_WINDOW` record, attaches that pointer with `GWLP_USERDATA`, and creates an
owned CSD tool frame that has its own caption state, DPI, theme brushes, fonts,
and Search child HWND.

| Concern | Behavior |
| --- | --- |
| Context lifetime | `WM_NCCREATE` allocates/registers `CSD_WINDOW`; `WM_NCDESTROY` destroys child/GDI resources, clears `GWLP_USERDATA`, unregisters, frees the heap record, then lets `DefWindowProc` finish. |
| Per-window state | The old single-window fields move into `CSD_WINDOW`: HWND, owner, role flags, DPI, theme, brushes, fonts, caption input/geometry/state, snap policy, child edit, and status text. |
| Owned tool frame | The main frame creates a `WS_EX_TOOLWINDOW` owned frame. Closing the tool does not post quit; closing the main window destroys owned CSD windows. |
| Theme broadcast | A small `csd_windows[]` registry lets theme/accent notifications refresh every live CSD context. |
| DPI isolation | `WM_DPICHANGED` updates only the active context; each Search edit receives its owning context's rebuilt text font. |
| Observability | Status text shows window role, context pointer, owner HWND, edit HWND, and snap policy so shared-state bugs are visible. |

This file deliberately does not carry forward tabs, overflow, or RTL from
`13`-`15`. The teaching point is the per-HWND ownership boundary; those richer
caption policies can be layered onto the same context shape later.

## `17_custom_shadow.asm`

The seventeenth example is a counterpoint to `03_dwm_frame.asm`: it removes the
one-pixel `DwmExtendFrameIntoClientArea` margin trick and draws an app-owned
shadow with a separate layered popup. The main HWND remains a normal CSD window;
only the shadow pixels move into the owned `WS_EX_LAYERED` surface.

| Concern | Behavior |
| --- | --- |
| Shadow topology | `CreateShadowWindow` creates an owned borderless popup with `WS_EX_LAYERED`, `WS_EX_TRANSPARENT`, `WS_EX_TOOLWINDOW`, and local no-activate style support. |
| Pixel format | `RenderShadowPixels` fills a top-down 32bpp DIB with premultiplied BGRA values before `UpdateLayeredWindow(ULW_ALPHA)`. |
| Main HWND behavior | Hit-testing, resize edges, caption drag, snap-layout maximize, system menu, and the Search edit remain on the normal overlapped window. |
| Geometry sync | `WM_WINDOWPOSCHANGED`, `WM_SIZE`, `WM_SHOWWINDOW`, and `WM_DPICHANGED` update or hide the shadow as the owner moves, resizes, minimizes, restores, or changes DPI. |
| Trade-off | The example shows the extra lifetime, alpha, and tracking code you own when DWM is not providing the shadow and compositor behavior. |

This file is intentionally niche. It teaches layered-window ownership and
premultiplied alpha; the DWM-margin path remains the preferred route for normal
CSD windows.

## `18_dpi_manifest_matrix`

The eighteenth rung is a PE/manifest inspection matrix rather than one more
single executable. It assembles the same DPI-aware CSD body under four manifest
policies and records what the binary says versus what the window observes at
runtime.

| Concern | Behavior |
| --- | --- |
| Shared source | `18_dpi_manifest_matrix/matrix_app.inc` carries the `05`-style caption, DPI relayout, `WM_DPICHANGED` suggested-rect path, and CSD hit-testing. |
| Variants | Tiny wrappers under `generated/` define `unaware`, `system`, `pmv1`, and `pmv2` labels/resources while including the same source body. |
| Manifest inspection | `build_matrix.cmd` compiles resources, assembles all four binaries, extracts manifests with `mt.exe`, and captures `dumpbin` headers when available. |
| Runtime observation | Each variant writes `reports/runtime_<variant>.tsv` with startup, size, DPI-change, and destroy events; the window also paints variant/DPI counters in its status line. |
| Matrix report | `write_report.ps1` produces `reports/matrix.tsv`, combining manifest fields, optional PE-header text, and runtime logs when present. |

Build it separately from the normal rung build:

```cmd
cd C:\git\fasm2\examples\csd_basics\18_dpi_manifest_matrix
build_matrix.cmd
```

The first report is still useful on a single monitor: it proves the manifests
were embedded and records skipped tools. Runtime DPI behavior becomes stronger
evidence after running the variants across monitors with different scale
factors.

## `19_backdrop_viewer.asm`

The nineteenth rung returns to the `12_backdrop` caption surface and changes the
paint/configuration policy so backdrop material changes are actually visible.
The normal caption, Search child HWND, snap bridge, system menu, and backdrop
selector remain intact.

| Concern | Behavior |
| --- | --- |
| Extra DWM configuration | `include/addon/csd/backdrop.inc` adds a `CSD_BACKDROP_VIEWER_CONFIG` row and applies `DWMWA_USE_HOSTBACKDROPBRUSH`; on build `26100` or newer it also applies `DWMWA_REDIRECTIONBITMAP_ALPHA`. |
| Full-client DWM sheet | The window uses full `DwmExtendFrameIntoClientArea` margins so the material path is present across the client, not only a one-pixel edge. |
| Software probe | `RenderViewerPanelPixels` paints a color-grid probe directly into the viewer DIB, then blends the requested mode tint over it per alpha band. |
| Diagnostic viewport | `PaintFrame` paints a blended mode panel with an opaque requested/read-back header so every Settings click has an obvious visual result. |
| Alpha viewport | `ApplyViewerSurfaceAlpha` repairs opaque pixels outside the material rectangle and changes only alpha bytes inside it, preserving the panel RGB while writing clear/mid/strong alpha bands. |
| Deterministic repaint | The viewer does not skip arbitrary `FillRect` coverage; it rewrites the alpha surface every paint and returns `WVR_REDRAW` on full `WM_NCCALCSIZE`. |
| Diagnostics | The status line reports requested/read-back backdrop type, set/get `HRESULT`s, host-backdrop and alpha-redirection `HRESULT`s, OS build, edit HWND, and snap hit-test policy. |

The Settings glyph cycles semantic DWM requests. The visible material names are
the current Windows mapping; the example still reports requested/read-back enum
state because DWM owns the final composition policy.

| Name | Enum | Intent |
| --- | --- | --- |
| None | `DWMSBT_NONE` | Disable system backdrop material. |
| Mica | `DWMSBT_MAINWINDOW` | Long-lived main window. |
| Acrylic | `DWMSBT_TRANSIENTWINDOW` | Transient surface. |
| Mica Alt | `DWMSBT_TABBEDWINDOW` | Tabbed/titlebar surface. |

This file demonstrates why `12_backdrop.asm` stayed opaque: enabling redirected
alpha without owning the alpha channel can make unrelated GDI regions
transparent. The viewer bounds that risk to one deliberate material rectangle.
The mode-colored panel prevents a silent failure mode: even if the compositor
does not honor redirected alpha, the visible header and blended RGB bands still
change. When redirected alpha is honored, the same RGB bands also carry clear,
middle, and strong alpha values into the DWM redirection surface.

## Current Advanced Rungs And Planned Direction

The first nine rungs build the core CSD frame one mechanism at a time. The
remaining rungs are implemented, but they also define the current direction for
the example set: accessibility, animation, backdrop policy, tabbed captions,
responsive layout, RTL, multi-window ownership, custom shadows, and
DPI/manifest inspection. Each has a companion brief that records the design
boundary and the larger trajectory.

### `10_a11y_keyboard.asm`

Implemented source: `10_a11y_keyboard.asm`.
Implementation brief: `10_a11y_keyboard.md`.

- Deliver the accessibility rung for the 09-level owner-drawn buttons.
- Keep Alt+Space reaching the system menu.
- Handle F10/Alt keyboard entry, high contrast, and UI Automation exposure.
- Keep the native Search edit on its child-HWND provider.

Support surface: local caption keyboard state, explicit visual focus order,
high-contrast policy, static UIA provider objects, COM vtables, dynamic
UIA/OLE API loading, and shared command invocation by descriptor index.

### `11_caption_fade.md`

Implemented source: `11_caption_fade.asm`.
Implementation brief: `11_caption_fade.md`.

- Give the reserved `fadePhase` byte a timer-driven hover/press fade.
- Keep animation in the caption state/rendering layer, not in a render thread.
- Resolve current colors at paint time so live theme changes do not blend stale
  cached colors.

Support surface: a small animation helper over caption state rows, plus transient
`DC_BRUSH` fills for blended colors.

### `12_backdrop.md`

Implemented source: `12_backdrop.asm`.
Implementation brief: `12_backdrop.md`.

- Apply `DWMWA_SYSTEMBACKDROP_TYPE` on supported Windows 11 builds.
- Keep opaque `FillRect` coverage in the GDI paint path so size and drag redraws
  never preserve stale pixels.
- Keep theme, snap, child-edit, and system-menu behavior intact while cycling
  backdrop modes.

Support surface: a small backdrop policy row and DWM backdrop equates; visible
material is demonstrated separately by `19_backdrop_viewer.asm`.

### `19_backdrop_viewer.md`

Implemented source: `19_backdrop_viewer.asm`.
Implementation brief: `19_backdrop_viewer.md`.

- Add the extra DWM configuration needed for a visible backdrop-viewer surface.
- Keep the `12` caption/edit/snap/system-menu behavior so the paint policy is
  the only major teaching change.
- Render a color-grid software probe and mode-tinted diagnostic panel through a
  32bpp premultiplied-alpha DIB, repair alpha outside the viewer rectangle, and
  write clear/mid/strong alpha bands inside it before copying to the redirected
  window surface.

Support surface: `CSD_BACKDROP_VIEWER_CONFIG`, host-backdrop and redirection
alpha `DwmSetWindowAttribute` calls, software-composited viewer pixels, and a
bounded material viewport over a normal CSD frame.

### `13_caption_tabs.md`

Implemented source: `13_caption_tabs.asm`.
Implementation brief: `13_caption_tabs.md`.

- Promote the caption into a dynamic tab strip sharing space with window
  buttons.
- Replace the single drag rectangle with gaps around tab hit rectangles.
- Keep snap, system menu, DPI, theme, and child-caption behavior intact.

Support surface: `include/addon/csd/tabstrip.inc` with tab item, geometry,
state, and drag-map rows separate from fixed caption-control descriptors.

### `14_responsive_caption.md`

Implemented source: `14_responsive_caption.asm`.
Implementation brief: `14_responsive_caption.md`.

- Collapse optional caption rows by priority as width shrinks.
- Represent hidden app commands through an overflow button/menu.
- Use `WM_GETMINMAXINFO` once the caption reaches its irreducible width.

Support surface: responsive policy sidecar rows, overflow state, child-HWND
visibility rules, and DPI-aware minimum track-size calculation.

### `15_rtl_caption.asm`

Implemented source: `15_rtl_caption.asm`.
Implementation brief: `15_rtl_caption.md`.

- Treat leading/trailing as semantic start/end positions, not hard-coded
  left/right pixels.
- Keep physical resize hit-tests correct while caption controls mirror.
- Use RTL text/menu flags without replacing the real system menu.

Support surface: RTL layout policy, optional `WS_EX_LAYOUTRTL` equate support,
mirrored geometry, and RTL-aware text/menu drawing.

### `16_multi_window.asm`

Implemented source: `16_multi_window.asm`.
Implementation brief: `16_multi_window.md`.

- Move global CSD window state into a per-HWND context.
- Add an owned tool frame that shares policy but not hover, DPI, or child state.
- Propagate theme changes across live CSD windows while handling DPI per window.

Support surface: `CSD_WINDOW` context storage through `GWLP_USERDATA`, live-window
registration, and per-window resource lifetime.

### `17_custom_shadow.asm`

Implemented source: `17_custom_shadow.asm`.
Implementation brief: `17_custom_shadow.md`.

- Replace the one-pixel DWM margin trick with an app-owned layered shadow.
- Use `UpdateLayeredWindow` and premultiplied alpha.
- Document the behavior DWM normally supplies for free.

Support surface: shadow HWND ownership, 32bpp DIB generation, alpha constants,
and move/resize/DPI synchronization.

### `18_dpi_manifest_matrix`

Implemented directory: `18_dpi_manifest_matrix/`.
Implementation brief: `18_dpi_manifest_matrix.md`.

- Build the same caption under multiple DPI-awareness manifests.
- Extract/inspect the embedded manifest and PE headers.
- Record runtime DPI behavior in a comparison report.

Support surface: matrix build script, manifest variants, optional runtime log,
and `mt.exe`/`dumpbin` inspection output.
