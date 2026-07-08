# `14_responsive_caption` Implementation Brief

This is the implementation brief for the Tier 2 overflow/collapse roadmap item.
The implemented rung is `14_responsive_caption.asm`; this file keeps the
responsive layout policy, overflow state, minimum-size enforcement, and
verification gates readable beside the source.

## Implementation Status

Implemented source: `14_responsive_caption.asm`.

The current implementation uses a sidecar `CSD_CAPTION_RESPONSIVE` table,
`caption_visible[]` runtime rows, an app-owned overflow popup, child-HWND
show/hide handling for Search, and a `WM_GETMINMAXINFO` handler that writes
`MINMAXINFO.ptMinTrackSize` after DPI-aware frame adjustment. It also carries
the `13_caption_tabs` keyboard/UIA tab model forward and extends it with a
visible overflow-button target whose UIA `ItemStatus` names the hidden caption
commands.

## Goal

Teach how a client-side-decorated caption behaves when the window becomes too
narrow for every app-owned control and tab to remain visible.

The rung should demonstrate:

- priority-based collapse over caption descriptors and tab rows;
- an app-owned overflow button/menu for hidden caption actions;
- tab shrink and tab clipping policy;
- hiding and restoring child HWNDs such as the Search edit;
- `WM_GETMINMAXINFO` enforcement when the caption reaches its irreducible
  minimum width;
- no overlap between tabs, child controls, and window buttons.

## Non-Goals

- Do not implement a full browser-grade tab overflow scroller.
- Do not add tab drag reordering or tear-off.
- Do not replace the real system menu with the overflow menu.
- Do not make every earlier rung responsive.
- Do not move all metrics into shared source just to implement this rung.

## Responsive Policy

The responsive caption has two layers:

| Layer | Responsibility |
| --- | --- |
| Collapse policy | Decide which optional caption rows are visible, hidden, or represented by overflow. |
| Minimum tracking | Prevent the window from shrinking beyond the width required for the non-optional surface. |

These are separate. Collapse policy should run first. `WM_GETMINMAXINFO` is the
last line of defense after no more meaningful content can be hidden.

## Priority Model

Do not overload `CSD_CAPTION_DESCRIPTOR.flags` with every responsive decision.
Add a sidecar policy table or promote the descriptor only after the shape is
stable:

```asm
struct CSD_CAPTION_RESPONSIVE
  priority        dd ?
  minWidthDip     dd ?
  overflowCommand dd ?
  flags           dd ?
ends
```

Suggested flags:

| Flag | Meaning |
| --- | --- |
| `CSD_RESP_REQUIRED` | This row is never hidden by collapse policy. |
| `CSD_RESP_CAN_HIDE` | This row can disappear without an overflow command. |
| `CSD_RESP_TO_OVERFLOW` | This row moves into the overflow menu when hidden. |
| `CSD_RESP_CHILD_HWND` | This row owns a child HWND that must be shown/hidden with visibility. |

Suggested priority order, from hardest to hide to easiest:

| Surface | Policy |
| --- | --- |
| Close, maximize, minimize | Required. Native window-management affordances stay visible. |
| System menu glyph | Required unless the keyboard/system-menu path is separately proven. |
| Active tab | Required while any tab can fit. |
| New-tab button | Hide after tabs reach minimum width; optionally move to overflow. |
| Settings | Move to overflow. |
| Search edit child | Hide or move to overflow; also hide the child HWND. |
| Inactive tabs | Shrink first, then clip/hide from farthest side. |

This rung builds on `13_caption_tabs`; the simpler Search/Settings-only collapse
shape is now just a fallback idea for future experiments, not this source.

## Layout Algorithm

Responsive layout should be deterministic and inspectable. A useful shape:

1. Convert all DIP widths to pixels for the current DPI.
2. Reserve trailing window buttons first.
3. Reserve required leading controls.
4. Compute the remaining caption band.
5. Fit tabs using preferred widths, then minimum widths.
6. Hide optional rows by priority until the required surface fits.
7. Populate overflow menu rows for hidden commands marked
   `CSD_RESP_TO_OVERFLOW`.
8. Recompute drag rectangles after visibility decisions.

Visible rows get normal geometry. Hidden rows should get empty geometry:

```text
left = right = top = bottom = 0
```

That keeps existing point-to-geometry searches from finding hidden rows.

For tab strips, keep a visible range:

```asm
visibleFirst dd ?
visibleCount dd ?
hiddenBefore dd ?
hiddenAfter  dd ?
```

The active tab should be included in the visible range if any tab can be shown.
When inactive tabs are hidden, the overflow status can show counts or a menu.

## Overflow UI

The overflow control is app-owned caption UI, not the real system menu.

Recommended shape:

- add one fixed descriptor row for overflow, probably leading and near Settings;
- hide it while there are no overflow items;
- show it when at least one `CSD_RESP_TO_OVERFLOW` item is hidden;
- click opens a popup menu containing app commands for hidden rows.

The real system menu remains owned by the System glyph and Alt+Space. Do not mix
`SC_*` window-management items into the app overflow menu.

The overflow button should be excluded from the drag rectangles just like other
caption controls.

## Child HWND Handling

The Search edit from `09` is the hardest existing row because it owns a child
window. When responsive policy hides that row:

- call `ShowWindow(hSearchEdit, SW_HIDE)`;
- do not include its row in hit-testing;
- avoid moving it into a zero-width rect while visible;
- if it has focus, move focus back to the main window or the next reachable
  caption control;
- when visible again, call `ShowWindow(hSearchEdit, SW_SHOW)` and then
  `CsdCaptionMoveChild`.

Parent painting should still skip `CSD_CAPTION_CHILD` rows only when the child
is visible and owns pixels. If the row is hidden, it should not reserve caption
space or hit-test as a child slot.

## Minimum Track Size

`WM_GETMINMAXINFO` is available in the local USER equates, and `MINMAXINFO`
already exposes `ptMinTrackSize`.

The handler should compute a minimum client size in DIPs, convert it to pixels,
then adjust it to a top-level window size:

1. Sum the irreducible caption width:
   - resize edge allowance;
   - required leading rows;
   - required trailing rows;
   - minimum active-tab width if tabs exist;
   - drag padding that must remain visible.
2. Choose a body/content minimum height.
3. Build a client `RECT` at that size.
4. Call `CsdAdjustWindowRectForDpi` with the current style/exstyle and DPI.
5. Store the adjusted width/height in `MINMAXINFO.ptMinTrackSize`.

This is a window-size contract, not a paint hint. It prevents Windows from
offering impossible sizes during interactive resize.

Local examples with the basic pattern:

- `examples/font_icons/glyphset.asm` handles `WM_GETMINMAXINFO` and writes
  `MINMAXINFO.ptMinTrackSize`.
- `examples/font_icons/uwpchar.asm` uses the same message for a larger tool UI.

## Hit-Testing Contract

The hit-test order from `13` still applies, with one extra rule: hidden geometry
must be unhit-testable.

Precedence:

1. Resize edges and corners.
2. Visible fixed caption controls.
3. Visible tab close buttons.
4. Visible tab bodies.
5. Visible overflow button.
6. Drag rectangles.
7. Body client area.

Do not let collapsed controls leave stale rectangles behind. A hidden Search
row that still returns `HTCLIENT` is worse than clipping because it creates an
invisible input blocker.

## Accessibility And Keyboard

Responsive state must be visible to accessibility and keyboard routing:

- hidden owner-drawn rows should not be reported as visible controls;
- overflowed commands should be reachable through the overflow button/menu;
- hidden child HWNDs should not remain in tab order;
- keyboard focus should move if the focused row collapses;
- active tab selection should remain reachable even when inactive tabs clip.

The implemented source reports visible tab bodies/close glyphs plus the
overflow glyph through a local `WM_GETOBJECT` UIA provider. The overflow provider
uses `caption.overflow` as its automation id and `UIA_ItemStatusPropertyId` to
publish the current hidden-command set, for example `Search hidden`. F6 enters
keyboard mode, Left/Right include the overflow glyph when visible, Enter/Space
invoke the focused target, Delete closes only tab targets, and Alt+Space still
opens the real system menu.

This is still not a complete provider for every fixed owner-drawn caption row in
this richer tabbed caption. `10_a11y_keyboard.asm` remains the fixed-descriptor
provider example; this rung proves the responsive dynamic-target policy.

## Message Integration

Expected integration:

| Message/path | Responsive responsibility |
| --- | --- |
| `WM_CREATE` | Initialize responsive policy and overflow state. |
| `WM_SIZE` | Run collapse policy, relayout controls/tabs/children, repaint. |
| `WM_GETMINMAXINFO` | Write minimum track size after DPI-aware frame adjustment. |
| `WM_DPICHANGED` | Recompute minimums and responsive layout at the new DPI. |
| `WM_NCHITTEST` | Ignore hidden rows and use recomputed drag rectangles. |
| `WM_COMMAND` | Route overflow menu commands back to the same app command handlers. |
| `WM_MOUSEMOVE` / `WM_MOUSELEAVE` | Clear hot state when the hot row collapses. |
| `WM_ACTIVATE` | Clear hot/pressed state as before. |

## Verification

Build gate:

```cmd
cd C:\git\fasm2\examples\csd_basics
_build.cmd
```

Manual behavior gate:

- Narrowing the window hides optional rows in the documented priority order.
- Hidden controls do not hit-test, hover, press, or receive keyboard focus.
- The Search child HWND hides and shows without flicker or stale focus.
- Overflow appears only when it has hidden commands to represent.
- Overflow commands route to the same handlers as visible controls.
- F6 and Left/Right can reach the visible overflow glyph.
- UIA reports `caption.overflow` only while it is visible and its `ItemStatus`
  names the hidden Search/New Tab/Settings command set.
- Window buttons never overlap tabs, Search, Settings, or overflow.
- The active tab remains visible while a tab can fit.
- `WM_GETMINMAXINFO` prevents shrinking below the irreducible caption width.
- DPI changes recompute collapse decisions and minimum track size.
- Drag rectangles remain available in visible gaps after every collapse state.

Observability gate:

- Use status text or debug overlay to show visible/hidden/overflow row counts.
- Narrow to an overflow state and query UIA for `caption.overflow`; expected
  status text includes the hidden command names, such as `Search hidden`.
- Use `examples/msgflood` to confirm resize traffic includes
  `WM_GETMINMAXINFO` and that narrow-width dragging still returns the expected
  `HT*` results.

## Source Anchors

Current source points this brief now anchors:

| Source | Role |
| --- | --- |
| `include/equates/user64.inc:106` | `MINMAXINFO` structure. |
| `include/equates/user64.inc:555` | `WM_GETMINMAXINFO` equate. |
| `include/addon/csd/caption.inc:50` | Current fixed-control layout, before visibility policy. |
| `include/addon/csd/caption.inc:132` | Current geometry hit-test; hidden rows must not match. |
| `include/addon/csd/child.inc:3` | Search child placement helper used only after the row is visible. |
| `14_responsive_caption.asm:288` | Descriptor rows, including the overflow button inserted before Settings. |
| `14_responsive_caption.asm:331` | Responsive sidecar rows for required, overflow, and child-HWND policy. |
| `14_responsive_caption.asm:629` | `IsTabKeyboardTargetVisible`, including the overflow keyboard target. |
| `14_responsive_caption.asm:1543` | `UiaOverflowStatusString`, the hidden-command `ItemStatus` source. |
| `14_responsive_caption.asm:1890` | `UiaRoot_ElementProviderFromPoint`, which hit-tests tabs and overflow. |
| `14_responsive_caption.asm:2095` | `UiaFragment_BoundingRectangle`, which returns overflow button bounds. |
| `14_responsive_caption.asm:2328` | `HandleGetObject`, the local raw UIA provider entry point. |
| `14_responsive_caption.asm:2686` | `ComputeResponsivePolicy`, the priority collapse pass. |
| `14_responsive_caption.asm:2780` | `LayoutCaptionControls`, which clears hidden geometry before hit-testing. |
| `14_responsive_caption.asm:2886` | `LayoutCaptionChildren`, which hides or shows the Search `EDIT`. |
| `14_responsive_caption.asm:3183` | `DrawCaptionControls`, including overflow keyboard-focus rendering. |
| `14_responsive_caption.asm:3833` | `ShowOverflowMenu`, the app-owned overflow command menu. |
| `14_responsive_caption.asm:4487` | `HandleGetMinMaxInfo`, the DPI-aware minimum track-size handler. |
| `13_caption_tabs.md` | Tab-strip and multi-drag-rect contract this rung builds on. |
