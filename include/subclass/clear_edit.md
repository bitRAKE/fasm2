# clear_edit.inc

`clear_edit.inc` subclasses a single-line `EDIT` control and paints a right-edge
font-icon clear affordance when the caller reports that the edit contains text.

The helper is intentionally small: the parent window remains responsible for
normal `EN_CHANGE` handling, filtering, and application state. The subclass only
tracks whether the clear glyph should be visible, draws the glyph, changes the
cursor over the hit rectangle, and clears the edit when the glyph is clicked.

## Include Model

Include the helper after the Win64 Windows include used by the examples:

```asm
include 'windows.inc'
include 'subclass/clear_edit.inc'
```

The helper contributes writable globals with:

```asm
define __GLOBAL_BSS__ clear_edit_bss
```

This requires a data model that expands `__GLOBAL_BSS__` into writable storage,
as the example `windows.inc` files do with `irpv xbss,__GLOBAL_BSS__`. A source
that uses a different format/data model must provide equivalent storage for
`clear_edit_bss`.

The include depends on the normal Win64 imports for `user32`, `gdi32`,
`kernel32`, and `comctl32` subclass APIs:

- `SetWindowSubclass`
- `DefSubclassProc`
- `RemoveWindowSubclass`
- `TrackMouseEvent`

## Public Interface

### `ClearEdit_Register`

```asm
fastcall ClearEdit_Register
```

Loads the shared arrow cursor used when the pointer is over the clear glyph.
Returns nonzero in `eax` on success. Call once during application startup before
attaching controls.

### `ClearEdit_Attach`

```asm
fastcall ClearEdit_Attach, hwndEdit, hasText
```

Attaches the subclass to a single-line edit control. `hasText` is nonzero when
the edit already contains text at attach time. Returns nonzero in `eax` on
success.

The subclass allocates one small state block from the process heap and frees it
from `WM_NCDESTROY`. It also owns the icon font handle for that edit control.

### `CLEAR_EDIT_STATUSCHANGED`

```asm
invoke SendMessageW,[hwndEdit],CLEAR_EDIT_STATUSCHANGED,textPresent,0
```

The parent sends this message to the edit after it observes an empty/non-empty
state change. `wParam` is nonzero when text is present and zero when the edit is
empty.

It is also safe to send this after every `EN_CHANGE`; sending only on transitions
avoids unnecessary invalidation.

## Clear Behavior

When the user clicks the clear glyph, the subclass:

- sets the edit text to an empty string with `SetWindowTextW`
- clears hover state
- invalidates the glyph area
- sends the parent a normal `WM_COMMAND` notification with `EN_CHANGE`

The parent should handle that `EN_CHANGE` the same way it handles user typing and
then report the resulting status back with `CLEAR_EDIT_STATUSCHANGED`.

## Customization Constants

The include currently fixes these policy values:

- `CLEAR_EDIT_SUBCLASS_ID`
- `CLEAR_EDIT_PAD`
- `CLEAR_EDIT_HIT`
- `CLEAR_EDIT_ICON_PX`
- `CLEAR_EDIT_MASK`
- `CLEAR_EDIT_MASK_HOVER`
- `ECLEAR_CHARACTER`

Change them in the include before promoting per-application overrides.
