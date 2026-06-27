# Coding Policy

This project is a fasm2 Win64 GUI codebase. Prefer small, local changes that
match the existing assembly style, keep application state explicit, and make
Win32 contracts visible at the call site.

## Build Validation

- Build from a Visual Studio developer environment.
- Use `_build.cmd` as the normal executable validation path.
- Treat assembler success as necessary but not sufficient for UI, persistence,
  or message-loop changes; run a focused smoke test when behavior changes.

## Calling Convention

- Use `fastcall` for local `proc` labels.
- Use `invoke` for imported APIs and function pointers.
- Proc parameter labels are home-slot addresses, not live register aliases.
- Use incoming ABI registers directly when the value is already where the code
  needs it and no intervening call can clobber it.
- Spill `proc` arguments at entry only when they are needed after calls or when
  the spill makes later code materially clearer.
- Preserve only nonvolatile registers whose values are needed after the code
  that mutates them. Keep `uses` lists narrow.
- Prefer parameter ordering that aligns with Win64 ABI register positions:
  `rcx`, `rdx`, `r8`, `r9`. Put the values that are already live in those
  registers in the matching argument positions when the API or helper contract
  is still being designed.
- Use `addr local` for out-parameters instead of preloading volatile argument
  registers and passing those registers positionally.
- The project uses the RSP-based `proc64.inc` frame macros, so `rbp` is
  available as a normal nonvolatile register. Add `rbp` to the `uses` list
  before using it.

## Abstraction Boundaries

- Do not add a `proc`, macro, or helper that only renames a single API call or
  replaces one obvious line with another single line.
- An abstraction must remove real duplication, hide a fragile calling
  convention, centralize a tested algorithm, own state or lifetime, or provide
  a clearer contract than the raw call site.
- Prefer direct Win32 calls for one-off setup and obvious operations,
  especially when the helper body would be only `invoke SomeApi,...` plus
  `ret`.
- If a helper exists only to make a call look uniform, delete it and keep the
  call site explicit.

## Windows API Alignment

Prefer the Windows API surface when it already provides the operation or
structure contract the code needs. This keeps examples aligned with PSDK names,
reduces custom helper vocabulary, and makes call sites easier to compare with C
or SDK documentation.

For example, use the imported RECT helpers such as `CopyRect`, `OffsetRect`,
`InflateRect`, `IntersectRect`, `UnionRect`, and `SetRect` instead of creating a
local `proc` that copies or edits `RECT` fields by hand. A hand-written RECT
copy, including a shorter SIMD copy, is justified only when it is a measured or
clearly documented performance decision. In that case, leave a short comment at
the implementation or call site explaining why the API call is intentionally not
used.

## Reusable Modules

Most small assembly examples can be one source file. Split a program into
includes only when the pieces are reusable semantic units, not just because the
file is long.

Good module boundaries are large puzzle pieces a reader can learn, carry to
another program, and refine over time:

- a local executable policy layer such as `windows.inc`;
- state modules that own their data and BSS;
- feature modules that expose command-sized procedures;
- renderer modules that own GDI objects or other lifetimes;
- UI routing modules that connect Windows messages to feature procedures.

Keep entry sources thin when a program is modular. The entry file should show
the include order, initialize shared policy, create the first window, and run
the message loop. It should not hide feature ownership by manually emitting
feature data or directly coordinating unrelated subsystems.

Module-level global storage registers itself through `__GLOBAL_DATA__` and
`__GLOBAL_BSS__`, then is emitted by the central data/BSS section points. Do not
add new feature-specific section emitters or direct feature data macro calls in
the entry source.

Feature emitters must own their alignment. Do not assume the previous emitter
ended on a useful boundary; start BSS emitters with `align 8`, and add local
`align` directives before qword tables or structures that follow byte/word
buffers or 32-bit fields.

## Macro Iteration

Use `iterate` when repetition is structural and a reader is likely to refine
the repeated list as a unit. This improves readability and code density for
table-like setup/teardown code.

Good fits:

- creating and destroying related GDI objects;
- resource, font, color, brush, or control tables;
- repeated validation or copy patterns with the same shape;
- small dispatch tables whose entries differ only by data.

Avoid forcing `iterate` into control-flow-heavy logic where explicit branches
are easier to inspect. The goal is readability and maintainability, not
micro-optimization.

## Resource And UI Identity

Resource and control IDs shared by assembly and the Windows resource compiler
belong in one common ID file, typically a small C-preprocessor-style header such
as `resource.h`.

Keep shared ID files austere:

- only `#define NAME value` lines;
- no policy prose;
- no assembler-only expressions;
- no duplicate definitions in `.rc` or `.inc` files;
- use numeric constants accepted by both fasm2 and `rc.exe`.

Resource-backed examples should keep the resource script responsible for UI
identity: menus, accelerator tables, dialogs, manifests, icons, and version
metadata. Assembly code should load and route those resources, not duplicate
their IDs or labels.

When an app has accelerators and modeless dialogs, the message loop should give
accelerators first chance, then offer remaining messages to modeless dialogs,
then run the normal translate/dispatch path:

```asm
invoke  GetMessageW,addr msg,0,0,0
test    eax,eax
jle     .shutdown
invoke  TranslateAcceleratorW,[hMain],[hAccel],addr msg
test    eax,eax
jnz     .message_loop
invoke  IsDialogMessage,[hFindDlg],addr msg
test    eax,eax
jnz     .message_loop
invoke  TranslateMessage,addr msg
invoke  DispatchMessageW,addr msg
```

Use the explicit `W` API when the generic alias machinery conflicts with an
imported neutral name.

## Presentation Contracts

For UI-heavy examples, separate simulation/rendering from overlay presentation
when that makes the message contract easier to learn.

The GDI snake example uses this contract:

- `WM_ERASEBKGND` stretch-blits the current board bitmap;
- `WM_PAINT` draws overlay text only;
- `WM_TIMER` advances simulation and invalidates;
- `WM_KEYDOWN` mutates input/game state;
- `WM_SIZE` changes preview difficulty only while the board is unlocked.

If a stretch-blitted board is grid-based, snap the pre-game window size to cell
increments. Otherwise destination pixels are distributed unevenly and cells
render with irregular widths or heights.

When UI is visually minimal but behaviorally complex, document the hidden state
machine and message map. The visible surface may only be a board and overlay,
but the code still needs clear ownership of simulation state, renderer state,
dialog state, and persistence state.

## Persistence

Persisted records should own identity and comparison keys. UI labels may be
derived from persisted blobs but should not become hidden identity unless that
is the documented policy.

For user-visible arcade or game state, validate stored data before accepting it.
If the table fails validation, treat it as missing and reset to defaults. Simple
tamper-evident storage is acceptable for examples: pack the record, obfuscate
with a rolling key, and store a signature over the decoded data. Do not claim
cryptographic security unless the implementation actually provides it.

## String Handling

The project includes `win64wx.inc`, so the default inline string materializer
matches wide-character `TCHAR` strings. `windows.inc` provides an A/W-dispatching
`fastcall.inline_string`:

- plain literals use the active `TCHAR` flavor;
- `<A,'text'>` forces ANSI;
- `<W,'text'>` forces wide characters;
- `GLOBSTR.reuse` and `GLOBWSTR.reuse` deduplicate repeated inline literals.

For APIs that have generic A/W aliases, prefer the generic API name with an
undecorated literal:

```asm
invoke  LoadLibrary,'user32.dll'
invoke  MessageBox,[hMain],'Saved.','example',MB_OK
```

Use decorated API names only when the call genuinely requires that specific
entry point or when the argument is not a string literal and the code is already
explicitly wide or ANSI:

```asm
invoke  GetSaveFileNameW,addr ofn
invoke  SendMessageW,[hRichEd],EM_STREAMIN,[stream_flags],addr estr
```

Keep a named data-section string only when a stable address is required outside
the argument materializer. Valid cases include structure initializers, registry
subkeys, `OPENFILENAME` filters, pointer tables, mutable buffers, and
runtime-built text storage.

## Documentation

- Keep audit notes additive while issues are being worked down.
- When a policy or convention becomes explicit through cleanup, record it here
  instead of leaving it implicit in scattered call sites.
- Example READMEs should explain why a shape exists, not only list files.
  Especially for modular examples, document what each module owns and why that
  boundary is reusable.
