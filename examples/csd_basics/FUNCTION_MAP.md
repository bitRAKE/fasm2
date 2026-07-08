# CSD Function Map

This map describes the current `13`-`15` tabbed-caption family after the shared
UIA and local support extractions. It is meant to guide future factoring: move
mechanics only when the call relationships prove they are common, and keep
caption policy in the example that teaches it.

## Layers

| Layer | Files | Responsibility |
| --- | --- | --- |
| Reusable CSD mechanics | `include/addon/csd/*.inc` | Public-ish helpers for caption rows, DPI, theme, state, snap, backdrop, child placement, tab-strip rows, and policy-free UIA mechanics. |
| Example-family support | `examples/csd_basics/csd_example_support.inc` | Shared support routines that bind to the tabbed-caption demo globals used by `13`, `14`, and `15`. |
| Rung policy | `13_caption_tabs.asm`, `14_responsive_caption.asm`, `15_rtl_caption.asm` | Descriptor tables, tab layout policy, overflow policy, RTL policy, UIA element tree, rendering shape, command routing, and message dispatch. |

## Main Flow

```text
start
  -> RegisterClassExW/CreateWindowExW
  -> WindowProc
       -> WM_CREATE: DPI/theme/init tabs/UIA/children
       -> WM_SIZE/WM_DPICHANGED: LayoutCaptionControls -> LayoutTabs -> LayoutCaptionChildren
       -> WM_NCHITTEST: HitTestCsdFrame
       -> mouse messages: HandleCaptionMouse* / HandleCaptionLButton*
       -> keyboard messages: HandleTabKeyDown / HandleTabSysKeyDown
       -> WM_GETOBJECT: HandleGetObject -> CsdUiaHandleGetObject
       -> WM_PAINT: PaintFrame -> DrawCaptionControls -> DrawTabs
       -> WM_SYSCOMMAND/WM_COMMAND: system commands, app commands, overflow/RTL policy
```

## Shared Example Support

`csd_example_support.inc` owns code that is duplicated by shape across `13`,
`14`, and `15`, but still names demo globals such as `hMain`, `current_theme`,
`caption_input`, `tabstrip`, `caption_drag_rects`, and `hSearchEdit`.

| Group | Procedures |
| --- | --- |
| DPI/font setup | `SetDpiState`, `RefreshDpiState`, `RebuildCaptionFont`, `HandleDpiChanged`, `PrepareInitialWindowRect` |
| Theme/backdrop/brushes | `DestroyThemeBrushes`, `RebuildThemeBrushes`, `BackdropNameForType`, `BackdropCapabilityLabel`, `RefreshThemeAndFrame`, `CycleBackdropMode` |
| Paint object lifetime | `CreatePaintObjects`, `DestroyPaintObjects`, `DestroyCaptionChildren`, `HandleCtlColorEdit` |
| Geometry lookup | `AddCaptionDragRect`, `CaptionIndexFromClientPoint`, `CaptionControlFromClientPoint`, `ClientLParamFromScreenLParam` |
| Color policy | `CsdBlendColorref`, `CaptionFillColorForState`, `CaptionGlyphColorForState`, `TabFillColorForState`, `TabTextColorForState` |
| Tab state helpers | `SetTabHot`, `SetTabPressed`, `ClearTabInput`, `SetActiveTab` |
| Fade timer | `StartCaptionFadeTimer`, `StopCaptionFadeTimer`, `HandleCaptionFadeTimer` |
| Activation/snap bridge | `HandleCaptionActivate`, `HandleSnapNcMouseMove`, `HandleSnapNcLButtonDown`, `HandleSnapNcLButtonUp` |

These stayed local to the example directory because they are not clean
library calls. They assume the tabbed-caption demo's global names and call back
into rung-owned routines such as `ApplyTabStates`, `UpdateThemeStatus`,
`LayoutCaptionControls`, `LayoutCaptionChildren`, `HandleCaptionLButtonDown`,
and `HandleCaptionLButtonUp`.

## Rung-Owned Policy

These functions remain in the individual examples because their behavior is the
lesson of each rung:

| Area | `13_caption_tabs` | `14_responsive_caption` | `15_rtl_caption` |
| --- | --- | --- | --- |
| Text font child update | `RebuildTextFont` omits live child font reset. | `RebuildTextFont` updates the Search child font. | Same as `14`. |
| Child creation/layout | Simple visible Search child. | Hide/show Search based on collapse policy. | Same collapse behavior plus no-inherit-layout creation style. |
| Caption layout | Fixed controls plus tab band. | Responsive visibility and overflow. | Mirrored leading/trailing plus responsive overflow. |
| Menus | Real system menu only. | Real system menu plus app overflow menu. | System and overflow menus add RTL popup flags. |
| UIA tree | Fixed buttons, tab bodies, close glyphs. | Tab bodies, close glyphs, overflow. | Same as `14`, with mirrored bounds. |
| Commands | New tab, settings/backdrop, tab select/close. | Adds overflow command routing. | Adds F2 layout toggle and RTL status. |

## UIA Boundary

`include/addon/csd/uia.inc` owns COM mechanics:

- `CsdUiaLoadApis`
- `CsdUiaInitProvider`
- `CsdUiaGuidEquals`
- `CsdUiaReturnFragmentInterface`
- `CsdUiaReturnSimpleInterface`
- `CsdUiaVariant*`
- `CsdUiaProvider_AddRef` / `CsdUiaProvider_Release`
- `CsdUiaRuntimeIdForSlot`
- `CsdUiaFragment_GetRuntimeId`
- `CsdUiaSelection_SelectionContainer`
- `CsdUiaHandleGetObject`

The examples still own the UIA provider tree:

- slot/index/part mapping;
- visible-child traversal;
- property values, names, and automation IDs;
- focus behavior;
- Invoke and SelectionItem routing.

## Extraction Rules

Move a function into `include/addon/csd/` only when it can accept explicit
parameters and stop naming example globals. Move a function into
`csd_example_support.inc` only when the three tabbed-caption examples share the
same behavior and the dependency on demo globals is intentional. Leave a
function in the example when it teaches the rung's policy or when `14`/`15`
change its behavior for overflow or RTL.
