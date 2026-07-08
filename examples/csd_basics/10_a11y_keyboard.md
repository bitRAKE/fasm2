# `10_a11y_keyboard` Implementation Brief

This is the implementation brief for the first full accessibility rung. Its job
is to keep the keyboard, high-contrast, and UI Automation work scoped, testable,
and honest beside `10_a11y_keyboard.asm`.

## Implementation Status

Implemented source: `10_a11y_keyboard.asm`.

The current implementation starts from `09_embed_edit.asm` and delivers the
09-level accessibility rung:

- F10 or Alt enters caption keyboard mode.
- Left/Right moves through System menu, Settings, Minimize, Maximize/Restore,
  and Close, skipping the native Search edit child.
- Enter/Space invokes the focused caption row through the same system-command
  and app-command paths used by mouse activation.
- Alt+Space opens the real system menu.
- Escape exits caption keyboard mode before falling back to the demo's
  Escape-to-close behavior.
- `WM_ACTIVATE`/`WM_KILLFOCUS` clear caption keyboard mode.
- High contrast is detected with `SystemParametersInfo(SPI_GETHIGHCONTRAST)`;
  caption colors switch to system colors and keyboard focus is drawn with
  `DrawFocusRect`.
- `WM_GETOBJECT` returns a local UI Automation provider root for
  `UiaRootObjectId`.
- Static COM provider objects expose one root and five owner-drawn caption
  button children: System menu, Settings, Minimize, Maximize/Restore, and Close.
- Each owner-drawn button reports name, automation ID, Button control type,
  enabled state, keyboard-focus state, screen-coordinate bounds, runtime ID, and
  the Invoke pattern.
- UIA Invoke routes through the same descriptor-index command helper used by
  keyboard invocation.
- The Search edit remains a native child-HWND provider and is not duplicated by
  the owner-drawn caption provider.

The provider support is intentionally local to this example. It dynamically
loads `UIAutomationCore.dll` and `oleaut32.dll` through existing `kernel32`
imports instead of widening the shared `win64wx.inc` import list. Promotion into
reusable `include/addon/csd/` support remains a later packaging step.

## Goal

Make the owner-drawn caption controls reachable through keyboard and UI
Automation while preserving the CSD mechanics already taught by `09`:

- Alt+Space opens the real system menu.
- F10 or Alt enters caption keyboard navigation.
- Left/Right moves across caption controls.
- Enter/Space invokes the focused caption control.
- Narrator and UIA inspection tools can discover names, bounds, enabled state,
  focus state, control type, and the Invoke pattern for each owner-drawn button.
- High-contrast mode uses system colors and a visible focus indicator.

The native Search `EDIT` child should keep its HWND/provider behavior. The new
provider work is for the owner-drawn caption rows that are currently only
`HTCLIENT` pixels.

## Non-Goals

- Do not turn the examples into a general UI framework.
- Do not replace the real system menu with a custom menu.
- Do not hand-roll accessibility for the child `EDIT`; let the child HWND expose
  its native provider.
- Do not require the earlier rungs to grow UIA code. `10` can include additional
  support files or private metadata while the previous rungs remain readable.

## First Gate: PSDK And Local UIA Support

The current repository has the normal keyboard and high-contrast equates
(`WM_KEYDOWN`, `WM_SYSKEYDOWN`, `VK_F10`, `VK_LEFT`, `VK_RIGHT`,
`VK_RETURN`, `VK_SPACE`, `VK_ESCAPE`, `SPI_GETHIGHCONTRAST`). This example now
locally defines the minimal UIA provider surface rather than adding global API
include files.

Implemented local support:

| Area | Needed surface |
| --- | --- |
| USER/object routing | `WM_GETOBJECT`, `OBJID_CLIENT`, `UiaRootObjectId` |
| UIA core API | `UiaReturnRawElementProvider`, `UiaHostProviderFromHwnd`, `UiaDisconnectProvider` |
| Provider COM interfaces | `IRawElementProviderSimple`, `IRawElementProviderFragmentRoot`, `IRawElementProviderFragment`, `IInvokeProvider` |
| UIA ids | `UIA_InvokePatternId`, `UIA_NamePropertyId`, `UIA_ControlTypePropertyId`, `UIA_AutomationIdPropertyId`, `UIA_BoundingRectanglePropertyId`, `UIA_IsEnabledPropertyId`, `UIA_IsKeyboardFocusablePropertyId`, `UIA_HasKeyboardFocusPropertyId`, `UIA_ButtonControlTypeId` |
| OLE Automation values | `VARIANT`, `VT_EMPTY`, `VT_I4`, `VT_BSTR`, `VT_BOOL`, `VARIANT_TRUE`, `SysAllocString`, `SafeArrayCreateVector`, `SafeArrayPutElement`, `SafeArrayDestroy` |
| High contrast | `HIGHCONTRAST`, `HCF_HIGHCONTRASTON` |

Local SDK evidence checked against Windows SDK `10.0.26100.0`:

- `UIAutomationCoreApi.h` defines `UiaRootObjectId` and declares
  `UiaReturnRawElementProvider`, `UiaHostProviderFromHwnd`, and
  `UiaDisconnectProvider`.
- `UIAutomationCore.h` declares the provider COM interfaces and navigation
  methods.
- `UIAutomationClient.idl` defines the property, pattern, and control-type IDs.
- `WinUser.h` defines `WM_GETOBJECT`, `OBJID_CLIENT`, and high-contrast data.
- `oaidl.h`, `wtypes.h`, and `oleauto.h` define `VARIANT`, `VARTYPE`, `BSTR`,
  `SAFEARRAY`, `SysAllocString`, and the `SafeArray*` helpers.

The reusable include work should still be verified with a tiny assemble-time
interface/offset probe when this code is promoted out of the example.

## Metadata Shape

The descriptor table needs accessibility identity in addition to paint and
command identity. The long-term reusable shape is to extend
`CSD_CAPTION_DESCRIPTOR` with stable metadata:

```asm
struct CSD_CAPTION_DESCRIPTOR
  id              dd ?
  glyph           dd ?
  command         dd ?
  hitCode         dd ?
  flags           dd ?
  widthDip        dd ?
  namep           dq ?       ; UIA Name, e.g. "Close"
  automationIdp   dq ?       ; stable per-app ID string
  defaultActionp  dq ?       ; usually "Invoke"
ends
```

To keep earlier rungs untouched, `10` keeps metadata as local string rows keyed
by descriptor index. Promotion into `CSD_CAPTION_DESCRIPTOR` can happen when the
includes move toward `include/addon/csd/`.

Suggested names:

| Row | Name | Automation ID |
| --- | --- | --- |
| System menu | `System menu` | `caption.system` |
| Search edit | native child provider | native child provider |
| Settings | `Settings` | `caption.settings` |
| Minimize | `Minimize` | `caption.minimize` |
| Maximize/Restore | `Maximize` or `Restore` from `IsZoomed` | `caption.maximize` |
| Close | `Close` | `caption.close` |

## Provider Model

The implementation uses one root provider for the top-level window and one
static child provider per owner-drawn caption control. Each provider object
carries:

- vtable pointer(s);
- reference count;
- descriptor index, or `CSD_CAPTION_NO_INDEX` for the root;
- keyboard-order index for sibling navigation.

`WM_GETOBJECT` handling:

1. If `lParam == UiaRootObjectId`, return the root provider through
   `UiaReturnRawElementProvider(hwnd, wParam, lParam, provider)`.
2. For other object IDs, fall through to `DefWindowProc` so standard providers
   can answer requests the custom caption does not own.
3. Do not answer `WM_GETOBJECT` before the provider state is initialized or
   after teardown begins.

Provider behavior:

| Interface | Responsibility |
| --- | --- |
| `IRawElementProviderSimple` | Provider options, pattern lookup, property values, host provider for the root. |
| `IRawElementProviderFragmentRoot` | `ElementProviderFromPoint` maps screen point to caption geometry; `GetFocus` returns the keyboard-focused caption row. |
| `IRawElementProviderFragment` | Parent/next/previous/first/last navigation, runtime ID, bounding rectangle, focus setting. |
| `IInvokeProvider` | Calls the same command path as mouse activation. |

Property and fragment handling returns:

- `UIA_NamePropertyId`: descriptor name.
- `UIA_AutomationIdPropertyId`: stable automation ID.
- `UIA_ControlTypePropertyId`: `UIA_ButtonControlTypeId` for owner-drawn caption
  buttons.
- `IRawElementProviderFragment::get_BoundingRectangle`: current geometry
  converted to screen coordinates.
- `IRawElementProviderFragment::GetRuntimeId`: a small `SAFEARRAY(VT_I4)` using
  `UiaAppendRuntimeId`.
- `UIA_IsEnabledPropertyId`: true for exposed caption buttons in this rung.
- `UIA_IsKeyboardFocusablePropertyId`: true for owner-drawn caption buttons.
- `UIA_HasKeyboardFocusPropertyId`: true for the current caption keyboard focus.

Pattern handling:

- Return `IInvokeProvider` only for actionable rows.
- Return no pattern for pure containers or hidden rows.
- The Search child row should not duplicate the native edit's provider.

## Keyboard State

Add a small keyboard state row separate from mouse input:

```asm
struct CSD_CAPTION_KEYBOARD
  active        dd ?
  focusIndex    dd ?
  orderIndex    dd ?
ends
```

Keyboard routing:

| Message/input | Behavior |
| --- | --- |
| `WM_SYSKEYDOWN/VK_SPACE` with Alt | Open the real system menu. |
| `WM_KEYDOWN/VK_F10` or plain Alt release/entry policy | Enter caption navigation and focus the first actionable owner-drawn row. |
| Left/Right | Move to previous/next actionable owner-drawn row, wrapping inside the caption controls. |
| Enter/Space | Invoke the focused row through the same command path used by mouse clicks. |
| Escape | Leave caption keyboard mode and clear focus state. |
| Focus loss/deactivate | Clear caption keyboard mode and repaint. |

Visual rules:

- Keyboard focus is distinct from hover and pressed state.
- Focus should draw a visible focus rectangle or high-contrast border without
  relying on hover color.
- `WM_ACTIVATE/WA_INACTIVE` must clear keyboard focus just as it clears hot and
  pressed state.

## High Contrast

High contrast is not just a dark-mode variant. When
`SystemParametersInfo(SPI_GETHIGHCONTRAST)` reports `HCF_HIGHCONTRASTON`:

- prefer system colors such as `COLOR_WINDOW`, `COLOR_WINDOWTEXT`,
  `COLOR_HIGHLIGHT`, and `COLOR_HIGHLIGHTTEXT`;
- keep the close button understandable without depending only on red fill;
- draw focus with a high-contrast visible primitive;
- refresh on `WM_SETTINGCHANGE` and repaint all caption rows.

## Message Integration

`10` should start from `09` so it includes the hardest case: owner-drawn caption
buttons plus a real child `EDIT`.

Expected new handlers:

- `WM_GETOBJECT` for UIA provider return;
- `WM_SETFOCUS` / `WM_KILLFOCUS` for keyboard focus state;
- expanded `WM_KEYDOWN` and `WM_SYSKEYDOWN` routing;
- `WM_SETTINGCHANGE` high-contrast refresh layered on the existing theme
  refresh.

Expected support modules:

- local provider constants/structs in `10_a11y_keyboard.asm`;
- static provider objects and COM vtables;
- dynamic UIA/OLE API loading through `LoadLibraryW`/`GetProcAddress`;
- reusable candidates for later promotion:
  `csd_accessibility.inc`, `csd_keyboard.inc`, and `uia_min.inc`.

## Verification

Build gate:

```cmd
cd C:\git\fasm2\examples\csd_basics
_build.cmd
```

Manual keyboard gate:

- F10 enters the caption.
- Left/Right moves focus across System menu, Settings, Minimize,
  Maximize/Restore, and Close, skipping the child Search edit.
- Enter/Space invokes the focused row.
- Alt+Space opens the real system menu.
- Escape exits caption keyboard mode.
- Deactivation clears focused/pressed visuals.

UIA gate:

- Inspect.exe, Narrator, or UIAutomationClient sees one top-level provider and
  child button elements for the owner-drawn caption rows.
- Each owner-drawn row reports name, automation ID, button control type, bounds,
  enabled state, keyboard-focus state, and Invoke pattern where appropriate.
- Invoking a provider routes through the same command path as mouse activation.
- The Search edit appears through its native child HWND provider, not as a
  duplicate owner-drawn caption button.

Automated smoke evidence from PowerShell UIAutomationClient:

- top-level window found by process ID with the app title as its UIA name;
- five `caption.*` button rows found: System menu, Settings, Minimize, Maximize,
  and Close;
- all five report positive bounding rectangles and the Invoke pattern;
- one native `Edit` provider remains for Search with automation ID `4202`.

High-contrast gate:

- Toggle high contrast and verify caption text, focus, and command buttons
  remain legible.
- The focused row is visible without relying on accent or close-button red.

## Source Anchors

| Source | Role |
| --- | --- |
| `10_a11y_keyboard.asm:93` | `CSD_CAPTION_KEYBOARD`, the local keyboard-focus state row. |
| `10_a11y_keyboard.asm:170` | `keyboard_order`, the explicit visual focus order. |
| `10_a11y_keyboard.asm:380` | `RefreshHighContrast`, the `SPI_GETHIGHCONTRAST` and system-color override path. |
| `10_a11y_keyboard.asm:549` | `LoadUiaApis`, dynamic `UIAutomationCore.dll`/`oleaut32.dll` binding. |
| `10_a11y_keyboard.asm:611` | `InitUiaProviders`, static root/child provider setup. |
| `10_a11y_keyboard.asm:835` | `UiaProvider_QueryInterface`, provider COM identity and interface routing. |
| `10_a11y_keyboard.asm:947` | `UiaSimple_GetPropertyValue`, name/automation/control/focus property handling. |
| `10_a11y_keyboard.asm:1216` | `UiaFragment_BoundingRectangle`, caption geometry to screen-coordinate UIA bounds. |
| `10_a11y_keyboard.asm:1330` | `HandleGetObject`, `WM_GETOBJECT` return through `UiaReturnRawElementProvider`. |
| `10_a11y_keyboard.asm:1358` | `EnterCaptionKeyboard`, F10/Alt entry into caption focus. |
| `10_a11y_keyboard.asm:1370` | `ClearCaptionKeyboard`, focus cleanup for Escape/deactivation/focus loss. |
| `10_a11y_keyboard.asm:1387` | `MoveCaptionKeyboardFocus`, Left/Right wrapping across owner-drawn rows. |
| `10_a11y_keyboard.asm:1542` | Focus rectangle rendering in `DrawCaptionControls`. |
| `10_a11y_keyboard.asm:1930` | `InvokeCaptionIndex`, shared descriptor-index invocation for keyboard/UIA. |
| `10_a11y_keyboard.asm:1996` | `WM_KEYDOWN` routing for F10, arrows, Enter/Space, and Escape. |
| `10_a11y_keyboard.asm:2042` | `WM_SYSKEYDOWN` routing for Alt, F10, and Alt+Space. |
| `10_a11y_keyboard.asm:2374` | Focus, UIA, and keyboard message integration in `WindowProc`. |

## References

- Microsoft Learn: [`WM_GETOBJECT` message](https://learn.microsoft.com/en-us/windows/win32/winauto/wm-getobject)
- Microsoft Learn: [handling `WM_GETOBJECT`](https://learn.microsoft.com/en-us/windows/win32/winauto/handling-the-wm-getobject-message)
- Microsoft Learn: [`UiaReturnRawElementProvider`](https://learn.microsoft.com/en-us/windows/win32/api/uiautomationcoreapi/nf-uiautomationcoreapi-uiareturnrawelementprovider)
- Microsoft Learn: [`IRawElementProviderSimple::GetPropertyValue`](https://learn.microsoft.com/en-us/windows/win32/api/uiautomationcore/nf-uiautomationcore-irawelementprovidersimple-getpropertyvalue)
- Microsoft Learn: [UI Automation support for the Button control type](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-supportbuttoncontroltype)
- Microsoft Learn: [UI Automation control patterns overview](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-controlpatternsoverview)
