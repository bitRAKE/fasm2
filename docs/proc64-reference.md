# `proc64.inc` for fasm2/fasmg

This document describes the macros in:

- `fasm2\include\macro\proc64.inc`

The file is a Win64-oriented call/procedure macro layer for the fasmg dialect. It is small enough to read in one sitting, but dense enough that a few details are easy to miss. Some of those details are merely convenient tricks; others are serious footguns.

The source revision inspected here is the file dated `2026-02-24 21:53:30`.

## Scope

`proc64.inc` assumes the Microsoft x64 calling convention:

- integer/pointer arguments 1..4 in `rcx`, `rdx`, `r8`, `r9`
- floating-point arguments 1..4 in `xmm0`..`xmm3`
- 32 bytes of shadow space
- 16-byte stack alignment at call sites

It is not a SysV AMD64 layer. It also does not generate unwind metadata, stack probes, or any Windows exception metadata.

## Source Map

| Lines | Topic |
| --- | --- |
| 1-25 | Register aliases for the first four argument positions |
| 27-45 | `fastcall.frame`, `frame`, `end frame`, `endf` |
| 48-59 | `fastcall.inline_string` |
| 61-246 | `fastcall` call marshaller |
| 248-254 | `invoke`, `cinvoke` |
| 256-259 | `pcountcheck`, `pcountsuffix` |
| 261-294 | Default `prologuedef` / `epiloguedef` |
| 303-505 | `proc`, `ret`, `locals`, `endl`, `proclocal` |
| 507-550 | `static_rsp_*` alternate prologue/epilogue hooks |

## Big Picture

The file contains four separate systems:

1. Call-site helpers: `fastcall`, `invoke`, `cinvoke`
2. Outgoing-stack reservation helpers: `fastcall.frame`, `frame`, `endf`
3. Procedure-definition helpers: `proc`, `ret`, `locals`, `endl`, `proclocal`
4. Prologue/epilogue customization hooks: `prologue@proc`, `epilogue@proc`, `close@proc`

These systems are related, but not fully integrated. That is why some combinations are elegant and some are surprising.

## Register Aliases

The first 25 lines define aliases such as:

- `fastcall.r1` = `rcx`
- `fastcall.r2` = `rdx`
- `fastcall.r3` = `r8`
- `fastcall.r4` = `r9`
- `fastcall.rf1` = `xmm0`
- `fastcall.rf2` = `xmm1`
- `fastcall.rf3` = `xmm2`
- `fastcall.rf4` = `xmm3`

There are also size-specific aliases:

- `fastcall.rd1` = `ecx`
- `fastcall.rw1` = `cx`
- `fastcall.rb1` = `cl`

The `fastcall` macro uses these names instead of hard-coding the registers in its body.

## `fastcall`: the Real Workhorse

`fastcall` is the core marshalling macro:

```asm
fastcall proc,args
```

It:

- classifies each argument
- places the first four arguments in the Win64 argument registers
- writes stack arguments into the outgoing call frame
- reserves at least 32 bytes of shadow space
- rounds the total call frame up to a 16-byte boundary
- emits the final `call proc`

### What `proc` Means

`fastcall` emits a literal `call proc` at the end.

That means:

- use `fastcall SomeLabel,...` for a direct call to a code label
- use `fastcall [ptr],...` for an indirect call through memory
- use `fastcall rax,...` only if `call rax` is what you want

### Argument Kinds

The macro recognizes several special cases.

| Argument form | Meaning |
| --- | --- |
| ordinary operand | integer/pointer argument |
| `float expr` | scalar floating-point argument |
| `addr expr` | address-of, implemented with `lea` |
| quoted string | turned into inline `TCHAR` data, then passed by address |
| nested `fastcall ...` or `invoke ...` | evaluate nested call first, use its `rax` result |

For ordinary arguments, the source operand size controls the move width:

- 8-byte or unsized: `mov rcx/rdx/r8/r9,...`
- 4-byte: `mov ecx/edx/r8d/r9d,...`
- 2-byte: `mov cx/dx/r8w/r9w,...`
- 1-byte: `mov cl/dl/r8b/r9b,...`

This is convenient, but it also means the macro will not automatically promote a byte or word argument to a clean 32-bit value. If the callee expects an `int`, pass an `int`.

### Shadow Space and Stack Layout

The macro counts one 8-byte slot per argument, then does:

- minimum frame size = `20h`
- otherwise enough slots for all arguments
- then rounded up to a multiple of 16

Examples:

- 0..4 arguments: `20h`
- 5 arguments: `30h`
- 6 arguments: `30h`
- 7 arguments: `40h`

Argument 5 goes to `[rsp+20h]`, argument 6 to `[rsp+28h]`, and so on.

### `invoke` and `cinvoke`

These are thin wrappers:

```asm
invoke  proc,args   ; expands to fastcall [proc],args
cinvoke proc,args   ; same thing on Win64
```

So:

- `invoke MessageBox,...` is for imported-IAT style labels or qword pointer variables
- `fastcall MessageBox,...` is for a direct label call

A very common mistake is this:

```asm
invoke MyLocalProc,1,2,3
```

That does not call `MyLocalProc` directly. It expands to `fastcall [MyLocalProc],...`, which means an indirect call through the qword stored at `MyLocalProc`.

For local procedures, use `fastcall MyLocalProc,...`.

### Floating-Point Arguments

`float expr` puts the low 32 or 64 bits into `xmm0`..`xmm3` for the first four positions, or into stack slots for later positions.

Examples:

```asm
invoke glUniform1f,[uTimeLoc],float xmm1
invoke glClearColor,float dword 0.02,float dword 0.02,float dword 0.04,float dword 1.0
```

Important limits:

- this is for scalar single/double values
- it is not a general SIMD/vector argument layer
- it does not mirror FP arguments into GP registers for varargs calls

That last point matters for Microsoft x64 varargs and unprototyped-call rules. If you need C varargs behavior with FP arguments, set the registers manually.

### `addr`

`addr expr` means "pass the address of `expr`", implemented with `lea`.

Examples:

```asm
invoke GetMessage,addr msg,NULL,0,0
invoke ChoosePixelFormat,[hdc],addr pfd
```

Use `addr symbol`, not `addr [symbol]`. The macro itself adds the addressing form.

### Strings

If an argument looks like a string literal, `fastcall` calls `fastcall.inline_string`, which by default emits inline `TCHAR` data and passes its address.

More on that below.

## Nested Calls Inside `fastcall` / `invoke`

This is where most of the cleverness lives.

The macro makes one pass over the arguments looking only for nested `fastcall` or `invoke` expressions. It evaluates those nested calls first. If there is more than one nested call, earlier `rax` results are spilled into the current outgoing-call area.

Then it makes a second pass and loads the real call arguments.

### What This Means in Practice

Nested calls are supported, but only partially:

- the nested result is assumed to live in `rax`
- earlier nested results may be spilled into the outer call frame
- non-call arguments are not "captured" before later nested calls run

So this can clobber data.

### Confirmed Hazard: Earlier Volatile Arguments

This source:

```asm
mov rcx,1234h
fastcall TargetA,rcx,fastcall TargetB
```

assembled to the equivalent of:

```asm
mov rcx,1234h
sub rsp,20h
sub rsp,20h
call TargetB
add rsp,20h
mov rdx,rax
call TargetA
add rsp,20h
```

Notice what did not happen:

- the first argument was not saved anywhere before `TargetB`
- `rcx` was reused as-is after the nested call

If `TargetB` clobbers `rcx`, the first argument is wrong.

### Safe Pattern

Save volatile inputs before a nested call:

```asm
mov rbx,rcx                 ; nonvolatile
fastcall TargetA,rbx,fastcall TargetB
```

or:

```asm
mov [saved_value],rcx
fastcall TargetA,[saved_value],fastcall TargetB
```

### More Nested-Call Limits

1. Nested FP-returning calls are not handled as FP values. The nested-call path is built around `rax`, not `xmm0`.
2. Raw `rsp`-relative memory arguments are fragile, because the macro allocates temporary outgoing space and also uses the home-area slots as scratch for nested results.
3. `rax` is scratch during argument setup even without nested calls.

The safest assumption is:

- nested `invoke`/`fastcall` is only safe for integer/pointer return values
- any earlier volatile input must be saved explicitly first

## `fastcall.frame`, `frame`, `end frame`, `endf`

`fastcall.frame` controls whether `fastcall` allocates stack space itself or only tracks how much space would be needed.

### Default Mode

At the top of the file:

```asm
fastcall.frame = -1
```

When it is negative, each `fastcall` does its own:

- `sub rsp,framesize`
- `call ...`
- `add rsp,framesize`

### Tracking Mode

When `fastcall.frame >= 0`, a `fastcall` no longer allocates or frees its own call frame. Instead it updates `fastcall.frame` to the maximum frame size needed so far.

This is how the OpenGL example precomputes one shared frame size for a block of code:

```asm
fastcall.frame = 0
...
MAIN_FRAME := fastcall.frame
fastcall.frame = -1
```

Then the code manually reserves `MAIN_FRAME` once.

### `frame`

`frame ... end frame` is a lexical helper around that tracking mode.

```asm
frame
    invoke Foo, ...
    invoke Bar, ...
end frame
```

It:

- switches `fastcall.frame` to tracking mode for the block
- emits one `sub rsp,size` before the block
- lets nested `fastcall` / `invoke` reuse that space
- emits `add rsp,size` at `end frame`

`endf` is just a short alias for `end frame`.

### Important Separation from `proc` Epilogues

`frame` is not integrated with `ret`.

Inside a `proc`, bare `ret` expands to the current epilogue hook. It does not know that you opened a `frame` block.

That means this source:

```asm
proc FrameProc
    frame
        fastcall TargetA,1,2,3,4,5
        ret
    endf
endp
```

can assemble into:

```asm
push rbp
mov rbp,rsp
sub rsp,30h
...
call TargetA
leave
retn
add rsp,30h       ; dead code
```

The `leave` balanced the stack at runtime, but the `endf` still emitted its `add rsp,...` after the `ret`.

So:

- you still need `endf` in the source to restore assembler state
- but if you want a clean runtime flow, structure the code so execution reaches `endf` before `ret`

A safer pattern is:

```asm
proc Better
    frame
        ...
    endf
    ret
endp
```

or with one exit label:

```asm
proc Better
    frame
        ...
        jmp .done
    .done:
    endf
    ret
endp
```

### Raw `call` Is Invisible to Tracking

Only `fastcall` / `invoke` participate in `fastcall.frame` tracking.

If you write a raw:

```asm
call SomeProc
```

inside a tracked block or a static-RSP procedure, you must reserve the required shadow space yourself.

## `fastcall.inline_string`

Default definition:

```asm
macro fastcall?.inline_string var
    local data,continue
    jmp continue
    if sizeof.TCHAR > 1
        align sizeof.TCHAR,90h
    end if
    match value, var
        data TCHAR value,0
    end match
    redefine var data
    continue:
end macro
```

Meaning:

- emit a jump over inline string data
- emit a zero-terminated `TCHAR` string
- redefine the original macro argument to the string label

On `win64a.inc`, `TCHAR` is ANSI bytes. On `win64w.inc`, `TCHAR` is UTF-16 words.

### Consequences

The default behavior means:

- string data is emitted at the call site
- identical strings are duplicated
- the code section now contains embedded data

That is fine for small code, but not always what you want.

### Customizing It

The shipped `examples\globstr\demo_windows.asm` shows the intended extension point:

```asm
macro fastcall?.inline_string var
    local data
    data GLOBSTR var,0
    redefine var data
end macro
```

This redirects string materialization into a global-string mechanism instead of inline code/data.

If you redefine this macro, make sure it still does one essential thing:

- it must leave `var` redefined to an address expression usable by the rest of `fastcall`

If your replacement emits bytes into the instruction stream, it also needs to handle control flow safely, like the default `jmp continue` version does.

## `proc`

The `proc` macro defines named procedures and creates helper macros inside the procedure body.

Basic forms:

```asm
proc Name
proc Name, p1,p2,p3
proc Name uses rbx rsi rdi, p1,p2
proc c Name, p1,p2
proc stdcall Name, p1,p2
```

### `proc c` and `proc stdcall` on Win64

The parser accepts `c` and `stdcall`, but the default 64-bit code generation does not meaningfully distinguish them.

- `cinvoke` is identical to `invoke`
- `proc c` and `proc stdcall` mainly pass different `flag` values to custom hook macros

There is no 32-bit-style name decoration here.

### Unused Procedures Vanish

A `proc` body is wrapped in:

```asm
if used name
    name:
    ...
end if
```

So an unreferenced procedure emits no code at all.

That is useful for dead-code removal, but it is surprising if you expected every `proc` to become a label unconditionally.

## Default Prologue / Epilogue

The defaults are:

```asm
push rbp
mov rbp,rsp
sub rsp,locals+padding
push used_regs...
```

and:

```asm
pop used_regs...
leave
retn
```

A few notes:

- locals are addressed from `rbp`
- parameter labels start at `rbp+16`
- saved `uses` registers are below the local area
- padding is inserted so that later calls remain aligned

### `uses`

`uses` simply expands to `push` / `pop`.

That means it is only appropriate for pushable general-purpose registers.

Do not expect it to handle:

- `xmm6`..`xmm15`
- `ymm` / `zmm`
- any custom save convention

If you need nonvolatile XMM preservation, do it manually or with a custom prologue.

## Parameter Labels: Home Slots, Not Register Variables

This is the single most important semantic point of `proc64.inc`.

For a default `proc`, parameter labels start at `rbp+16`:

- arg1 at `[rbp+10h]`
- arg2 at `[rbp+18h]`
- arg3 at `[rbp+20h]`
- arg4 at `[rbp+28h]`
- arg5 at `[rbp+30h]`

That matches the Win64 home-area / stack layout.

But the caller does not automatically write the first four register arguments into those home slots.

So inside:

```asm
proc WindowProc hwnd,wmsg,wparam,lparam
```

the symbols `hwnd`, `wmsg`, `wparam`, `lparam` are memory locations, not aliases for `rcx`, `rdx`, `r8`, `r9`.

If you want `[hwnd]` to hold the actual incoming `rcx`, you must spill it yourself:

```asm
proc WindowProc hwnd,wmsg,wparam,lparam
    frame
    mov [hwnd],rcx
    mov [wmsg],rdx
    mov [wparam],r8
    mov [lparam],r9
    ...
    endf
    ret
endp
```

The OpenGL example does exactly this for `hwnd`.

### Practical Rule

- use the raw registers directly if you only need the first four arguments briefly
- spill them to their home slots or to locals if you want stable named storage
- arguments 5 and later already live in memory

## Typed Parameters Are Packed, Not Slot-Spaced

This is one of the sharpest x64-specific traps in the file.

Typed parameters are laid out with:

```asm
label ?argname:type
rb type
```

That means the offset advances by the declared type size, not by an 8-byte ABI slot.

For example:

```asm
proc TypedParams a:dword,b:dword,c:qword
    mov eax,[a]
    mov edx,[b]
    mov rcx,[c]
    ret
endp
```

assembled to accesses at:

- `a` -> `[rbp+10h]`
- `b` -> `[rbp+14h]`
- `c` -> `[rbp+18h]`

That is tightly packed. It does not match Win64 argument-slot spacing.

So on x64:

- `proc p, a,b,c` is ABI-shaped
- `proc p, a:dword,b:dword,c:qword` is a packed overlay and usually not ABI-shaped

### Recommendation

For real Win64 call interfaces, prefer untyped parameters:

```asm
proc Foo, a,b,c
```

and then read them with explicit sizes:

```asm
mov eax,[a]
mov edx,[b]
mov rcx,[c]
```

Treat typed parameters as an advanced overlay tool, not as a safe C-style prototype system.

## `ret`

Inside a `proc`, `ret` is redefined.

### Bare `ret`

```asm
ret
```

expands to the current epilogue hook, usually:

```asm
leave
retn
```

### `ret imm`

```asm
ret 8
```

does not use the epilogue hook. It expands directly to:

```asm
retn 8
```

That means it bypasses:

- register restoration from `uses`
- custom epilogue logic
- static-RSP cleanup

On Win64, `retn imm` is almost always the wrong thing anyway, because callers do not expect callee stack cleanup.

Practical rule:

- in `proc64.inc`, use bare `ret`
- only use `ret imm` if you intentionally want raw machine semantics and are cleaning up everything yourself

## `locals`, `endl`, and `proclocal`

Inside a `proc`, the macro defines a local-layout DSL.

### `locals ... endl`

This opens a `virtual at localbase@proc+current` block. You can declare locals with ordinary data-like syntax:

```asm
proc Example
    locals
        counter dd ?
        value   dq ?
        pt      POINT
        buf     rb 64
    endl
    ...
    ret
endp
```

The labels become stack-relative addresses.

### Initializers Emit Runtime Stores

If you initialize locals inside the block, the macro emits runtime `mov` instructions after the prologue.

Example:

```asm
proc InitLocals
    locals
        a dq 0123456789ABCDEFh
        b dd 11223344h
        c db 55h
    endl
    ret
endp
```

assembled to code equivalent to:

```asm
sub rsp,10h
mov dword [rbp-10h],89ABCDEFh
mov dword [rbp-0Ch],01234567h
mov dword [rbp-08h],11223344h
mov byte  [rbp-04h],55h
```

Notice that the 64-bit constant was split into two 32-bit stores. That is intentional: x86-64 has no generic `mov qword [mem], imm64` encoding.

### `proclocal`

`proclocal` is a shorthand that expands to a `locals ... endl` block. It accepts several forms:

- `name:type`
- `name[count]:type`
- `name[count]`
- `name type`
- `name`

Examples:

```asm
proclocal temp:qword, flags:dword, buf[64]:byte, pair[2]
```

Notes:

- `name[count]` defaults to qword array storage
- structure types work
- bare names default to qword storage

## Prologue / Epilogue Hooks

Three symbols control procedure generation:

- `prologue@proc`
- `epilogue@proc`
- `close@proc`

By default:

- `prologue@proc equ prologuedef`
- `epilogue@proc equ epiloguedef`
- `close@proc` is empty

The hook macros receive:

```asm
procname,flag,parmbytes,localbytes,reglist
```

This is the intended extension mechanism for alternate stack layouts.

## `static_rsp_prologue`, `static_rsp_epilogue`, `static_rsp_close`

These three macros provide a useful alternate procedure style:

- save `uses` registers first
- reserve locals plus a tracked outgoing-call frame once
- keep `rsp` stable for all `fastcall` / `invoke` inside the procedure

This is especially handy when:

- the procedure makes many calls
- you want one outgoing frame reservation
- you prefer `rsp`-relative stability to repeated `sub/add rsp`

### How to Enable It

```asm
prologue@proc equ static_rsp_prologue
epilogue@proc equ static_rsp_epilogue
close@proc    equ static_rsp_close

proc Work uses rbx, a,b,c,d,e
    ...
    invoke Target,1,2,3,4,5
    ret
endp

restore prologue@proc,epilogue@proc,close@proc
```

### What It Does

`static_rsp_prologue`:

- pushes the `uses` registers
- computes aligned local storage
- reserves one outgoing frame sized from `fastcall.frame`
- sets:
  - `localbase@proc`
  - `regsbase@proc`
  - `parmbase@proc`
  - `framesize@proc`
- switches `fastcall.frame` into tracking mode

`static_rsp_close` then assigns the final tracked maximum back into the prologue's `frame` symbol, forcing later assembly passes to grow the prologue if needed.

In a test procedure with one `uses rbx` and one 5-argument `fastcall`, the result was:

```asm
push rbx
sub  rsp,30h
...
call Target
add  rsp,30h
pop  rbx
ret
```

There was no extra per-call `sub/add rsp` inside the body. The one prologue reservation covered the call.

### Caveats

- it still does not spill the first four incoming argument registers for you
- raw `call` instructions are still invisible to `fastcall.frame`
- unwind metadata is still not generated

## Advanced Alternate Usage: `fastcall -` with a Temporary `call` Macro

`fastcall` always ends with `call proc`.

That means you can temporarily redefine `call` and pass a dummy token to reuse only the argument marshalling.

`com64.inc` does exactly that:

```asm
macro call dummy
    mov rax,[rcx]
    call [rax+Interface.proc]
end macro

fastcall -,handle,args
purge call
```

This is a very nice pattern for:

- COM vtable calls
- custom dispatchers
- trampoline wrappers
- "set up Win64 arguments, but use a special final call sequence"

## `pcountcheck` and `pcountsuffix`

By default:

```asm
macro pcountcheck? proc*,args*
end macro

define pcountsuffix %
```

So plain `proc64.inc` does not enforce parameter counts.

However, `win64axp.inc` and `win64wxp.inc` redefine `pcountcheck` and load prototype-count tables for imported APIs. In that mode, `invoke` / `fastcall` can reject bad argument counts at assembly time.

Also, a `proc` defines `name% := parmbytes/8` when used.

This is useful, but remember the typed-parameter caveat:

- packed typed parameters can make `parmbytes/8` diverge from the conceptual parameter count

For ordinary untyped x64 procedures, it is fine.

## Problems and Footguns

### 1. `invoke` is indirect

`invoke Foo,...` means `fastcall [Foo],...`, not `fastcall Foo,...`.

Use `fastcall` for local direct calls.

### 2. The first four `proc` parameters are not automatically stored

`hwnd`, `arg1`, and so on are home-slot labels, not magical register aliases.

Spill `rcx`, `rdx`, `r8`, `r9` yourself if you want `[param]`.

### 3. Typed parameters are packed by type size

On x64 that usually does not match ABI slot spacing. Treat typed parameters as overlays, not prototypes.

### 4. Nested calls can clobber earlier volatile arguments

`fastcall Target,rcx,fastcall Helper` is dangerous unless `rcx` has already been saved somewhere stable.

### 5. Nested-call support assumes `rax` results

It is not a safe generic mechanism for FP-returning nested calls.

### 6. `ret imm` bypasses the epilogue hook

It skips `uses` restoration and custom epilogues. On Win64 it is almost always wrong.

### 7. `frame` and `ret` are not integrated

Forgetting `endf` breaks assembler state. Reaching bare `ret` before `endf` can leave dead `add rsp,...` code behind.

### 8. Raw `call` is not frame-tracked

Only `fastcall` / `invoke` update `fastcall.frame`.

### 9. `uses` is GPR-only

Nonvolatile XMM registers must be saved manually.

### 10. Inline strings go into the code stream by default

That can duplicate data and produce mixed code/data layout.

### 11. No Windows x64 unwind metadata

These macros emit code only. They do not emit `.pdata` / `.xdata` or SEH unwind records.

That matters for:

- structured exception unwinding
- stack walking
- some debugger/profiler behavior

### 12. No large-frame stack probing

The default and static prologues just do `sub rsp,...`. If you need a very large frame, probe it yourself in the Windows-approved way.

### 13. Unreferenced `proc`s are omitted

If a procedure is never "used", it emits no code at all.

### 14. Avoid dot labels in `proc` bodies

`proc` opens a `namespace name`, but fasmg dot labels still follow the most recent regular label. That makes `.loop` easy to misread and easy to break after later edits or nested blocks.

For future work, prefer explicit labels such as `foo_loop` / `foo_done` in procedure bodies. That keeps the naming stable and makes the label owner obvious at a glance.

## Recommended Practices

1. Use `fastcall` for direct local labels and `invoke` for imported/qword-pointer style labels.
2. Treat `proc` parameter names as stack locations, not register names.
3. Keep x64 parameters untyped unless you explicitly want a packed overlay.
4. Save any volatile input before using nested `invoke` / `fastcall` expressions.
5. Use bare `ret`, not `ret imm`.
6. Balance every `frame` with `end frame` / `endf`, even if the epilogue would restore `rsp` anyway.
7. Use `static_rsp_*` if the procedure makes many calls and you want a single reserved outgoing frame.
8. Redefine `fastcall.inline_string` if you want pooled strings instead of code-local literals.
9. Do not assume the macros handle varargs FP duplication, unwind metadata, or stack probing for you.

## Good Minimal Patterns

### Direct call

```asm
fastcall LocalRoutine,1,2,3
```

### Imported WinAPI call

```asm
invoke MessageBox,HWND_DESKTOP,addr text,addr caption,MB_OK
```

### Procedure with manual spills

```asm
proc WindowProc hwnd,wmsg,wparam,lparam
    frame
        mov [hwnd],rcx
        mov [wmsg],rdx
        mov [wparam],r8
        mov [lparam],r9
        ...
    endf
    ret
endp
```

### Static-RSP procedure

```asm
prologue@proc equ static_rsp_prologue
epilogue@proc equ static_rsp_epilogue
close@proc    equ static_rsp_close

proc Worker uses rbx, a,b,c,d,e
    mov [a],rcx
    invoke Target,1,2,3,4,5
    ret
endp

restore prologue@proc,epilogue@proc,close@proc
```

### COM-style marshalling reuse

```asm
macro call dummy
    mov rax,[rcx]
    call [rax+Interface.Method]
end macro

fastcall -,handle,arg1,arg2
purge call
```

## Final Assessment

`proc64.inc` is compact and powerful, but it is not a high-level ABI abstraction. It is closer to a carefully engineered macro toolkit with a Win64 bias.

Used in its intended style, it is elegant:

- direct local calls with `fastcall`
- imported calls with `invoke`
- manual spills of incoming register parameters
- optional frame preallocation
- custom prologue hooks when needed

Used as if it were a C prototype system, it becomes misleading:

- typed parameters do not mean ABI-safe parameters
- nested calls are only partially expression-safe
- `ret imm` is too raw
- no runtime metadata is generated

Read it as a macro toolkit, not as a full calling-convention compiler, and it makes much more sense.
