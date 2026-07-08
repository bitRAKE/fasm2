# CSD Concepts

This companion explains the "why" behind the CSD example set without changing
the example sources. Keep it open beside `README.md` and the `.asm` files:

- `README.md` says what each rung owns.
- `Client-Side Decoration.md` gives the conceptual survey.
- this file points from concepts to source anchors and explains the small
  decisions that are easy to copy without understanding.

## Caption Table Spine

The reusable shape starts in `04_caption_controls.asm` and is shared by later
rungs:

```text
caption_descriptors[]        caption_geometry[]          caption_state[]
---------------------        ------------------          ---------------
id                           RECT in physical pixels     value
glyph                        written by layout           fadePhase
command                                                fromValue
hitCode                                                animFlags
flags
widthDip
```

The three arrays intentionally answer different questions:

- descriptor: what this control is and what command it represents;
- geometry: where that descriptor landed after the current layout pass;
- state: what transient input/rendering state is active now.

Worked row: the Close button.

```text
CSD_CAPTION_DESCRIPTOR
  id       = ID_CAPTION_CLOSE
  glyph    = ICON_CLOSE
  command  = SC_CLOSE
  hitCode  = HTCLIENT
  flags    = CSD_CAPTION_ALIGN_TRAILING |
             CSD_CAPTION_DANGER |
             CSD_CAPTION_SYSCMD
  widthDip = CSD_BUTTON_WIDTH_DIP
        |
        v
CsdCaptionLayout writes one trailing RECT in caption_geometry.
        |
        +--> DrawCaptionControls chooses the danger brushes and draws ICON_CLOSE.
        |
        +--> CaptionControlFromClientPoint finds the row under the cursor.
        |
        +--> HandleCaptionClientClick dispatches WM_SYSCOMMAND/SC_CLOSE.
```

The same descriptor therefore drives layout, hit-testing, rendering, and command
routing. Later rungs add DPI, theme, state, snap policy, and child-window
ownership without replacing the table.

## Concept-To-Source Index

Line numbers reflect the current source layout and are meant as reading aids;
the label names are the stable anchors.

| Concept | Current source anchor | What to inspect |
| --- | --- | --- |
| Reclaim the frame | `01_nccalc_frame.asm:188`, `WindowProc/wnd_nccalcsize` | The handler accepts the proposed window rect as client area and requests redraw on the full `WM_NCCALCSIZE` form. |
| Paint every pixel | `01_nccalc_frame.asm:74`, `PaintFrame`; `01_nccalc_frame.asm:211`, `wnd_erasebkgnd` | `WM_PAINT` owns the body, caption band, and app-drawn edge. |
| Restore native input | `02_nchittest_frame.asm:162`, `HitTestCsdFrame` | Standard `HT*` values hand drag, resize, and snap loops back to USER. |
| Preserve DWM behavior | `03_dwm_frame.asm:31`, `dwm_margins`; `03_dwm_frame.asm:306`, `wnd_create` | One-pixel DWM margins bring shadow and compositor animations back. |
| Use the caption surface | `04_caption_controls.asm:59-115`, `caption_descriptors`/geometry/state | The caption row becomes data-driven app UI plus system commands. |
| Reduced drag area | `04_caption_controls.asm:347`, `HitTestCsdFrame` | Only the strip not occupied by controls returns `HTCAPTION`. |
| System menu support | `04_caption_controls.asm:303`, `ShowSystemMenu` | The menu glyph opens the real system menu, not an app imitation. |
| DPI-dependent geometry | `05_dpi_frame.asm:126`, `SetDpiState`; `05_dpi_frame.asm:193`, `LayoutCaptionControls`; `include/addon/csd/caption.inc:50`, `CsdCaptionLayout` | DIPs stay in descriptors; physical rectangles are recomputed per DPI. |
| DWM theme state | `06_dwm_theme.asm:205`, `RefreshThemeAndFrame`; `include/addon/csd/theme.inc:38`, `CsdThemeRefresh` | DWM accent/dark-mode inputs become one `CSD_THEME` row. |
| Hover/pressed/inactive state | `07_caption_state.asm:787`, `HandleCaptionLButtonDown`; `07_caption_state.asm:818`, `HandleCaptionLButtonUp`; `include/addon/csd/state.inc:25`, `CsdCaptionStateApply` | Client input mutates the state column and redraws owner-drawn glyphs. |
| Snap-layout channel switch | `08_snap_layouts.asm:921`, `HandleSnapNcMouseMove`; `08_snap_layouts.asm:961`, `HandleSnapNcLButtonDown`; `08_snap_layouts.asm:980`, `HandleSnapNcLButtonUp` | `HTMAXBUTTON` moves one glyph into non-client message traffic. |
| Child HWND in caption | `09_embed_edit.asm:279`, `CreateCaptionChildren`; `09_embed_edit.asm:299`, `LayoutCaptionChildren`; `09_embed_edit.asm:344`, `DrawCaptionControls` | The Search row becomes a child `EDIT`; parent painting skips that row. |
| Keyboard caption access | `10_a11y_keyboard.asm:1358`, `EnterCaptionKeyboard`; `10_a11y_keyboard.asm:1387`, `MoveCaptionKeyboardFocus`; `10_a11y_keyboard.asm:1983`, `InvokeCaptionKeyboardFocus` | Owner-drawn caption buttons become reachable from F10/Alt, arrows, Enter/Space, and Alt+Space without replacing the real system menu. |
| UI Automation caption provider | `10_a11y_keyboard.asm:549`, `LoadUiaApis`; `10_a11y_keyboard.asm:835`, `UiaProvider_QueryInterface`; `10_a11y_keyboard.asm:1330`, `HandleGetObject` | `WM_GETOBJECT` exposes the owner-drawn caption buttons as UIA Button elements while the Search edit remains a native child provider. |
| High-contrast caption colors | `10_a11y_keyboard.asm:380`, `RefreshHighContrast`; `10_a11y_keyboard.asm:1542`, focus rendering | `SPI_GETHIGHCONTRAST` switches demo colors to system colors and keeps keyboard focus visible. |
| Caption fade animation | `11_caption_fade.asm:348`, `CsdBlendColorref`; `11_caption_fade.asm:466`, `DrawCaptionControls`; `11_caption_fade.asm:937`, `HandleCaptionFadeTimer` | State rows blend from `fromValue` to `value` through `fadePhase` until no active rows remain. |
| DWM system backdrop | `include/addon/csd/backdrop.inc:37`, `CsdBackdropProbePolicy`; `include/addon/csd/backdrop.inc:139`, `CsdBackdropApply`; `12_backdrop.asm:587`, `PaintFrame` | A separate policy row gates and reads back `DWMWA_SYSTEMBACKDROP_TYPE`; the GDI paint path still covers every pixel. |
| Visible backdrop viewport | `include/addon/csd/backdrop.inc:196`, `CsdBackdropViewerApply`; `19_backdrop_viewer.asm`, `RenderViewerPanelPixels`; `19_backdrop_viewer.asm`, `DrawViewerModePanel`; `19_backdrop_viewer.asm`, `ApplyViewerSurfaceAlpha`; `19_backdrop_viewer.asm`, `PaintFrame` | The viewer applies extra DWM configuration, renders a blended software probe and mode-colored diagnostic panel through a 32bpp DIB, and repairs redirected alpha to opaque outside the material rectangle. |
| Caption tabs | `include/addon/csd/tabstrip.inc:13`, `CSD_TAB_ITEM`; `13_caption_tabs.asm:2204`, `LayoutTabs`; `13_caption_tabs.asm:3051`, `HitTestCsdFrame` | A dynamic tab row set occupies the old drag band and publishes separate draggable gap rectangles. |
| Tab keyboard access | `13_caption_tabs.asm:658`, `EnterTabKeyboard`; `13_caption_tabs.asm:694`, `MoveTabKeyboardFocus`; `13_caption_tabs.asm:838`, `InvokeTabKeyboardFocus` | F6 enters visible tab targets, Left/Right walks tab bodies and close glyphs, Ctrl+Tab moves between tab bodies, and Alt+Space stays on the real system menu. |
| Tab UI Automation provider | `13_caption_tabs.asm:1551`, `UiaSimple_GetPropertyValue`; `13_caption_tabs.asm:1757`, `UiaFragment_Navigate`; `13_caption_tabs.asm:1994`, `UiaInvoke_Invoke` | Visible tab bodies expose TabItem, selected state, bounds, Invoke, and SelectionItem; visible close glyphs expose separate Button invoke targets. |
| Responsive caption | `14_responsive_caption.asm:174`, `caption_responsive`; `14_responsive_caption.asm:789`, `ComputeResponsivePolicy`; `14_responsive_caption.asm:2545`, `HandleGetMinMaxInfo` | A sidecar visibility policy hides optional rows, populates app overflow, and writes a DPI-aware minimum track size. |
| RTL mirrored caption | `15_rtl_caption.asm:245`, `layout_flags`; `15_rtl_caption.asm:576`, `LayoutTabs`; `15_rtl_caption.asm:982`, `LayoutCaptionControls`; `15_rtl_caption.asm:1679`, `ShowSystemMenu` | The same physical geometry table mirrors semantic leading/trailing placement, tab flow, and popup layout while resize edges stay physical. |
| Per-HWND context | `16_multi_window.asm:58`, `CSD_WINDOW`; `16_multi_window.asm:328`, `CsdWindowAlloc`; `16_multi_window.asm:1348`, `WindowProc` context load; `16_multi_window.asm:1606`, `wnd_ncdestroy` | Window-owned CSD state moves out of process globals and is attached to each HWND through `GWLP_USERDATA`. |
| Custom layered shadow | `17_custom_shadow.asm:66`, `BLENDFUNCTION`; `17_custom_shadow.asm:342`, `CreateShadowSurface`; `17_custom_shadow.asm:423`, `RenderShadowPixels`; `17_custom_shadow.asm:583`, `UpdateShadowWindow` | The main CSD HWND stays normal while an owned layered popup carries premultiplied BGRA shadow pixels around it. |
| DPI manifest matrix | `18_dpi_manifest_matrix/generated/18_pmv2.asm:2`, variant label; `18_dpi_manifest_matrix/matrix_app.inc:721`, DPI-change logging; `18_dpi_manifest_matrix/write_report.ps1:1`, matrix report | One CSD source is assembled under four manifests so binary metadata and observed DPI messages can be compared directly. |

The Chrome/Views discussion in `Client-Side Decoration.md` is background, not a
hidden framework in this directory. These examples deliberately write the
message handlers directly so the mechanics stay visible.

## Silent Decisions

| Anchor | Decision | Why it is correct | What breaks if changed |
| --- | --- | --- | --- |
| `01_nccalc_frame.asm`, `wnd_nccalcsize` | Return `0` for the simple form; return `0300h` (`WVR_REDRAW`) when `wParam != 0`. | `0` accepts USER's proposed rect as the client rect; `WVR_REDRAW` prevents preserved stale pixels after size/move recalculation. | Forwarding to `DefWindowProcW` carves the standard caption and border back out; omitting redraw can leave old client pixels smeared. |
| `01_nccalc_frame.asm`, `wnd_erasebkgnd` | Return `1`. | The app says the erase is handled because `WM_PAINT` fills every visible pixel. | Default erase can flash the background before the full paint pass. |
| `02_nchittest_frame.asm`, `HitTestCsdFrame` | Sign-extend LOWORD/HIWORD from `lParam`. | Hit-test coordinates are signed screen coordinates; monitors can live at negative X/Y. | Zero-extension misclassifies windows on monitors left or above the primary display. |
| `03_dwm_frame.asm`, `dwm_margins` | Use one-pixel margins. | The tiny margin re-enters the DWM frame path for shadow and animations without restoring a visible system frame. | Zero margins lose the compositor frame behavior; large margins create unwanted glass/inset assumptions. |
| `include/addon/csd/dpi.inc`, `CsdCreateTextFontForDpi` | Negate the font height before `CreateFontW`. | A negative height requests character height; a positive height requests cell height. | Text renders smaller than expected and drifts from normal system-menu text. |
| `include/addon/csd/theme.inc`, `CsdThemeRefresh` | `bswap eax; shr eax,8`. | DWM reports ARGB as `0xAARRGGBB`; GDI `COLORREF` stores `0x00BBGGRR`. | Accent-derived GDI brushes swap red/blue or carry the wrong byte order. |
| `include/addon/csd/snap.inc`, `CSD_SNAP_WINDOWS11_BUILD = 22000` | Gate snap-layout hit-testing on Windows 11 RTM or newer. | The snap-layout flyout is a Windows 11 shell affordance and expects `HTMAXBUTTON`. | Down-level systems get a hit-test code they do not need, or Windows 11 misses the flyout. |
| `08_snap_layouts.asm`, `HTMAXBUTTON` path | Return `HTMAXBUTTON` only for the maximize row when snap is enabled. | That one result moves hover/click traffic for the glyph into `WM_NC*` messages. | Owner-drawn state stops updating unless the NC messages are bridged back. |
| `07_caption_state.asm`, `HandleCaptionLButtonUp` | Fire only when pressed index equals release index. | This is the standard "drag off to cancel" button affordance. | A press on one glyph followed by release on another can dispatch the wrong command. |
| `09_embed_edit.asm`, `WS_CLIPCHILDREN` plus `CSD_CAPTION_CHILD` | Clip child windows and skip parent drawing for the child row. | The child `EDIT` owns text, caret, selection, IME, and its pixels. | Parent repaint can draw over the child slot or flicker underneath it. |
| `10_a11y_keyboard.asm`, explicit keyboard order | Use `0,2,5,4,3` for caption focus instead of descriptor order. | Descriptor order groups trailing rows as close/max/min for layout, but visual keyboard order is system/settings/min/max/close. | Arrow keys would appear to move backward across the window buttons. |
| `10_a11y_keyboard.asm`, high-contrast override | Refresh DWM/theme state first, then override paint colors with system colors when high contrast is active. | High contrast is an accessibility mode, not a dark-theme variant; system colors are the contract users choose. | Accent/red close-button colors can become illegible or communicate only by hue. |
| `11_caption_fade.asm`, state IDs instead of cached colors | Store `fromValue`/`value` plus a phase, then resolve colors during paint. | Theme and accent changes immediately affect both ends of the blend. | Active fades can interpolate through stale colors after a live theme change. |
| `12_backdrop.asm`, unconditional body/title fills | Fill the body and caption even when a system backdrop is active. | Normal GDI windows preserve old redirection-surface pixels if broad regions are left uncovered. | Transparent text and resize/move preservation can smear old pixels across the client. |
| `19_backdrop_viewer.asm`, alpha repair pass | Render normal GDI into a 32bpp DIB, draw a color-grid probe blended with the requested mode tint, then set alpha opaque outside the viewer rectangle and write clear/mid/strong alpha bands inside it. | `DWMWA_REDIRECTIONBITMAP_ALPHA` is window-wide; GDI writes RGB but does not reliably define alpha, and transparent pixels need known RGB content to distinguish software blend from DWM material behavior. | Unrelated caption/body pixels can become see-through, the material viewport can smear old redirected pixels, or `None` and material modes can look indistinguishable during testing. |
| `13_caption_tabs.asm`, tab gaps not one drag rectangle | Store tab rectangles and gap rectangles separately. | Tabs stay app-owned `HTCLIENT` content while empty caption space still moves the window. | Treating the whole band as draggable makes tab clicks move the window; treating it all as client loses native move behavior. |
| `13_caption_tabs.asm`, keyboard sidecar | Keep tab keyboard focus as `active/index/part` beside the tab strip rows. | Keyboard focus can point at a visible tab body or close glyph without changing the item, geometry, or paint-state rows. | Folding keyboard focus into tab state would mix input modality with hover/pressed/active rendering and leave stale focus after responsive relayout. |
| `14_responsive_caption.asm`, visibility sidecar | Keep collapse/overflow state beside descriptors instead of folding it into descriptor flags. | Descriptor rows remain stable render and command identities while layout can hide rows by current width. | Hidden rows can leave stale hit-test rectangles or make descriptor flags mean too many unrelated things. |
| `15_rtl_caption.asm`, manual mirror instead of `WS_EX_LAYOUTRTL` reliance | Store already-mirrored physical rectangles in the normal geometry rows. | Painting and hit-testing continue to compare the same `RECT` values, and physical resize edges remain `HTLEFT`/`HTRIGHT`. | A mixed logical/physical model can draw in one place and hit-test in another. |
| `16_multi_window.asm`, 09-level surface for ownership | Use the simpler Search/edit caption while demonstrating heap contexts and an owned tool frame. | Per-window state changes are visible without combining the tab, overflow, and RTL algorithms into the same refactor. | The ownership lesson gets buried under unrelated policy code, and regressions become hard to attribute. |
| `17_custom_shadow.asm`, owned layered popup | Keep the main HWND normal and put only the shadow pixels in the layered companion window. | USER still owns the real top-level move/resize/minimize/system-menu contract while the demo isolates `UpdateLayeredWindow` alpha behavior. | Making the main window layered changes the behavioral contract and hides the cost this rung is meant to teach. |
| `18_dpi_manifest_matrix`, same source under four manifests | Change only the embedded manifest and variant labels; keep the caption source body identical. | Differences in `GetDpiForWindow`, `WM_DPICHANGED`, and suggested-rect behavior can be attributed to process DPI awareness instead of source drift. | Four hand-edited apps would make the matrix a code comparison instead of a manifest/loader observation. |

## Reading `04` Without The Cliff

`04_caption_controls.asm` is the steepest rung because it introduces four ideas
at once. Read it in four passes:

1. Table shape: `caption_descriptors`, `caption_geometry`, `caption_state`, and
   `CsdCaptionLayout`.
2. Rendering: `DrawCaptionControls`, `FontIcon_DrawGlyph`, and the brush setup.
3. Hit-testing: `CaptionControlFromClientPoint`, `HitTestCsdFrame`, and the
   reduced `HTCAPTION` strip.
4. Commands: `ShowSystemMenu`, `HandleCaptionClientClick`, and
   `CsdUpdateSystemMenuState`.

The point of the rung is not "draw some icons." It is "the caption is now a
data-driven control strip, while the real system menu and `WM_SYSCOMMAND`
contract remain in use."

## Constants Notes

These values are still intentionally local to the teaching rungs rather than
centralized into a shared metrics include. `METRICS.md` is the fuller companion;
this table is the quick index.

| Value | Where | Meaning |
| --- | --- | --- |
| `CSD_TITLE_HEIGHT = 52` | `01`-`03` | Fixed 96-DPI title band for the pre-DPI rungs. |
| `CSD_TITLE_HEIGHT_DIP = 56` | `04`-`08`, `18` | DIP-based caption height used once controls enter the caption; the DPI matrix returns to this 05-style surface. |
| `CSD_TITLE_HEIGHT_DIP = 48` | `09`-`17`, `19` | Shorter row to make the embedded edit feel like a title-bar search field. |
| `CSD_CONTENT_PAD` / `CSD_CONTENT_PAD_DIP = 26` | `01`-`19` | Aesthetic text/content inset; later rungs also derive drag padding from it. |
| `CSD_EDGE_PX` / `CSD_EDGE_DIP = 4` | all rungs | App-drawn visual edge, not the full resize hit-test strip. |
| `CSD_RESIZE_PX = 8` and fallback `8` DIPs | `02`-`04`, `include/addon/csd/dpi.inc` | Fixed/pre-DPI resize strip and the fallback if `GetSystemMetricsForDpi` fails. |
| `CSD_BUTTON_WIDTH_DIP = 44` | `04`-`19` | Aesthetic owner-drawn caption-button width. |
| `CSD_DRAG_PAD_DIP = CSD_CONTENT_PAD[_DIP] / 2` | `04`-`19` | Gap left between caption controls and draggable or tabbed caption space. |
| `CSD_INITIAL_CLIENT_W_DIP = 920`, `CSD_INITIAL_CLIENT_H_DIP = 500` | `05`-`19` | Desired client size before `AdjustWindowRectExForDpi` expands to a top-level window. |
| `CSD_SEARCH_WIDTH_DIP = 240` | `09`-`17`, `19` | Width reserved for the live Search edit child. |
| `CSD_CHILD_PAD_X_DIP = 8`, `CSD_CHILD_PAD_Y_DIP = 12` | `09`-`17`, `19` | Insets that keep the edit control inside the Search caption slot. |
| `CSD_SHADOW_MARGIN_*_DIP`, `CSD_SHADOW_RADIUS_DIP`, `CSD_SHADOW_MAX_ALPHA` | `17` | Local shadow bitmap geometry and opacity policy for the app-owned layered popup. |
| `CSD_VIEWER_HEIGHT_DIP = 210`, `CSD_VIEWER_GAP_DIP = 18` | `19` | Local material-viewport size and spacing for the backdrop viewer. |
| `CSD_TAB_MIN_WIDTH_DIP = 96`, `CSD_TAB_PREF_WIDTH_DIP = 150` | `13`-`15` | Minimal tab fit policy that 14 combines with descriptor overflow and 15 mirrors. |
| `CSD_TAB_DRAG_RECT_MAX = 8` | `13`-`15` | Small fixed drag-gap array for the tabbed caption teaching example. |
| `CSD_MIN_BODY_W_DIP = 520`, `CSD_MIN_BODY_H_DIP = 320` | `14`-`15` | Client minimums combined with irreducible caption width for `WM_GETMINMAXINFO`. |
| `CSD_LAYOUT_RTL = 0001h` | `15` | Local layout-policy flag that mirrors semantic leading/trailing caption geometry. |
| `CSD_TEXT_FONT_PX96 = 13` | `include/addon/csd/dpi.inc` | Base Segoe UI text size before DPI scaling. |
| `CSD_SNAP_WINDOWS11_BUILD = 22000` | `include/addon/csd/snap.inc` | Windows 11 RTM build, the first snap-layout target for this policy. |
| `CSD_BACKDROP_WINDOWS11_BUILD = 22621` | `include/addon/csd/backdrop.inc` | Windows 11 build gate for `DWMWA_SYSTEMBACKDROP_TYPE`. |
| `CSD_BACKDROP_ALPHA_BUILD = 26100` | `include/addon/csd/backdrop.inc` | Windows 11 build gate used before applying `DWMWA_REDIRECTIONBITMAP_ALPHA`. |

If the metrics are promoted later, the useful split is "mechanical values with a
Win32 reason" versus "aesthetic values chosen for these demos."

## Observability

The repository already has `examples/msgflood`, a message-discovery harness. It
does not spy on another process; it logs messages for its own HWND. It is still
the right companion when learning CSD message traffic:

1. Build and run `examples/msgflood`.
2. Use `Filters` to show non-client traffic, or press `V` to turn the noise
   filter off temporarily.
3. Move, resize, maximize, and right-click its window and watch
   `WM_NCHITTEST`, `WM_NCCALCSIZE`, `WM_NCMOUSEMOVE`, `WM_NCLBUTTONDOWN`, and
   `WM_NCLBUTTONUP` arrive.
4. Run the matching CSD rung and compare the behavior to the message contract in
   `README.md`.

For direct CSD introspection, a future debug-overlay tool should paint
the current hit-test result and descriptor/geometry/state row under the cursor.
Until that exists, `msgflood` provides the live message vocabulary and ordering.

## Current Omissions

The examples are concise teaching programs, not production frame templates.

- Object-creation and DWM/theme calls intentionally keep failure handling light
  so the message and state contracts stay readable. Production code must check
  GDI object creation, font creation, DWM calls, class registration, and child
  window creation.
- `10_a11y_keyboard.asm` makes the 09-level owner-drawn buttons keyboard
  reachable, high-contrast legible, and visible to UIA clients.
  `13_caption_tabs.asm` adds keyboard and UIA reachability for visible tabs and
  close glyphs; later overflow reporting still needs its own work.
- The assumed reader already knows the Win64 calling convention, fasmg
  `struct`/`iterate`/`proc` style, GDI object lifetime, and the basic Win32
  message loop.
