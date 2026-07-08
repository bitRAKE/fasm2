# `13_caption_tabs` Implementation Brief

This is the implementation brief for the first Tier 2 roadmap item. The
corresponding source now exists as `13_caption_tabs.asm`; this document remains
the tab-strip data model, layout contract, hit-testing, command-routing, and
verification contract for the rung.

## Goal

Promote the reclaimed caption area from a row of demo controls into the real
use case for client-side decoration: a tabbed title bar sharing space with
native window-management commands.

The rung should demonstrate:

- a dynamic tab array in the caption area;
- an app-owned new-tab button;
- close buttons inside individual tabs;
- active, hover, pressed, and inactive tab state;
- drag-region computation from the gaps around tabs, not from one continuous
  title rectangle;
- continued support for the real system menu, snap layouts, DPI, theme, and the
  owner-drawn window buttons.

## Non-Goals

- Do not build a browser framework or document model.
- Do not implement tab tear-off, cross-window tab drag, or tab reordering yet.
- Do not replace `CSD_CAPTION_DESCRIPTOR` with a tab-specific table.
- Do not make the tab strip a child HWND. This rung should keep the owner-drawn
  caption lesson visible.
- Do not hide hit-testing behind a framework router.

## Why This Rung Is Different

The current caption layout has three parallel arrays for fixed controls:

```text
descriptor row -> geometry row -> state row
```

`CsdCaptionLayout` places leading controls from the left, trailing controls from
the right, and writes one `caption_drag_rect` between them. That is enough for a
toolbar-like caption. It is not enough for tabs.

Tabbed captions need two related but separate surfaces:

| Surface | Owner | Hit-test behavior |
| --- | --- | --- |
| Window controls | existing `CSD_CAPTION_DESCRIPTOR` table | Buttons return `HTCLIENT`, except the maximize row can become `HTMAXBUTTON` for snap. |
| Tabs and new-tab button | new tab-strip state | Tabs return `HTCLIENT`; the gaps before, after, and between tabs return `HTCAPTION`. |

The teaching point is the drag map. A real tabbed caption is draggable in the
empty space around tabs, but not on tab bodies, close glyphs, the new-tab
button, the system menu glyph, or window buttons.

## Starting Point

`13_caption_tabs.asm` starts from `12_backdrop.asm`, the latest implemented rung
with theme, snap, DPI, child-caption behavior, animation, and backdrop policy.
That starting point preserves the earlier proof that:

- a caption row can host app-owned content;
- owner-drawn rows can be skipped when a child owns pixels;
- `HTMAXBUTTON` can be bridged back into caption state;
- DPI changes relayout caption geometry and child placement;
- caption fades and backdrop paint policy survive a more complex caption
  surface.

`10_a11y_keyboard` is the simpler fixed-descriptor accessibility rung. `13`
keeps that lesson local to a richer caption: the same UIA root now exposes
fixed owner-drawn caption controls plus the dynamic tab body/close targets, while
the Search edit remains a native child HWND.

## Data Model

Keep the fixed window-button descriptor table for system menu, new-tab,
settings, minimize, maximize, and close. Add a tab-strip module with its own
rows:

```asm
struct CSD_TAB_ITEM
  id              dd ?
  flags           dd ?
  titlep          dq ?
  iconGlyph       dd ?
  minWidthDip     dd ?
  preferredDip    dd ?
  maxWidthDip     dd ?
ends

struct CSD_TAB_GEOMETRY
  tabRect         RECT
  closeRect       RECT
  titleRect       RECT
ends

struct CSD_TAB_STATE
  value           db ?
  closeValue      db ?
  fadePhase       db ?
  reserved        db ?
ends

struct CSD_TABSTRIP
  itemp           dq ?
  geomp           dq ?
  statep          dq ?
  count           dd ?
  capacity        dd ?
  activeIndex     dd ?
  hotIndex        dd ?
  hotPart         dd ?
  pressedIndex    dd ?
  pressedPart     dd ?
ends
```

The implementation can start with a small heap-backed vector or a fixed-capacity
array plus `count`. A fixed capacity is acceptable for the teaching rung if the
source names that choice; the important lesson is that tabs are a dynamic row
set whose count changes at runtime.

Tab parts:

| Part | Meaning |
| --- | --- |
| `CSD_TAB_PART_BODY` | Selecting or dragging within a tab body. |
| `CSD_TAB_PART_CLOSE` | Closing a tab. |
| `CSD_TAB_PART_NEW` | App-owned new-tab button. |
| `CSD_TAB_PART_GAP` | Empty draggable caption space. |

Do not overload `CSD_CAPTION_DESCRIPTOR.flags` with tab-specific state. The
window-button table and tab-strip table should meet only at layout and
hit-testing boundaries.

## Layout Contract

The layout pass has three inputs:

1. current client rect and DPI;
2. leading fixed caption controls;
3. trailing fixed window controls.

It should produce:

- fixed caption-control geometry, as today;
- tab rectangles between the leading controls and trailing window buttons;
- a list of drag rectangles for gaps not occupied by tabs or controls;
- optional overflow state for tabs that do not fit.

Suggested layout sequence:

1. Run a fixed-control layout to reserve the system menu, new-tab button,
   settings, and trailing window buttons.
2. Compute `tabBand.left` after the leading fixed controls plus drag padding.
3. Compute `tabBand.right` before the trailing controls minus drag padding.
4. Allocate tab widths from active tab outward, clamped between min and max DIP
   widths.
5. Write one `CSD_TAB_GEOMETRY` per visible tab.
6. Write gap rectangles into a small drag-region array.

The existing single `caption_drag_rect` should become either:

```asm
caption_drag_rects RECT CSD_CAPTION_DRAG_RECT_MAX dup ?
caption_drag_rect_count dd ?
```

or a tab-strip-owned drag array. The hit-test path should ask "is this point in
any drag rect?" instead of checking one rectangle.

## Hit-Testing Contract

Hit-test precedence matters:

1. Resize edges and corners still win first.
2. Fixed caption controls win next.
3. Tab close buttons win before tab bodies.
4. Tab bodies win before gaps.
5. Drag gaps return `HTCAPTION`.
6. The remaining body returns `HTCLIENT`.

The maximize button remains special: when the snap policy is capable, it may
return `HTMAXBUTTON` and move into the non-client message channel. Tabs must not
return `HTMAXBUTTON`; they are app-owned client content.

The hit-test helper should return both "what Windows should see" and "what app
part is under the cursor." A compact shape:

```asm
struct CSD_CAPTION_HIT
  htCode          dd ?
  captionIndex    dd ?
  tabIndex        dd ?
  tabPart         dd ?
ends
```

This prevents later mouse handlers from re-running several incompatible
point-to-row searches.

## Input And Command Routing

The existing caption button flow uses:

- hot index;
- pressed index;
- capture on mouse down;
- command only when press and release match.

Tabs should use the same semantics:

| Action | Behavior |
| --- | --- |
| Click tab body | Select the tab if press and release hit the same tab body. |
| Click close glyph | Close the tab if press and release hit the same tab close part. |
| Click new-tab button | Append a tab, select it, relayout, repaint. |
| Drag off close glyph | Cancel close command. |
| Mouse leave | Clear hot tab and hot close state. |
| Deactivate | Clear hot/pressed state and paint inactive. |

Closing the active tab should select a neighbor before removing the row, then
compact the item, geometry, and state arrays together. The table-spine lesson
still applies: dynamic rows must keep their parallel arrays in sync.

## Rendering Contract

Use the existing `font_icons` dependency for glyphs:

- tab close glyph uses the icon font;
- new-tab button uses a plus/add glyph;
- optional tab icon uses the same `FontIcon_DrawGlyph` helper as caption
  buttons.

Draw tabs as owner-drawn GDI shapes and text:

- active tab fill stands out from inactive tabs;
- inactive tabs are legible in light and dark themes;
- close glyph appears only on the hot tab or active tab, unless the design
  deliberately shows it always;
- tab titles ellipsize within `titleRect`;
- active tab connects visually to the content body or material backdrop.

Do not add a nested card/panel look inside the caption. The tab strip is the
caption surface, not a framed widget floating on top of it.

## Responsive Behavior

This rung should implement a minimal fit policy so the example is usable at
small widths, but leave full overflow menus to `14_responsive_caption`.

Minimum behavior:

- tabs shrink down to `minWidthDip`;
- the active tab stays visible as long as any tab can fit;
- non-active tabs are clipped from the far side when width is exhausted;
- the new-tab button may hide after all visible tabs reach minimum width;
- the remaining draggable gaps are recomputed after clipping.

Do not silently overlap tabs with trailing window buttons. If the tab band is
too narrow, hide tabs or enforce a minimum window width in the next rung.

## Accessibility And Keyboard

`13_caption_tabs.asm` now implements a keyboard mode over the owner-drawn
caption targets:

- `F6` enters tab keyboard mode on the active visible tab body;
- Left/Right walks fixed caption buttons, visible tab bodies, and close glyphs
  in visual order;
- Ctrl+Tab and Ctrl+Shift+Tab jump between visible tab bodies;
- Enter/Space invokes the focused fixed button, tab body, or close glyph;
- Delete closes the focused tab when more than one tab remains;
- Escape leaves tab keyboard mode before the demo-level Escape-to-close path;
- Alt+Space continues to open the real system menu.

The UI Automation surface exposes fixed caption controls as Button invoke
targets (`caption.system`, `caption.newtab`, `caption.settings`,
`caption.minimize`, `caption.maximize`, and `caption.close`). Each visible tab
body reports TabItem, name, selected state, bounds, Invoke, and SelectionItem;
each visible close glyph is a separate Button invoke target. The Search child
HWND remains visible in the UIA tree through its native child-window provider
and is not duplicated by the owner-drawn provider.

The UIA support mechanics are shared through `include/addon/csd/uia.inc`, and
the repeated tabbed-caption demo support now lives in
`csd_example_support.inc`. This rung still owns the caption/tab element policy,
names, navigation order, and command dispatch.

## Message Integration

Expected integration points:

| Message/path | Tab-strip responsibility |
| --- | --- |
| `WM_CREATE` | Initialize default tabs and active index. |
| `WM_SIZE` | Relayout fixed controls, tabs, drag rectangles, and child controls. |
| `WM_DPICHANGED` | Recompute all tab widths and text/icon rectangles at the new DPI. |
| `WM_NCHITTEST` | Use tab hit-test plus drag rectangles after resize checks. |
| `WM_MOUSEMOVE` / `WM_MOUSELEAVE` | Update tab and close-button hot state. |
| `WM_LBUTTONDOWN` / `WM_LBUTTONUP` | Capture, press/release matching, select/close/new-tab. |
| `WM_NCMOUSEMOVE` / `WM_NCLBUTTON*` | Continue routing only the maximize snap path. |
| `WM_KEYDOWN` / `WM_SYSKEYDOWN` | Route tab keyboard mode and keep Alt+Space on the real system menu. |
| `WM_GETOBJECT` | Return the local caption/tab UIA provider root for fixed buttons, visible tab bodies, and close glyph elements. |
| `WM_ACTIVATE` | Clear hot/pressed tab state when inactive. |
| `WM_COMMAND` | Route app commands such as new-tab or close-tab if implemented as command IDs. |

## Verification

Build gate:

```cmd
cd C:\git\fasm2\examples\csd_basics
_build.cmd
```

Manual behavior gate:

- New-tab appends a tab, selects it, and relayouts without moving window
  buttons.
- Closing a tab compacts item, geometry, and state rows together.
- Pressing a close glyph and releasing elsewhere cancels close.
- Clicking a tab selects it; clicking a gap moves the window.
- Dragging from empty space between tabs returns `HTCAPTION`.
- Dragging from a tab body does not move the window.
- Maximize still exposes the snap flyout on Windows 11 through `HTMAXBUTTON`.
- System menu and Alt+Space still reach the real system menu.
- F6 enters tab keyboard mode, Left/Right walks fixed buttons and visible tab
  targets, Ctrl+Tab jumps between tab bodies, Enter/Space invokes, Delete closes
  tabs only, and Escape exits tab keyboard mode before closing the demo window.
- UIA clients see fixed caption buttons, one TabItem per visible tab body, one
  Button per visible close glyph, SelectionItem state on tab bodies, Invoke on
  fixed buttons/tab bodies/close glyphs, and the native Search child HWND
  separately.
- DPI changes keep tab text and glyphs close to system-menu text size.
- Narrow windows shrink or clip tabs without overlap.

Observability gate:

- Pair with `examples/msgflood` to compare normal `HTCAPTION` drag traffic with
  tab-body `HTCLIENT` traffic.
- Add temporary status text or a debug overlay showing tab index, part, and
  returned `HT*` value while testing.

## Source Anchors

Implemented source points for this brief:

| Source | Role |
| --- | --- |
| `include/addon/csd/caption.inc:25` | Fixed caption-control descriptor row remains the window-button/app-button table. |
| `include/addon/csd/caption.inc:50` | Leading/trailing fixed-control layout still reserves the tab band. |
| `include/addon/csd/caption.inc:132` | Point-to-caption-control search still handles fixed controls before tabs. |
| `include/addon/csd/child.inc:3` | Child placement pattern from the Search row. |
| `include/addon/csd/tabstrip.inc:13` | `CSD_TAB_ITEM` starts the dynamic tab item/geometry/state row set. |
| `include/addon/csd/uia.inc:63` | `CSD_UIA_PROVIDER` stores the shared COM interface slots and provider state. |
| `include/addon/csd/uia.inc:129` | `CsdUiaLoadApis` dynamically loads UIAutomationCore and OLE Automation entry points. |
| `include/addon/csd/uia.inc:176` | `CsdUiaInitProvider` initializes one provider object with consumer-owned vtables. |
| `include/addon/csd/uia.inc:330` | `CsdUiaRuntimeIdForSlot` builds the provider runtime ID safe array. |
| `include/addon/csd/uia.inc:388` | `CsdUiaFragment_GetRuntimeId` adapts provider slots to UIA fragment runtime IDs. |
| `include/addon/csd/uia.inc:405` | `CsdUiaSelection_SelectionContainer` returns the root provider for SelectionItem elements. |
| `include/addon/csd/uia.inc:417` | `CsdUiaHandleGetObject` handles the mechanical `WM_GETOBJECT/UiaRootObjectId` return path. |
| `csd_example_support.inc:105` | `RefreshThemeAndFrame` centralizes the demo theme/backdrop repaint path. |
| `csd_example_support.inc:140` | `AddCaptionDragRect` writes the shared multi-rectangle drag map. |
| `csd_example_support.inc:249` | `CaptionFillColorForState` keeps the shared demo button/close fill policy. |
| `csd_example_support.inc:367` | `SetTabHot` updates shared tab hot state and reapplies tab visual state. |
| `csd_example_support.inc:624` | `HandleDpiChanged` applies the suggested DPI rectangle and relayouts caption children. |
| `13_caption_tabs.asm:508` | `EnterTabKeyboard` starts keyboard mode on the active visible tab. |
| `13_caption_tabs.asm:544` | `MoveTabKeyboardFocus` walks fixed controls, visible tab bodies, and close glyphs. |
| `13_caption_tabs.asm:696` | `InvokeTabKeyboardFocus` invokes the focused fixed control, tab body, or close glyph. |
| `13_caption_tabs.asm:731` | `HandleTabKeyDown` routes F6, arrows, Ctrl+Tab, Enter/Space, Delete, and Escape. |
| `13_caption_tabs.asm:815` | `HandleTabSysKeyDown` keeps Alt+Space on the real system menu. |
| `13_caption_tabs.asm:828` | `InitUiaProviders` binds the shared UIA helpers to the example-owned provider tree. |
| `13_caption_tabs.asm:1367` | `UiaSimple_GetPropertyValue` reports names, IDs, control types, focusability, focus, and selected state. |
| `13_caption_tabs.asm:1498` | `UiaRoot_ElementProviderFromPoint` maps screen points back to fixed-control, tab-body, or close providers. |
| `13_caption_tabs.asm:1584` | `UiaFragment_Navigate` filters the provider tree to currently visible owner-drawn caption targets. |
| `13_caption_tabs.asm:1652` | `UiaFragment_BoundingRectangle` reports fixed-control, tab-body, and close-glyph bounds in screen coordinates. |
| `13_caption_tabs.asm:1788` | `UiaInvoke_Invoke` invokes the focused caption target through the same command path as keyboard mode. |
| `13_caption_tabs.asm:1814` | `UiaSelection_Select` gives visible tab bodies a SelectionItem select path. |
| `13_caption_tabs.asm:1879` | `HandleGetObject` returns the local caption/tab provider root for `WM_GETOBJECT/UiaRootObjectId`. |
| `13_caption_tabs.asm:1927` | `LayoutTabs` computes visible tabs and drag-gap rectangles. |
| `13_caption_tabs.asm:2160` | `TabPartFromClientPoint` distinguishes tab close glyphs from tab bodies. |
| `13_caption_tabs.asm:2210` | `PointInCaptionDragRects` tests the multi-rectangle drag map. |
| `13_caption_tabs.asm:2325` | `DrawTabs` renders tab fills, text, close glyphs, and keyboard focus. |
| `13_caption_tabs.asm:2598` | `HitTestCsdFrame` applies resize, fixed-control, tab, gap, and client precedence. |
| `13_caption_tabs.asm:2728` | `AddDemoTab` appends a tab and selects it. |
| `13_caption_tabs.asm:2749` | `CloseDemoTab` compacts item and state rows after close. |
| `13_caption_tabs.asm:2810` | `InvokeCaptionIndex` dispatches fixed caption controls for keyboard/UIA. |
| `13_caption_tabs.asm:2973` | `HandleCaptionMouseMove` updates fixed-control and tab hover state. |
| `13_caption_tabs.asm:3037` | `HandleCaptionLButtonDown` captures tab body/close presses. |
| `13_caption_tabs.asm:3098` | `HandleCaptionLButtonUp` dispatches matched tab select/close commands. |
