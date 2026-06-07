# fasm2 Coding Guide

This guide records the current house style for Win64 fasm2 examples in this
repository. It is the practical layer above `docs/proc64-reference.md`: use that
document for macro internals, and use this one when writing or reviewing code.

The style is intentionally register-first and low-boilerplate. Make the Win32
contract visible, keep state ownership explicit, and avoid moves or wrappers
that only make the assembly look higher level.

## Build And Review Baseline

Build from a Visual Studio developer environment. For examples with `_build.cmd`,
that script is the normal validation path.

Assembler success is required but not enough for UI work. If a change touches
message routing, painting, control state, input, or persistence, run a focused
smoke test that exercises that surface.

Before editing fasm2 code:

- read the local include or module that owns the behavior;
- read `docs/code_policy.md` for repo policy;
- read `docs/proc64-reference.md` when changing call setup, local frames, or
  macro-heavy code.

## Module Shape

Prefer small modules that own one feature or lifetime. A reusable include should
hide the data it owns instead of requiring the entry source to remember
feature-specific section emitters.

Good:

```asm
macro clear_edit_bss
	align 8
	clear_edit_arrow_cursor dq ?
	clear_edit_empty_text dw 0
purge clear_edit_bss
end macro
define __GLOBAL_BSS__ clear_edit_bss
```

Avoid making the host repeat boilerplate such as:

```asm
define __GLOBAL_DATA__ clear_edit_data
define __GLOBAL_BSS__  clear_edit_bss
```

when the module can register those emitters itself.

BSS emitters must own alignment. Start with `align 8` when the emitter contains
handles, pointers, qwords, or structures with pointer fields.

Keep host requirements command-sized: initialize the module, attach it to a
control or object, and send any documented messages. Do not expose internal
buffers or helper ordering unless the host genuinely owns that state.

## Data And Structure Initializers

Leave zero values implicit. Do not spell fields such as `cbClsExtra: 0`,
`hIcon: 0`, `iString: 0`, or placeholder structure members unless the zero is
the point being documented.

Good:

```asm
wc WNDCLASSEX cbSize: sizeof.WNDCLASSEX,\
	style: CS_HREDRAW or CS_VREDRAW,\
	lpfnWndProc: WindowProc,\
	hbrBackground: COLOR_WINDOW + 1,\
	lpszClassName: class_name
```

Use `align` for required layout padding instead of anonymous zero fields:

```asm
struct ECLEAR
  pad	     dd ?
  hit	     dd ?
  icon_px    dd ?
  hover      dd ?
  tracking   dd ?
  has_text   dd ?
	     align 8
  font	     dq ?
ends
```

Exception: structure definitions that mirror Win32 records and are instantiated
as proc locals may need explicit padding fields. The proc-local address has a
variable component, and `align` inside the structure can fail assembly. Treat
those padding fields as structural layout, not dead state.

Keep structure state only when it varies by instance or is part of a real
contract. A constant color, glyph, or face name that is not per-instance state
should stay a constant or inline literal.

## Strings

Use inline strings in `invoke` or `fastcall` arguments when the call does not
need a stable named address:

```asm
invoke	CreateFontW,eax,0,0,0,FW_NORMAL,0,0,0,\
	DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,\
	CLEAR_EDIT_QUALITY,DEFAULT_PITCH or FF_DONTCARE,'Segoe MDL2 Assets'
```

Use `GLOBSTR` or `GLOBWSTR` only when the address has identity outside one call:
class names, resource names, structure fields, pointer tables, mutable buffers,
filters, registry keys, or strings referenced by multiple structures.

The local `windows.inc` policy normally enables reusable inline strings. Let
that machinery remove repeated literal data before inventing a named constant.

## Calling Convention

Use `fastcall` for local `proc` labels. Use `invoke` for imported APIs and
function pointers.

```asm
fastcall ClearEdit_Invalidate,[hwnd],rbx
invoke	InvalidateRect,[hwnd],addr rc,1
```

`invoke SomeApi,...` expands to an indirect call through `[SomeApi]`. That is
right for imports and wrong for local procedures.

### Parameters Are Home Slots

`proc` parameter labels are stack home-slot addresses, not aliases for incoming
registers. Spill only what you need after a call or what makes later code
materially clearer.

Good:

```asm
proc ClearEdit_GlyphRect hwnd,statep,rectp
    locals
	cr RECT
    endl
	mov	[statep],rdx
	mov	[rectp],r8
	invoke	GetClientRect,rcx,addr cr
	mov	rcx,[statep]
	mov	rdx,[rectp]
```

Here `rcx` is used directly for the first call because it is still the incoming
`hwnd`. `rdx` and `r8` are spilled because they are needed after
`GetClientRect` clobbers volatile registers.

Avoid entry blocks that mechanically spill every argument:

```asm
mov	[hwnd],rcx
mov	[statep],rdx
mov	[rectp],r8
```

unless each value is actually needed after calls.

### Match Register Width To The Value

Use the ABI register that corresponds to the argument position and the value
width. If a DWORD is the fourth argument, load `r9d`. If a pointer is the fourth
argument, load `r9`.

Good:

```asm
invoke	GetDlgCtrlID,[hwnd]
mov	r8d,EN_CHANGE
shl	r8d,16
or	r8d,eax
mov	r9,[hwnd]
invoke	SendMessageW,[parent],WM_COMMAND,r8,r9
```

This builds `wParam` in the third-argument register and `lParam` in the
fourth-argument register. It avoids a temporary `cmd` local and makes the
Win32 message contract visible.

When a register is already the right argument, pass it directly:

```asm
invoke	SetTextColor,rsi,edx
invoke	DrawTextW,rsi,addr glyph_text,1,addr rc,\
	DT_SINGLELINE or DT_CENTER or DT_VCENTER or DT_NOPREFIX or DT_NOCLIP
```

### Respect Left-To-Right Argument Expansion

`fastcall` and `invoke` expand arguments left-to-right. A later argument may
clobber earlier volatile setup. Do not preload `rdx`, `r8`, or `r9` and then
pass a later expression that can overwrite it.

Use `addr local` for local out-parameters:

```asm
invoke	GetClientRect,rcx,addr cr
```

not:

```asm
lea	r8,[cr]
invoke	GetClientRect,rcx,r8
```

The `addr` form lets the macro load the address into the correct slot at the
right point.

Nested `invoke` or `fastcall` arguments are supported but fragile. Save any
earlier volatile input to a nonvolatile register or memory before the nested
call.

## Operand Sizes

Let typed labels and structure fields carry their size. Prefer:

```asm
mov	[rbx+ECLEAR.icon_px],CLEAR_EDIT_ICON_PX
cmp	[rdx+ECLEAR.has_text],0
mov	[x],eax
```

over:

```asm
mov	dword [rbx+ECLEAR.icon_px],CLEAR_EDIT_ICON_PX
cmp	dword [rdx+ECLEAR.has_text],0
mov	dword [x],eax
```

Use an explicit size when the target is untyped, intentionally narrower than
the natural operand, or ambiguous to the macro:

```asm
mov	dword [wmsg],edx        ; proc parameter home slot is qword-sized
mov	byte [rdi],0
mov	qword [rsp+20h],0
```

For proc parameters, remember that bare parameter labels are home slots. A
`UINT` parameter stored into its home slot should usually be written with a
32-bit destination.

## Register Lifetime

Use nonvolatile registers when a value must survive multiple calls. Keep the
`uses` list narrow and honest.

Good:

```asm
proc ClearEdit_DrawGlyph uses rbx rsi, hwnd,statep,hdc
	...
	mov	rsi,r8        ; hdc survives SelectObject/SetBkMode calls
	mov	rbx,rdx       ; state survives calls
```

Do not put a register in `uses` unless the procedure mutates it and needs it
preserved for the caller.

Use `jrcxz` for qword handle/pointer null tests when the value is already in
`rcx`:

```asm
mov	rcx,[rbx+ECLEAR.font]
jrcxz	no_font
invoke	DeleteObject,rcx
```

This is a good fit for cleanup paths and avoids another memory load.

## Helpers And Abstractions

A helper should own a real idea: geometry, lifetime, state transition,
marshalling hazard, repeated drawing, parsing, or cleanup.

Do not keep a helper that only renames one API call or makes a one-line call
look uniform. Inline it at the call site.

Good helper boundaries from `clear_edit.inc`:

- `ClearEdit_GlyphRect` owns hit-target geometry.
- `ClearEdit_HitTest` owns text-present gating plus rect bounds.
- `ClearEdit_Clear` owns state reset, edit clearing, and parent notification.

Bad helper boundary:

```asm
proc ClearEdit_TextChanged hwnd,has_text
	invoke	SendMessageW,[hwnd],ECM_TEXTCHANGED,dword [has_text],0
	ret
endp
```

The call site is clearer as the real message:

```asm
invoke	SendMessageW,[hFilter],ECM_TEXTCHANGED,[filter_has_text],0
```

## Control Flow

Within a `proc`, branch labels are scoped by the proc namespace. Short labels
such as `done`, `fail`, `cleanup`, and `default` are fine when they describe the
local path.

Prefer one shared default or cleanup path when several branches all fall back to
the same Win32 behavior:

```asm
default:
	invoke	DefSubclassProc,[hwnd],dword [wmsg],[wparam],[lparam]
	ret
```

Use `iterate` when repetition is structural and the repeated items form a list
a reader will maintain:

```asm
iterate <message,branch>,\
	WM_PAINT,	wm_paint,\
	ECM_TEXTCHANGED,ecm_textchanged,\
	WM_MOUSEMOVE,	wm_mousemove,\
	WM_MOUSELEAVE,	wm_mouseleave

	cmp	edx,message
	jz	branch
end iterate
```

Do not force `iterate` into logic where each case has a different shape. The
goal is to make the table visible, not to hide control flow.

## Win32 UI Procedures

Message procedures should make the Windows message contract easy to audit:

- save `hwnd`, `wmsg`, `wparam`, and `lparam` when later branches need them
  after calls;
- compare the live `edx` message value before any call clobbers it;
- route unhandled messages through the real default procedure;
- keep state mutation close to the message that owns it;
- return `0`, `1`, or the default procedure result according to the Win32
  message contract.

For subclass procedures, call the subclass default (`DefSubclassProc`) for
unhandled behavior. For destroy paths, call the default first when the base
control should see the teardown, then remove the subclass and release owned
objects.

For edit-control adornments, let the parent tell the adornment whether text is
present when the parent already tracks `EN_CHANGE`. Do not recalculate text
length inside paint or hit-test paths just to rediscover parent state.

## Painting

Do not cache drawing state that can be cheaply derived at paint time unless it
is expensive or externally visible state. For example, a clear glyph color can
be selected from `hover` and constants during paint; it does not need a
per-control structure field.

Select GDI objects, save the old object or mode, draw, then restore. Keep the
surface contract explicit:

```asm
invoke	SelectObject,rsi,[rbx+ECLEAR.font]
mov	[old_font],rax
invoke	SetBkMode,rsi,TRANSPARENT
mov	[old_bk],eax
...
invoke	SetBkMode,rsi,[old_bk]
invoke	SelectObject,rsi,[old_font]
```

Use font-icon glyphs as text when they are part of the UI language. This keeps
the package dogfooding its own drawing model instead of mixing in unrelated GDI
shape code.

## Cleanup

Cleanup code should be linear and should use the value already loaded when
possible:

```asm
mov	rcx,[rbx+ECLEAR.font]
jrcxz	no_font
invoke	DeleteObject,rcx
no_font:
invoke	HeapFree,[heap_handle],0,rbx
```

Free only objects this module owns. If the host owns a handle or brush, the
host releases it.

## Review Checklist

For fasm2 Win64 code reviews, check:

- local procedures are called with `fastcall`, imports/function pointers with
  `invoke`;
- proc arguments are not mechanically spilled at entry;
- values needed after calls are saved before the call;
- register width matches value width and ABI argument position;
- `addr local` is used for local out-parameters;
- no helper merely renames one API call;
- zero-valued structure fields and dead state are omitted;
- BSS/data emitters own their alignment and register themselves when reusable;
- inline literals are used unless a named string address is needed;
- nonvolatile `uses` lists are narrow;
- message procedures have one clear default path and correct return values;
- build and behavior smoke tests match the touched surface.
