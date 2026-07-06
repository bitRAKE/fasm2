# NEWCOFF — plan

Author: Rickey Bowers Jr. (bitRAKE). Co-developed with Claude (Anthropic).

What NEWCOFF does today is documented in [`newcoffms.md`](newcoffms.md)
(the backend) and [`newcoffcv.md`](newcoffcv.md) (debug support); the
suite in [`tests/newcoff`](../tests/newcoff/readme.md) verifies it, and
[`examples/hexer`](../examples/hexer/README.md) walks a new user through
it. This document is the forward log: what is next, what it needs, and
what order makes sense.

## Done since this plan was written

- **SHA-256 file checksums in F4** (kind 3). The hash library became
  [`include/macro/sha256.inc`](../include/macro/sha256.inc) with the
  area-indexed interface (`AreaSha256 msg` publishes digest bytes as
  `msg.0`..`msg.31` — the freqdump.g idiom, hash-size agnostic);
  [`scripts/sha256.inc`](../scripts/sha256.inc) asserts it against the
  NESSIE vectors. POSTPONE re-reads each registered source with `file`
  and hashes it; digests verified equal to `Get-FileHash`. Cost measured:
  the full 4-object hexer debug build is ~2 s; `NEWCOFF.NOCHECKSUM=1`
  opts out (kind none, zeroed). Two fasmg context lessons were paid for
  en route — recorded under the guardrails below.

## Next — unblocked

### 2. Stage 3 CodeView: named locals and parameters (`S_REGREL32`)

The payoff: watch windows show `dst`, `src`, `len` by name. With
`static_rsp` frames every local's rsp offset is an assembly-time
constant, so the `newcoff_debug_prologue` wrapper can emit these
mechanically from the proc macro's locals/params — no user annotation.
Primitive type indices below 0x1000 need no `.debug$T`: `T_INT4` 0x74,
`T_UINT4` 0x75, `T_QUAD` 0x76, `T_UQUAD` 0x77, 64-bit pointer forms in
the 0x06xx range. Plan: a `cvlocal name, rspofs [, type]` marker first,
then wire the proc machinery to call it.

### 3. Data and constant symbols

- `S_CONSTANT` (0x1107) — equates by name in the debugger
  (`CHUNK = 0x10000` visible in watch). Opt-in `cvconst` marker;
  sweeping *all* equates would be noise.
- `S_GDATA32` / `S_LDATA32` (0x110D/0x110C) — data symbols with SECREL
  binding and primitive types. Publics already reach the PDB through the
  linker; the gain is *statics* and typed display. A `cvdata` marker,
  possibly folded into `public`/label wrappers later.

## Later — needs a subsystem

### 4. `.debug$T`: real types from `macro/struct.inc`

The seductive one: `struct` definitions already know every field name,
offset and size — a bridge can emit `LF_FIELDLIST`/`LF_STRUCTURE`
(+ `LF_ARRAY`, `LF_POINTER`, `LF_MODIFIER`) from the same source of
truth, then `S_UDT` and typed data symbols make structures *expand* in
the debugger. Needs a type-index allocator (0x1000+) with dedup, and
record padding discipline (leaf records 4-aligned, `LF_PAD` bytes).
Design before code: this is its own module (`newcofftp.inc`?) with the
same records-in-POSTPONE shape.

### 5. Unwind beyond static frames

Current codes cover rsp allocation + nonvolatile pushes — everything
`static_rsp` frames produce. If other frame styles arrive:
`UWOP_SET_FPREG` (frame-pointer chaining), `UWOP_SAVE_NONVOL`,
`UWOP_SAVE_XMM128`, chained RUNTIME_FUNCTION entries for hot/cold
splits. Also `S_BLOCK32` for lexical scopes once `cvproc` allows
nesting (single-open today).

## Speculative — worth dreaming about

- **`S_INLINESITE` for macro expansions**: model macro invocations as
  inline frames, so stepping shows the macro name as a pseudo-frame.
  Debuggers built this for C++ inlining; fasm macro expansion is exactly
  isomorphic. Requires the `_ID` symbol route and `LF_FUNC_ID` in
  `.debug$T`, plus binary-annotation encoding — build after (4).
- **ARM64 machine**: the semantic-relocation-kind design means a new
  machine is one mapping table plus classifiers; the element algebra is
  ISA-neutral. Waiting on a use case.

## Standing guardrails

Lessons already paid for — respect them in all of the above:

1. fasmg name resolution is multi-pass *global*: struct field names must
   not collide with any macro anywhere in the assembly (`frame`, `file`,
   `section`...); wire-format structs stay `, packed`.
2. Struct-instance initializer values resolve in the *instance*
   namespace: always fully qualify (`NEWCOFF.X`); positional initializers
   keep CALM `asm` append lines free of field-name tokens.
3. Records are append-once; anything that looks like mutation becomes a
   POSTPONE scan. Counts derive from `($ - $$) / sizeof RECORD`, never
   from variables.
4. Qualified paths are not anchors: `SHA256.K0` binds to
   `CALLER.SHA256.K0` the moment anything creates a `SHA256` member in
   the caller's namespace — and your own writes (`SHA256.h1 = ...`) do
   exactly that. Library macros that must run from any namespace hang
   state off a macro-LOCAL symbol (locals carry their own context) and
   keep globals to flat single tokens, which fall back correctly.
5. Never open `namespace` on a macro-local label: its parent chain
   excludes the global scope, so even directives (`repeat`, `iterate`)
   stop resolving inside.
