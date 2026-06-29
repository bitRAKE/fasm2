# `DefWindowProcW` Default Handling — A Win32 Reference

A window procedure is a dispatcher with a fallback. Every message a procedure does not claim should fall through to `DefWindowProcW`, which supplies the behavior that makes a window *be a window*: it moves, sizes, closes, paints its frame, routes hit-tests, runs the system menu, switches keyboard layouts, and stores its own caption. Knowing when to defer to the default means knowing three things per message:

1. **What** `DefWindowProcW` does for free.
2. **The return value** it produces, and what breaks if a different value is returned.
3. Whether the message is **load-bearing** — that is, whether intercepting it without forwarding silently destroys downstream behavior, because the default handler *generates other messages or state* as a side effect.

The third axis is the one that causes the most confusion. `WM_CREATE` returning zero is the visible tip; the submerged mass is messages such as `WM_WINDOWPOSCHANGED`, whose default handler is the *only* source of `WM_SIZE` and `WM_MOVE`.

---

## Forwarding and return-value convention

The window procedure has the signature `LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM)`. Under the Windows x64 ABI the four arguments arrive in `rcx`, `rdx`, `r8`, `r9`, and `LRESULT` is returned in `rax`. Two correctness facts matter for an assembly implementation:

- `msg` is a `UINT` — 32 bits. Compare with the 32-bit register (`edx`), not `rdx`; the high half is not guaranteed zero.
- `LRESULT` is 64-bit. `xor eax, eax` is the correct "return 0" (it clears all of `rax`); a handle or pointer return must set the full register.

The two terminal idioms:

```asm
        xor     eax, eax            ; handled  -> return 0
        ret
        ; ----
        jmp     DefWindowProcW      ; unhandled -> forward; its RET returns to USER32
```

A tail `jmp DefWindowProcW` is valid only when the four argument registers are intact and the stack has been restored to its entry state (return address on top, the caller's 32-byte shadow space above it). If a frame was established, it must be torn down before the jump; otherwise `call DefWindowProcW` / `ret` is always safe.

---

## Parameter quick lookup

This table is only for unpacking `WPARAM` and `LPARAM` at a window-procedure entry point. On x64, `wParam` is the third argument (`r8`) and `lParam` is the fourth (`r9`). The table deliberately stays terse; behavior, return values, and forwarding obligations are covered in the sections that follow.

| Message(s) | `wParam` / `r8` | `lParam` / `r9` | Detail |
|---|---|---|---|
| `WM_NCCREATE` | unused | `CREATESTRUCT*` | See creation notes. |
| `WM_CREATE` | unused | `CREATESTRUCT*` | See creation notes. |
| `WM_CLOSE` | `0` | `0` | |
| `WM_DESTROY` | `0` | `0` | |
| `WM_NCDESTROY` | `0` | `0` | |
| `WM_QUIT` (message loop only) | exit code in `MSG.wParam` | `0` | Not delivered to a window procedure; `GetMessage` returns `0`. |
| `WM_PAINT` | `0` | `0` | The paint DC comes from `BeginPaint`, not the message parameters. |
| `WM_ERASEBKGND` | `HDC` | unused | The DC is clipped for the erase pass. |
| `WM_NCPAINT` | `HRGN`, or `1` for the whole frame | unused | |
| `WM_NCCALCSIZE` | `FALSE` or `TRUE` | `RECT*` if `FALSE`; `NCCALCSIZE_PARAMS*` if `TRUE` | [^nccalc] |
| `WM_NCHITTEST` | unused | packed signed screen `x,y` | [^coords] |
| `WM_NCACTIVATE` | active flag (`TRUE`/`FALSE`) | previous/next active `HWND`, `NULL`, or `-1` to suppress repaint | [^ncactivate] |
| `WM_NCLBUTTONDOWN` and other `WM_NC*BUTTON*` | `HT*` hit-test code | packed signed screen `x,y` | [^coords] |
| `WM_NCMOUSEMOVE` | `HT*` hit-test code | packed signed screen `x,y` | [^coords] |
| `WM_NCMOUSELEAVE` | unused | unused | |
| `WM_SETCURSOR` | `HWND` containing the cursor | `LOWORD` = `HT*`; `HIWORD` = triggering mouse message | [^setcursor] |
| `WM_SYSCOMMAND` | `SC_*` command in `wParam & 0FFF0h` | command-specific; often packed screen `x,y`, or Alt mnemonic for `SC_KEYMENU` | [^syscommand] |
| `WM_KEYDOWN`, `WM_KEYUP`, `WM_SYSKEYDOWN`, `WM_SYSKEYUP` | virtual-key code | packed repeat/scancode/extended/context/previous/transition bits | [^keybits] |
| `WM_CHAR`, `WM_SYSCHAR` | character code | same packed key bits as translated keystrokes | [^keybits] |
| `WM_GETDLGCODE` | virtual-key code, or `0` | `MSG*`, or `NULL` | |
| `WM_MOUSEACTIVATE` | top-level parent `HWND` | `LOWORD` = `HT*`; `HIWORD` = mouse message | [^mouseactivate] |
| `WM_LBUTTONDOWN`, `WM_LBUTTONDBLCLK`, other client `WM_*BUTTON*` | key/button state flags (`MK_*`) | packed signed client `x,y` | [^mousebuttons] |
| `WM_CONTEXTMENU` | source `HWND` | packed signed screen `x,y`, or `-1` for keyboard | [^coords] |
| `WM_MOUSEWHEEL` | `HIWORD` = wheel delta; `LOWORD` = `MK_*` flags | packed signed screen `x,y` | [^coords] |
| `WM_MOUSEHOVER` | key/button state flags (`MK_*`) | packed signed client `x,y` | [^coords] |
| `WM_MOUSELEAVE` | unused | unused | |
| `WM_CAPTURECHANGED` | unused | `HWND` gaining capture | |
| `WM_WINDOWPOSCHANGING` | unused | `WINDOWPOS*` | |
| `WM_WINDOWPOSCHANGED` | unused | `WINDOWPOS*` | Load-bearing generator for `WM_SIZE`/`WM_MOVE`. |
| `WM_SIZE` | `SIZE_*` resize type | `LOWORD` = client width; `HIWORD` = client height | [^sizes] |
| `WM_MOVE` | unused | packed client-area upper-left `x,y` | [^sizes] |
| `WM_SHOWWINDOW` | shown flag (`TRUE`/`FALSE`) | status code (`SW_*` reason) | |
| `WM_GETMINMAXINFO` | unused | `MINMAXINFO*` | |
| `WM_ENTERSIZEMOVE`, `WM_EXITSIZEMOVE` | unused | unused | |
| `WM_DPICHANGED` | `LOWORD` = X DPI; `HIWORD` = Y DPI | suggested window `RECT*` | |
| `WM_SETTEXT` | unused | zero-terminated string pointer | |
| `WM_GETTEXT` | buffer capacity in characters | output buffer pointer | |
| `WM_GETTEXTLENGTH` | unused | unused | |
| `WM_COMMAND` | packed source/id/notification | source-dependent, often `HWND` or `0` | [^command] |
| `WM_NOTIFY` | control id, or `0` | `NMHDR*` (or larger struct beginning with `NMHDR`) | |
| `WM_CTLCOLOR*` | child/control `HDC` | child/control `HWND` | |
| `WM_INITMENU` | `HMENU` | unused | |
| `WM_INITMENUPOPUP` | popup `HMENU` | `LOWORD` = popup position; `HIWORD` = system-menu flag | |
| `WM_MENUSELECT` | `LOWORD` = item/index; `HIWORD` = menu flags | `HMENU`; `NULL` when menu closed | [^menuselect] |
| `WM_MENUCOMMAND` | selected item index | `HMENU` | Requires `MNS_NOTIFYBYPOS`. |
| `WM_UNINITMENUPOPUP` | popup `HMENU` | `HIWORD` = menu type (`MF_SYSMENU` when system menu) | |
| `WM_HSCROLL`, `WM_VSCROLL` | `LOWORD` = scroll request; `HIWORD` = thumb position for thumb messages | scroll-bar `HWND`, or `0` for a standard window scroll bar | [^scroll] |
| `WM_TIMER` | timer id | `TIMERPROC`, or `0` | |
| `WM_HOTKEY` | hotkey id | `LOWORD` = modifier flags; `HIWORD` = virtual-key code | |
| `WM_CLIPBOARDUPDATE` | unused | unused | |
| `WM_DRAWCLIPBOARD` | unused | unused | |
| `WM_CHANGECBCHAIN` | removed viewer `HWND` | next viewer `HWND` | Legacy clipboard-viewer chain. |
| `WM_DROPFILES` | `HDROP` | unused | |
| `WM_DEVICECHANGE` | `DBT_*` event code | `DEV_BROADCAST_HDR*` or `0` | Event-specific. |
| `WM_POWERBROADCAST` | `PBT_*` event code | event-specific; `POWERBROADCAST_SETTING*` for `PBT_POWERSETTINGCHANGE` | |
| `WM_QUERYENDSESSION` | unused | `ENDSESSION_*` reason flags | |
| `WM_ENDSESSION` | ending flag (`TRUE`/`FALSE`) | `ENDSESSION_*` reason flags | |
| `WM_INPUTLANGCHANGEREQUEST` | request flags | input locale `HKL` | Load-bearing if forwarded. |
| `WM_INPUTLANGCHANGE` | character set | input locale `HKL` | |
| `WM_HELP` | usually unused | `HELPINFO*` | |
| `WM_QUERYDRAGICON` | unused | unused | |

[^nccalc]: `WM_NCCALCSIZE` switches the pointed-to structure based on `wParam`; do not treat `lParam` as one fixed layout.
[^ncactivate]: For `WM_NCACTIVATE`, `lParam == -1` tells `DefWindowProcW` not to repaint the non-client area for the activation change. Otherwise it may identify the previously active or next active window.
[^coords]: Mouse coordinates packed in `lParam` are signed 16-bit values. Use sign extension (`GET_X_LPARAM`/`GET_Y_LPARAM` style), not unsigned `LOWORD`/`HIWORD`.
[^setcursor]: `WM_SETCURSOR` is also parent-routed by `DefWindowProcW`; returning `TRUE` stops further cursor processing.
[^syscommand]: The low 4 bits of `WM_SYSCOMMAND`'s `wParam` are system-reserved. Mask with `0FFF0h` before comparing. `lParam` is not one universal shape; it depends on how the system command was produced.
[^keybits]: Keyboard `lParam` is a bitfield: repeat count, scan code, extended-key flag, context bit, previous-state bit, and transition bit.
[^mouseactivate]: `WM_MOUSEACTIVATE` uses a parent-window handle in `wParam`, then packs the hit-test result and triggering mouse message into `lParam`.
[^mousebuttons]: For `WM_XBUTTON*`, the high word of `wParam` identifies `XBUTTON1`/`XBUTTON2`; the low word still carries `MK_*` flags.
[^sizes]: `WM_SIZE` and `WM_MOVE` use packed 16-bit fields; this is convenient but not a substitute for querying full rectangles when large or multi-monitor coordinates matter.
[^command]: `WM_COMMAND` depends on the source: menu (`HIWORD(wParam)=0`, `LOWORD` item id, `lParam=0`), accelerator (`HIWORD=1`, `LOWORD` id, `lParam=0`), or control (`HIWORD` notification, `LOWORD` control id, `lParam=HWND`).
[^menuselect]: Menu closure is reported as `HIWORD(wParam)=0FFFFh` with `lParam=NULL`.
[^scroll]: For standard window scroll bars, `lParam` is `0`; for scroll-bar controls it is the control `HWND`. The 16-bit thumb position in `HIWORD(wParam)` is limited; query the full scroll info when precision matters.

---

## Why a window may not receive a given message

Not every window receives every message. A message can be gated by a style flag set at creation, by an opt-in registration call that must precede it, by the system being in a particular mode, by another handler that generates it as a side effect, or by what the message loop and owning thread are doing. When a message that "should" arrive does not, the cause is usually one of the following — and is frequently *outside* the window procedure.

### Gated by window or class style

| Message(s) | Requires | Note |
|---|---|---|
| `WM_*BUTTONDBLCLK` (e.g. `WM_LBUTTONDBLCLK`) | `CS_DBLCLKS` in the window class | Without it a double-click is delivered as two ordinary down/up pairs and the double-click message never appears. |
| `WM_HSCROLL` / `WM_VSCROLL` (from the built-in bars) | `WS_HSCROLL` / `WS_VSCROLL` (or a scroll-bar control) | No scroll-bar styles → no standard scroll-bar input. |
| `WM_DROPFILES` | `WS_EX_ACCEPTFILES` (set directly or via `DragAcceptFiles`) | Files dropped on a window without this style generate nothing. |
| `WM_PAINT` | `WS_VISIBLE` and a non-empty update region | A hidden window accumulates invalidation but is not painted until shown. |
| Mouse / keyboard / focus input | The window must be enabled | A `WS_DISABLED` window receives no input and cannot take focus; input is routed elsewhere. |
| Full-client repaint on resize | `CS_HREDRAW \| CS_VREDRAW` | Without these, only the newly exposed region is invalidated on a size change, not the whole client area. |

### Gated by an opt-in API call

These messages are never sent until a registration call arms them.

| Message | Armed by |
|---|---|
| `WM_TIMER` | `SetTimer` (and the timeout elapsing) |
| `WM_HOTKEY` | `RegisterHotKey` |
| `WM_MOUSELEAVE` / `WM_MOUSEHOVER` | `TrackMouseEvent` (re-armed per tracking session) |
| `WM_CAPTURECHANGED` | `SetCapture` (sent when capture is lost) |
| `WM_CLIPBOARDUPDATE` | `AddClipboardFormatListener` (legacy chain: `SetClipboardViewer` → `WM_DRAWCLIPBOARD` / `WM_CHANGECBCHAIN`) |
| Device-interface `WM_DEVICECHANGE` (`DBT_DEVICEARRIVAL`) | `RegisterDeviceNotification` (volume / `DBT_DEVNODES_CHANGED` broadcasts arrive without it) |
| Setting-specific `WM_POWERBROADCAST` (`PBT_POWERSETTINGCHANGE`) | `RegisterPowerSettingNotification` (suspend/resume broadcasts arrive without it) |

### Gated by system mode or state

| Message | Condition |
|---|---|
| `WM_ENTERSIZEMOVE` / `WM_EXITSIZEMOVE` | Sent only around an **interactive** modal move/size loop. Programmatic `SetWindowPos` / `MoveWindow` do not produce them. |
| `WM_DPICHANGED` | Sent only to windows in a **Per-Monitor** DPI-awareness mode. Under System or Unaware awareness the window is bitmap-scaled and this message never arrives. |
| `WM_INITMENU`, `WM_INITMENUPOPUP`, `WM_MENUSELECT`, `WM_UNINITMENUPOPUP` | Require a menu to exist and be interacted with. |
| `WM_MENUCOMMAND` (instead of `WM_COMMAND`) | Requires the `MNS_NOTIFYBYPOS` menu style; otherwise menu selections arrive as `WM_COMMAND` carrying the item ID. |

### Generated only as a side effect of another handler

These never originate on their own; they exist because handling some *other* message produced them. Intercept the producer without forwarding and the dependent messages vanish.

| Message | Produced by |
|---|---|
| `WM_SIZE`, `WM_MOVE`, `WM_SHOWWINDOW` | `DefWindowProcW`'s handling of `WM_WINDOWPOSCHANGED` (and `SWP_SHOWWINDOW`/`SWP_HIDEWINDOW`) |
| `WM_ERASEBKGND` | `BeginPaint` (or `RedrawWindow(RDW_ERASENOW)`) when the region's erase flag is set — never raised independently |
| `WM_PAINT` | The existence of an invalid region — from `InvalidateRect`/`InvalidateRgn`, `RedrawWindow`, `ScrollWindow`, or an uncovered area |
| `WM_CTLCOLOR*` | A child control of that class painting and asking its parent for colors |
| `WM_COMMAND` / `WM_NOTIFY` | Child controls, menus, or accelerator tables — absent those, neither arrives |
| `WM_SETCURSOR`, `WM_NC*BUTTON*` | Selected by the `WM_NCHITTEST` result for the cursor position |

### Gated by the message loop

Some messages are synthesized by translation steps in the loop, not by the window itself. A procedure that "never gets" them is usually behind a loop that omits the translating call.

| Message | Requires in the loop |
|---|---|
| `WM_CHAR` / `WM_SYSCHAR` | `TranslateMessage` (which posts them from `WM_KEYDOWN` / `WM_SYSKEYDOWN`) |
| Dialog navigation, `WM_GETDLGCODE` | `IsDialogMessage` |

### Gated by thread pumping

A window's messages flow only while its **owning thread** runs a message loop. A window created on a thread that does not pump receives no queued or synthesized messages — `WM_PAINT`, `WM_TIMER`, and input all stall, and the window appears hung. (Sent messages can still be serviced when that thread blocks at certain points, but posted and synthesized messages cannot.)

---

## Creation handshake

Window creation is a two-message conversation, and both messages gate creation on their return value.

| Message | Default return | Meaning of the return | Effect of swallowing |
|---|---|---|---|
| `WM_NCCREATE` | `TRUE` | `TRUE` continues creation; `FALSE` aborts and `CreateWindowExW` returns `NULL` | The default `WM_NCCREATE` is where the **caption text is stored** (from `lpWindowName`) and scroll-bar state is initialized. Returning `TRUE` without forwarding leaves the window with no title — `WM_GETTEXT` / `GetWindowTextW` return empty and the caption draws blank. |
| `WM_CREATE` | `0` | `0` continues; `-1` aborts (window destroyed, `CreateWindowExW` returns `NULL`) | Forwarding yields `0` = continue. A handler must return `0`; an accidental or stale `-1` in `rax` kills the window. |

The default forward is correct for both, but `WM_NCCREATE` is load-bearing for the title. A handler that intercepts it to stash a `this` pointer from `CREATESTRUCT.lpCreateParams` into `GWLP_USERDATA` should **call `DefWindowProcW` and propagate its return** rather than returning `TRUE` outright, or the caption is lost.

---

## Destruction sequence

Three messages, in this order. Only one is the application's responsibility.

| Message | Default behavior | Note |
|---|---|---|
| `WM_CLOSE` | Calls `DestroyWindow(hwnd)` | This is why the close button works with no code. Intercept only to prompt before closing; otherwise defer. |
| `WM_DESTROY` | Returns `0`. **Does not** call `PostQuitMessage` | The most common message-loop bug is relying on the default to quit. The default tears down the window but never posts `WM_QUIT`. The main window must call `PostQuitMessage` itself, or the loop spins on a dead window. |
| `WM_NCDESTROY` | Frees memory the system allocated for the window; returns `0`. The **last** message a window receives | Anything stored in `GWLP_USERDATA` that needs releasing is released here. The internal free must run, so do not swallow it. |

---

## Paint and erase

| Message | Default behavior | Return |
|---|---|---|
| `WM_PAINT` | Calls `BeginPaint`/`EndPaint`, which **validates the update region** (and dispatches `WM_ERASEBKGND` if the erase flag is set). Draws nothing else for a generic class. | `0` |
| `WM_ERASEBKGND` | Fills the client area with the class brush `WNDCLASS.hbrBackground`. Returns non-zero if it erased, `0` if `hbrBackground` is `NULL`. | non-zero / `0` |
| `WM_NCPAINT` | Paints the non-client frame: border, caption, scroll bars, sizing grip. `wParam` = update-region `HRGN` (or `1` for the whole frame). | `0` |

A key dependency: `WM_ERASEBKGND` is *emitted by `BeginPaint`*, not raised independently when a region goes dirty. A `WM_PAINT` handler that calls only `ValidateRect` keeps the region-clear but loses both the paint DC and the erase dispatch — and stops presenting, because nothing acquires a present-capable DC. A swap-chain renderer (DXGI/DComp/D2D) legitimately uses `ValidateRect`-only `WM_PAINT`; a GDI-in-the-cycle renderer must run `BeginPaint`/`EndPaint`.

---

## Non-client frame — the routing layer

This cluster is the core of `DefWindowProcW`. Intercepting any of it without forwarding generally means rebuilding the frame by hand.

| Message | Default behavior | Note |
|---|---|---|
| `WM_NCCALCSIZE` | Computes the client rectangle from the frame. `wParam == FALSE`: `lParam`→`RECT`, adjusted in place, returns `0`. `wParam == TRUE`: `lParam`→`NCCALCSIZE_PARAMS`, returns `0` (client = `rgrc[0]`). | The `wParam == TRUE` return is a bitmask of `WVR_*` flags; `0` means "use the computed rect." This is the hook for borderless/custom frames: handle it, do not forward, and report a custom client rect. |
| `WM_NCHITTEST` | Returns an `HT*` code for the cursor position. | `lParam` packs **signed** screen coordinates — sign-extend the words (`GET_X_LPARAM`/`GET_Y_LPARAM`), not the unsigned `LOWORD`/`HIWORD`. Codes include `HTCLIENT`, `HTCAPTION`, the edge/corner codes, `HTSYSMENU`, `HTMINBUTTON`/`HTMAXBUTTON`/`HTCLOSE`, `HTNOWHERE`, `HTTRANSPARENT`. Returning `HTCAPTION` for a client point makes the window draggable by its interior. |
| `WM_NCACTIVATE` | Redraws the caption/frame for the active/inactive state. | Returns `TRUE` to allow the activation redraw; `wParam` is the active flag. |
| `WM_NCLBUTTONDOWN` | Translates the `HT*` code in `wParam` into a `WM_SYSCOMMAND`: `HTCAPTION`→`SC_MOVE` (modal move loop), edge codes→`SC_SIZE`, `HTSYSMENU`→system menu, the caption-button codes→the matching `SC_*`. | The bridge from a frame click to actual window management. |
| `WM_NCMOUSEMOVE` / `WM_NCMOUSELEAVE` | Track hover state for caption-button hot tracking. | — |
| `WM_SETCURSOR` | Sets the cursor: `HTCLIENT` → class cursor (`WNDCLASS.hCursor`); non-client codes → the matching system cursor. Passes to the parent first when one exists. | `wParam` = hwnd under cursor, `LOWORD(lParam)` = hit-test code, `HIWORD(lParam)` = triggering mouse message. Returning `TRUE` halts further processing; forwarding continues the chain. |

---

## `WM_SYSCOMMAND` — the window-management vocabulary

The default `WM_SYSCOMMAND` handler implements window management from the window's side. The command is in the **upper bits** of `wParam`; the low 4 bits are system-reserved and must be masked off (`and wParam, 0FFF0h`) before comparison.

| `SC_*` (in `wParam & 0xFFF0`) | Default action |
|---|---|
| `SC_MOVE` | Modal move loop (mouse + arrow keys) |
| `SC_SIZE` | Modal resize loop |
| `SC_MINIMIZE` / `SC_MAXIMIZE` / `SC_RESTORE` | Window state transitions |
| `SC_CLOSE` | Sends `WM_CLOSE` (→ default `DestroyWindow`) |
| `SC_KEYMENU` | Keyboard menu activation (the Alt/F10 path) |
| `SC_MOUSEMENU` | Mouse menu activation |
| `SC_HSCROLL` / `SC_VSCROLL` | Scroll-bar interaction |
| `SC_CONTEXTHELP` | "?" help cursor mode |
| `SC_SCREENSAVE` / `SC_MONITORPOWER` | Screensaver / display-power requests |
| `SC_HOTKEY` / `SC_TASKLIST` | Window hotkey activation / task switcher |
| `SC_NEXTWINDOW` / `SC_PREVWINDOW` | Cycle windows |

Application-defined system-menu items use values `>= 0xF000` placed in the upper bits, handled before the rest is forwarded. This is the densest concentration of free behavior in the procedure.

---

## Keyboard defaults

Ordinary keys are almost entirely the application's; **system** keys carry the defaults.

| Message | Default behavior | Load-bearing? |
|---|---|---|
| `WM_KEYDOWN` / `WM_KEYUP` | Essentially nothing; returns `0`. | No. |
| `WM_CHAR` | Nothing; returns `0`. | No. |
| `WM_SYSKEYDOWN` | Handles Alt/F10 menu activation, **Alt+Space** (system menu), Alt+mnemonic, and **Alt+F4** → `WM_SYSCOMMAND`/`SC_CLOSE`. | **Yes.** Intercepting without forwarding the unhandled cases disables Alt+F4 and the menu keyboard interface. |
| `WM_SYSKEYUP` | Completes the above. | Yes. |
| `WM_SYSCHAR` | Selects a menu item by mnemonic; `MessageBeep` when the key matches nothing in menu mode. | Yes, for menu mnemonics. |

`WM_KEYDOWN` / `WM_CHAR` can be filtered freely; `WM_SYSKEYDOWN` / `WM_SYSCHAR` must forward their unhandled cases to preserve Alt+F4 and the menu bar.

---

## Mouse and activation

| Message | Default behavior | Return |
|---|---|---|
| `WM_MOUSEACTIVATE` | Activates the window on click. | `MA_ACTIVATE`. `MA_NOACTIVATE` / `MA_ACTIVATEANDEAT` / `MA_NOACTIVATEANDEAT` refine click-to-activate. |
| `WM_LBUTTONDOWN` etc. (client) | Little to nothing. | `0` |
| `WM_CONTEXTMENU` | Forwards to the **parent** window (bubbles up). | — |
| `WM_MOUSEWHEEL` | Not scrolled by default for a plain window. | `0` |

---

## Sizing, moving, min/max — the generation chain

The messages that look primary here are in fact **manufactured by `DefWindowProcW`**.

| Message | Default behavior |
|---|---|
| `WM_WINDOWPOSCHANGING` | Lets the system finalize the pending position; minimal default. |
| `WM_WINDOWPOSCHANGED` | **Sends `WM_SIZE` and `WM_MOVE`** (when size/position changed and `SWP_NOSIZE`/`SWP_NOMOVE` are not set), and `WM_SHOWWINDOW` for `SWP_SHOWWINDOW`/`SWP_HIDEWINDOW`. |
| `WM_SIZE` / `WM_MOVE` | Returns `0`; the geometry change already happened. |
| `WM_GETMINMAXINFO` | Fills `MINMAXINFO` with default max size/position and min/max **track** sizes from system metrics and window style. |

The consequence in one sentence: **`WM_SIZE` and `WM_MOVE` are not sent when `WM_WINDOWPOSCHANGED` is handled without calling `DefWindowProcW`.** Using `WM_WINDOWPOSCHANGED` as a single layout chokepoint and returning without forwarding silences every `WM_SIZE` handler. Either forward after the work, or read the new rectangle directly from the `WINDOWPOS` in `lParam`. Likewise, a `WM_GETMINMAXINFO` handler that does not forward must populate `MINMAXINFO` itself, or sizing constraints revert to defaults.

---

## Window text — the system owns the caption buffer

The default procedure maintains the window's title string internally. These three messages access that buffer; `GetWindowTextW`/`SetWindowTextW` are wrappers that send them.

| Message | Default behavior |
|---|---|
| `WM_SETTEXT` | Stores the string into the internal caption buffer, redraws the non-client caption, returns `TRUE`. |
| `WM_GETTEXT` | Copies the stored caption into the caller's buffer; returns the character count. |
| `WM_GETTEXTLENGTH` | Returns the stored caption's length. |

This buffer is seeded by the default `WM_NCCREATE` from `lpWindowName`, which is why that message is load-bearing. Overriding the text storage requires answering all three messages consistently against whatever holds the string.

---

## Session, language, theming, miscellaneous

| Message | Default behavior | Note |
|---|---|---|
| `WM_QUERYENDSESSION` | Returns `TRUE` (OK to shut down). | Return `FALSE` to block logoff/shutdown. |
| `WM_ENDSESSION` | Returns `0`. | — |
| `WM_INPUTLANGCHANGEREQUEST` | **Performs the keyboard-layout switch** to the requested layout. | **Load-bearing.** Intercepting without forwarding stops the language hotkey from switching layouts. |
| `WM_INPUTLANGCHANGE` | Notification after the switch; default returns `TRUE`. | — |
| `WM_CTLCOLOR*` (`BTN`/`EDIT`/`STATIC`/`LISTBOX`/`SCROLLBAR`/`DLG`/`MSGBOX`) | Selects default system colors into the child's DC and returns the default background brush. | To theme a control, return a custom `HBRUSH` **and** set the DC's text/back colors, or text becomes invisible against the new background. |
| `WM_GETDLGCODE` | Returns `0` for a plain window. | Relevant only to controls/dialogs (sent by `IsDialogMessage`). |
| `WM_HELP`, `WM_DROPFILES`, `WM_DEVICECHANGE`, `WM_QUERYDRAGICON` | Minimal / `0`. | Handle as needed. |

---

## Load-bearing messages

Intercepting any of these and returning **without** forwarding loses the listed behavior.

| Message | Lost by swallowing |
|---|---|
| `WM_NCCREATE` | Caption text storage; scroll-bar init (creation aborts on `FALSE`) |
| `WM_CLOSE` | The window no longer closes (no `DestroyWindow`) |
| `WM_NCDESTROY` | Internal window-memory free (plus `RemoveWindowSubclass` under a subclass — see *Forwarding inside a subclass*) |
| `WM_WINDOWPOSCHANGED` | **`WM_SIZE`, `WM_MOVE`, `WM_SHOWWINDOW` generation** |
| `WM_SYSCOMMAND` (unhandled cases) | Move/size/min/max/close/menu — the window manager |
| `WM_NCLBUTTONDOWN` | Frame-click → move/size/menu translation |
| `WM_NCHITTEST` | Frame routing (the border becomes non-interactive) |
| `WM_SETCURSOR` | Resize/arrow cursors over the frame |
| `WM_SYSKEYDOWN` (unhandled cases) | Alt+F4, Alt+Space, F10 menu activation |
| `WM_INPUTLANGCHANGEREQUEST` | Keyboard-layout switching |
| `WM_GETMINMAXINFO` | Default sizing limits (unless the struct is filled) |

The inverse — the **always-the-application's** message — is `WM_DESTROY`. The default does not `PostQuitMessage`.

---

## Return values that matter

| Message | Return to **continue/allow** | Return to **abort/deny** |
|---|---|---|
| `WM_NCCREATE` | `TRUE` | `FALSE` (creation fails) |
| `WM_CREATE` | `0` | `-1` (creation fails) |
| `WM_QUERYENDSESSION` | `TRUE` | `FALSE` (blocks shutdown) |
| `WM_ERASEBKGND` | non-zero = "erased" | `0` = "not erased" |
| `WM_MOUSEACTIVATE` | `MA_ACTIVATE` | `MA_NOACTIVATE` (+ `…ANDEAT` to also eat the click) |
| `WM_SETCURSOR` | `TRUE` = handled, stop | `FALSE` = keep processing |

For any message not listed, "handled" is `0` and "not handled" is a forward to `DefWindowProcW`.

---

## The `DefWindowProc` family

Several procedures occupy the same slot — the target a window procedure forwards an unclaimed message to — and the correct one must be used, or behavior is lost. Three are **terminal** defaults that layer additional handling on top of `DefWindowProcW`; the fourth is not terminal at all but a **chain forwarder**.

| Default proc | Role | Fallback for |
|---|---|---|
| `DefWindowProcW` | The base — everything cataloged above | Ordinary windows |
| `DefDlgProc` | Adds Tab/arrow navigation, default-button (`IDOK`/`IDCANCEL`) handling, focus management, `DM_*` | Custom dialog classes (`cbWndExtra >= DLGWINDOWEXTRA`) |
| `DefFrameProc` | Adds MDI frame behavior; forwards window-menu and child activation | MDI frame windows |
| `DefMDIChildProc` | Adds MDI child sizing, activation, system-menu integration | MDI child windows |
| `DefSubclassProc` | Passes the message to the **next handler in the subclass chain** (eventually the original window procedure, and only then a terminal default) | Subclass procedures installed via `SetWindowSubclass` |

For an ordinary top-level window, `DefWindowProcW` is the whole story. A hand-rolled dialog-class procedure must fall through to `DefDlgProc` instead, or Tab navigation and the default button stop working. A subclass procedure forwards to `DefSubclassProc`, covered next.

---

### Forwarding inside a subclass

Subclassing through the common-controls API (`SetWindowSubclass`, `RemoveWindowSubclass`, `DefSubclassProc`) inserts a procedure ahead of a window's existing one. Several subclasses form a chain that terminates at the original window procedure, which in turn forwards to one of the terminal defaults above. Everything in this reference still applies inside a subclass, with three adjustments.

**Forward to `DefSubclassProc`, not `DefWindowProcW`.** From a subclass, "forward the unclaimed message" means `DefSubclassProc` — it hands the message to the next handler in the chain, reaching the original procedure (and only then a terminal default). Calling `DefWindowProcW` directly from a subclass bypasses every handler below it *and* the original procedure; for a system control that means bypassing the control's own implementation, a subtle and hard-to-trace defect. (The comctl32 API is preferred over swapping `GWLP_WNDPROC` by hand precisely because it keeps chain order and removal safe and carries per-instance reference data.)

**Handlers take one of three shapes** — the concrete form of the claim-versus-forward decision:

| Shape | Form | Use |
|---|---|---|
| Replace | Handle fully, return without forwarding | Owner-draw and protocol messages that suppress the default and return their own result |
| Pre-process | Act first, then forward to `DefSubclassProc` | Adjusting or acting ahead of the default |
| Wrap | Forward to `DefSubclassProc` first, capture its return value and side effects, do additional work, return the captured value | Letting the default paint, then drawing over it |

**`WM_NCDESTROY` carries a second obligation.** Beyond the OS cleanup the default `WM_NCDESTROY` performs — which must still be forwarded — a subclass calls `RemoveWindowSubclass` here and releases anything its `dwRefData` owns. The conventional ordering is to forward to `DefSubclassProc` **first**, so the rest of the chain processes the final message while the chain is intact, and remove afterward; removing before the forward can disturb the chain walk `DefSubclassProc` relies on.

---

### Summary

A window procedure claims the messages whose behavior it defines and forwards the rest to the appropriate default procedure. The judgment calls are two: the dozen load-bearing messages whose default *generates* other messages or state, which must be forwarded even when the procedure also does work; and the handful of return values that gate creation, activation, and shutdown. Before any of that applies, though, the message must actually arrive — and whether it does is governed by style flags, registration calls, system mode, side-effect generation, the message loop, and the owning thread.
