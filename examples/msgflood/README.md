# MsgFlood Edit Log

This example is a Win32 message-discovery harness. It registers one main window
for many message sources and records the messages in a multiline edit control
instead of a console.

New lines are inserted at character zero with `EM_SETSEL` and `EM_REPLACESEL`.
That keeps the newest messages at the top. When the edit text approaches
`LOG_TEXT_LIMIT`, the logger deletes a tail range first, so old messages are
automatically pushed down and eventually discarded by the edit buffer policy.

## Build

Use the repository's normal developer prompt setup, then:

```bat
cd examples\msgflood
_build.cmd
```

## Controls

- `V` toggles the noise filter. With the filter off, timer, paint, mouse move,
  raw input, and window-position messages flood the log.
- `Q` closes the window.
- `Ctrl+Alt+M` triggers the registered hotkey.
- `Filters` has checkboxes for individual noisy groups. With the master noise
  filter enabled, a checked noisy group is shown and an unchecked group is
  suppressed.
- `Log > Clear` clears the edit buffer without resetting the message counter.
- `Log > Coalesce adjacent identical messages` consumes repeated adjacent
  messages with the same `msg`, `wParam`, and `lParam`. The message counter
  still advances, and the visible row changes to a range with an `xN` count.

The edit control is subclassed so `V` and `Q` still work when the log has
keyboard focus. The same subclass blocks user text mutations while allowing
selection, copy, scrolling, and programmatic log insertion.

## Message Sources

The example wires the main window for:

- raw mouse and keyboard input with `RIDEV_INPUTSINK`
- raw input device-change notification with `RIDEV_DEVNOTIFY`
- shell hook messages through `RegisterShellHookWindow`
- the registered `TaskbarCreated` message
- clipboard updates through `AddClipboardFormatListener`, resolved dynamically
- device-interface broadcast notifications
- power setting broadcasts for AC/DC source and console display state
- WTS session changes, resolved dynamically from `wtsapi32.dll`
- `Ctrl+Alt+M` hotkey messages
- a 250 ms timer
- tray icon callbacks through `Shell_NotifyIcon`
- drag-and-drop through `WS_EX_ACCEPTFILES`
- UIPI message filter updates when `ChangeWindowMessageFilterEx` is available

## Implementation Notes

`log_edit.inc` has a reentrancy guard because inserting text into the edit
control can synchronously send `WM_COMMAND/EN_CHANGE` back to the parent. Without
the guard, logging one message would generate another message to log.

The default filter posture is intentionally quiet: the master noise filter is
enabled and all noisy groups start unchecked. Use `Filters` to selectively let
through timer, raw input, mouse tracking, paint, non-client, window-position,
query, or control-color chatter.

Adjacent coalescing is enabled by default. It keeps the top-inserted edit list
useful during bursts without changing the monotonically increasing message
counter.

The tray icon uses a stable GUID identity and handles `TaskbarCreated` by
calling the tray setup path again. That follows the Windows shell lifetime
contract without making the tray icon the primary UI.

The high-volume messages stay in `message_names.inc` as data-driven `iterate`
lists. Extending the known-name table or the default noise filter means editing
one list instead of copying compare blocks.
