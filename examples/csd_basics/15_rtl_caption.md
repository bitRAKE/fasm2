# `15_rtl_caption` Implementation Brief

This is the implementation brief for the Tier 2 right-to-left roadmap item. It
now accompanies the implemented rung. Its job is to define the mirroring,
reading-order, layout, hit-testing, and verification contract beside
`15_rtl_caption.asm`.

## Implementation Status

Implemented source: `15_rtl_caption.asm`.

The current implementation toggles `CSD_LAYOUT_RTL` with `F2`, mirrors fixed
caption rows and tab geometry directly into physical `RECT`s, keeps physical
resize hit-tests unchanged, uses `DT_RTLREADING` for tab titles, adds
`TPM_LAYOUTRTL` to the real system menu and app overflow popup, and keeps the
Search child edit neutral with local `WS_EX_NOINHERITLAYOUT`. It also carries
the `14_responsive_caption` tab/overflow keyboard and UIA provider forward:
visible dynamic tab targets and `caption.overflow` report mirrored bounding
rectangles, and Left/Right keyboard motion follows the mirrored tab flow.

## Goal

Prove whether the caption abstraction is truly semantic. `CsdCaptionLayout`
already names rows as leading and trailing, but the current implementation maps
leading to the physical left side and trailing to the physical right side. RTL
is the test that forces those words to mean "start" and "end" instead of
"left" and "right."

The rung should demonstrate:

- right-to-left caption layout toggled at runtime or startup;
- fixed caption controls mirrored by semantic leading/trailing roles;
- tab-strip and overflow geometry mirrored if `13`/`14` exist;
- title/tab text drawn with right-to-left reading order;
- the real system menu shown with RTL menu layout;
- snap-layout `HTMAXBUTTON` still working wherever the maximize glyph lands;
- physical resize edges still returning the correct physical `HT*` codes.

## Non-Goals

- Do not localize every English string.
- Do not implement bidirectional text shaping beyond what `DrawTextW` and the
  selected font already provide.
- Do not rely on `WS_EX_LAYOUTRTL` alone to fix owner-drawn geometry.
- Do not make the child Search edit a full RTL text editor.
- Do not change the command identity or descriptor order just to mirror pixels.

## Include And Equate Gate

The local USER equates already include:

| Surface | Local status |
| --- | --- |
| `WS_EX_RTLREADING = 02000h` | present in `include/equates/user64.inc` |
| `WS_EX_LEFTSCROLLBAR = 04000h` | present in `include/equates/user64.inc` |
| `DT_RTLREADING = 20000h` | present in `include/equates/user64.inc` |
| `TPM_LAYOUTRTL = 8000h` | present in `include/equates/user64.inc` |

Windows SDK `10.0.26100.0` also defines:

```asm
WS_EX_NOINHERITLAYOUT = 00100000h
WS_EX_LAYOUTRTL       = 00400000h
LAYOUT_RTL            = 00000001h
```

`WS_EX_LAYOUTRTL` is an important missing style for this study area. The
implemented rung defines it locally together with `WS_EX_NOINHERITLAYOUT`, but
does not rely on top-level `WS_EX_LAYOUTRTL` to fix owner-drawn geometry.

`SetLayout`/`GetLayout` and `LAYOUT_RTL` are GDI-level options. Treat them as an
optional experiment, not the first implementation path. The first readable
version should compute mirrored rectangles itself and use `DrawTextW` flags for
text reading order.

## Coordinate Model

Keep one rule explicit:

```text
geometry rows are stored in the same coordinate space used by hit-testing and painting
```

Do not store "logical RTL" rectangles that later need to be translated again.
The existing hit-test path compares client coordinates against `RECT.left`,
`RECT.right`, `RECT.top`, and `RECT.bottom`; that remains viable only if layout,
paint, and hit-test use the same coordinate basis.

Physical resize edges are not semantic leading/trailing edges:

| Screen edge | Return |
| --- | --- |
| Physical left edge | `HTLEFT` |
| Physical right edge | `HTRIGHT` |
| Physical top-left corner | `HTTOPLEFT` |
| Physical top-right corner | `HTTOPRIGHT` |

Mirroring the caption buttons must not swap the meaning of the screen edges.
Dragging the left border still resizes the left border.

## Layout Contract

Add an RTL flag to the layout input or to a small caption layout policy row:

```asm
CSD_LAYOUT_RTL = 01h
```

When `CSD_LAYOUT_RTL` is clear:

- leading rows consume space from `client.left + edge`;
- trailing rows consume space from `client.right - edge`;
- drag rectangles fill the space between them.

When `CSD_LAYOUT_RTL` is set:

- leading rows consume space from the physical right side;
- trailing rows consume space from the physical left side;
- row order within each group should remain semantic, not accidentally reversed
  by loop direction unless that is the desired UI policy.

For the current fixed descriptor table, expected visual result in RTL:

| Row | LTR position | RTL position |
| --- | --- | --- |
| System menu | left leading group | right leading group |
| Search / tabs / Settings | after System menu | before System menu, flowing inward from the right |
| Minimize / Maximize / Close | right trailing group | left trailing group |

If `13` tabs exist, tabs should flow from right to left. The active tab remains
semantically active; only geometry changes.

## Window Styles

The rung should make style effects visible, but not mysterious:

| Style | Use |
| --- | --- |
| `WS_EX_LAYOUTRTL` | Optional top-level mirror style to study USER/GDI mirroring behavior. |
| `WS_EX_RTLREADING` | Text reading order for windows that expose window text. |
| `WS_EX_LEFTSCROLLBAR` | Relevant for scrollable child controls, not the owner-drawn caption itself. |
| `WS_EX_NOINHERITLAYOUT` | Consider on child HWNDs if top-level mirroring would otherwise mirror child controls unexpectedly. |

For the Search child edit, decide deliberately:

- If the edit is part of the RTL demo, create it with RTL reading/alignment
  policy and verify caret/text behavior.
- If the edit remains a simple neutral search field, prevent accidental layout
  inheritance and keep the parent caption geometry mirrored around it.

The README for the rung should say which policy was chosen.

## Text And Menu Rendering

Use `DrawTextW` flags rather than hidden state:

| Text | LTR flags | RTL flags |
| --- | --- | --- |
| Caption title | `DT_LEFT` or existing alignment | `DT_RIGHT or DT_RTLREADING` |
| Tab title | existing left/center policy | right-aligned or centered plus `DT_RTLREADING` |
| Body/status explanatory text | may stay LTR English for the demo | use `DT_RTLREADING` only if the string is RTL-aware |

For the real system menu:

- keep using `GetSystemMenu` and `TrackPopupMenu`;
- add `TPM_LAYOUTRTL` when the caption is in RTL mode;
- anchor the popup to the System glyph's actual rectangle after mirroring.

Do not create a fake system menu to get RTL visuals.

## Hit-Testing Contract

Hit-testing should not know whether a row is visually left or right. It should
ask the geometry table.

Required behavior:

- `CaptionControlFromClientPoint` finds mirrored rectangles without special
  cases.
- tab hit-testing uses mirrored tab geometry from `13`.
- overflow hit-testing uses mirrored overflow geometry from `14`.
- the maximize descriptor still returns `snap_policy.maxHitCode` when the point
  is over the maximize row, even if that row is on the physical left side.
- drag rectangles are recomputed after mirroring, not inferred from old LTR
  positions.

The easiest bug to introduce is a mixed model: mirrored painting but LTR
hit-testing. The verification gate should explicitly test every visible control.

## Accessibility And Keyboard

The implemented source exposes the dynamic tab/overflow surface through a local
`WM_GETOBJECT` UIA provider:

- provider bounding rectangles follow the same mirrored geometry rows used by
  hit-testing and painting;
- keyboard Left/Right mirrors the tab flow in RTL mode, so Right from the active
  tab can reach the mirrored leading-side overflow glyph;
- reported names, roles, automation ids, and overflow `ItemStatus` do not
  change just because geometry is mirrored;
- Alt+Space still opens the real system menu;
- focus rectangle drawing follows mirrored tab and overflow rectangles.

This remains intentionally scoped. The fixed owner-drawn system/search/new-tab/
settings/window rows are not all exposed by a 15-specific descriptor provider;
`10_a11y_keyboard.asm` remains the fixed-caption provider example.

## Message Integration

Expected integration:

| Message/path | RTL responsibility |
| --- | --- |
| `WM_CREATE` | Initialize RTL policy and optional extended styles. |
| Settings or test command | Toggle RTL mode, relayout, recreate affected child HWNDs if needed, repaint. |
| `WM_SIZE` | Relayout with current RTL flag. |
| `WM_DPICHANGED` | Recompute mirrored layout in the new DPI. |
| `WM_NCHITTEST` | Keep physical resize edges; use mirrored geometry for caption/tabs. |
| `WM_CONTEXTMENU` / System glyph | Use real system menu with `TPM_LAYOUTRTL` in RTL mode. |
| `WM_PAINT` | Draw text with the chosen RTL flags and mirrored rectangles. |

## Verification

Build gate:

```cmd
cd C:\git\fasm2\examples\csd_basics
_build.cmd
```

Manual LTR/RTL comparison:

- LTR mode matches the previous rung.
- RTL mode mirrors semantic leading/trailing groups.
- Physical resize edges still resize the physical edge under the cursor.
- Every visible caption control hit-tests at its mirrored position.
- The maximize glyph still exposes snap layouts on supported Windows 11 builds.
- System menu opens from the mirrored System glyph and uses RTL popup layout.
- Drag gaps still return `HTCAPTION`.
- Search child policy is visible and documented: intentionally RTL or
  intentionally neutral.
- Text is not clipped after switching alignment/reading flags.
- In a narrow overflow state, `caption.overflow` reports the same hidden-command
  `ItemStatus` in LTR and RTL, but its UIA bounding rectangle moves to the
  mirrored leading side.
- In RTL narrow mode, F6 followed by Right focuses `caption.overflow`.

Observability gate:

- Use a temporary overlay or status text to show RTL flag, hit-test result, row
  index, and physical x/y.
- Query UIA before and after F2. In the current smoke test, narrow overflow
  moved from x=172 in LTR to x=480 in RTL and retained `Search hidden`.
- Compare with `examples/msgflood` to verify `HTLEFT` and `HTRIGHT` still
  correspond to physical edges, not semantic leading/trailing roles.

## Source Anchors

Current source points this brief now anchors:

| Source | Role |
| --- | --- |
| `include/equates/user64.inc:425` | Existing `WS_EX_RTLREADING` equate. |
| `include/equates/user64.inc:1750` | Existing `DT_RTLREADING` equate. |
| `include/equates/user64.inc:1878` | Existing `TPM_LAYOUTRTL` equate. |
| `15_rtl_caption.asm:50` | Local `CSD_LAYOUT_RTL`, `WS_EX_NOINHERITLAYOUT`, and `WS_EX_LAYOUTRTL` constants. |
| `15_rtl_caption.asm:227` | `rtl.root`, the UIA root automation id. |
| `15_rtl_caption.asm:412` | `layout_flags`, the runtime LTR/RTL state row. |
| `15_rtl_caption.asm:545` | `RtlPopupMenuFlags`, which adds `TPM_LAYOUTRTL` only while RTL is active. |
| `15_rtl_caption.asm:807` | `MoveTabKeyboardFocus`, the tab/overflow keyboard traversal core. |
| `15_rtl_caption.asm:1592` | `UiaOverflowStatusString`, the hidden-command `ItemStatus` source. |
| `15_rtl_caption.asm:1940` | `UiaRoot_ElementProviderFromPoint`, which hit-tests mirrored tabs and overflow. |
| `15_rtl_caption.asm:2145` | `UiaFragment_BoundingRectangle`, which returns mirrored provider bounds. |
| `15_rtl_caption.asm:2378` | `HandleGetObject`, the local raw UIA provider entry point. |
| `15_rtl_caption.asm:2495` | `LayoutTabs`, including the right-to-left tab geometry branch. |
| `15_rtl_caption.asm:2901` | `LayoutCaptionControls`, including mirrored leading/trailing placement. |
| `15_rtl_caption.asm:3008` | Search child creation with `WS_EX_NOINHERITLAYOUT`. |
| `15_rtl_caption.asm:3408` | `DrawTabs`, where RTL tab titles and focus rectangles use mirrored geometry. |
| `15_rtl_caption.asm:3643` | `ShowSystemMenu`, anchored to the mirrored System glyph. |
| `15_rtl_caption.asm:3865` | `ToggleLayoutDirection`, the F2 runtime toggle. |
| `15_rtl_caption.asm:4960` | `WM_KEYDOWN` routing for tab/overflow keyboard mode and F2. |
| `13_caption_tabs.md` | Tab geometry that must mirror if tabs exist. |
| `14_responsive_caption.md` | Overflow and minimum-width policy that must mirror if overflow exists. |

## References

- Microsoft Learn: [Extended Window Styles](https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles)
- Microsoft Learn: [`DrawText` function](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-drawtext)
