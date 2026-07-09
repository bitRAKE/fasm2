# NEWCOFF — plan

Author: Rickey Bowers Jr. (bitRAKE). Co-developed with Claude (Anthropic).

What NEWCOFF does today is documented in [`newcoffms.md`](newcoffms.md)
(the backend) and [`newcoffcv.md`](newcoffcv.md) (debug support); the
suite in [`tests/newcoff`](../tests/newcoff/readme.md) verifies it, and
[`examples/hexer`](../examples/hexer/README.md) walks a new user through
it. This document is the forward log: what is next, what it needs, and
what order makes sense.

## Done since this plan was written

- **SHA-256 file checksums in F4** (kind 3). The hash library is
  [`include/macro/sha256.inc`](../include/macro/sha256.inc):
  `SHA256.calc <byte generator>` consumes any generator statement
  (`file X`, `db ...`, a macro) directly — nothing is copied — and
  leaves the digest in `SHA256.result` as a 32-character string, so
  other hash functions can share the interface regardless of digest
  size. [`scripts/sha256.inc`](../scripts/sha256.inc) asserts it against
  the NESSIE vectors. The F4 emitter re-reads each registered source
  with `file` and hashes it; digests verified equal to `Get-FileHash`.
  Cost: the full 4-object hexer debug build is ~1.7 s;
  `NEWCOFF.NOCHECKSUM=1` opts out (kind none, zeroed). The namespace
  lessons paid for en route are guardrails 4-6 below.

- **Stage 3, `S_REGREL32` named locals/params**: `cvlocal` marker (offset
  derived from the symbol — it must be rsp-relative; per-item `name:type`
  with primitive CodeView indices, default `T_UQUAD`), and
  `newcoff_debug_procs` intercepting PROC to harvest declaration
  parameter names automatically (resolved through the proc symbol, since
  declaration tokens carry pre-namespace context). It also installs
  a NEWCOFF-owned proc64 wrapper
  ([`include/format/newcoffproc64.inc`](../include/format/newcoffproc64.inc)),
  so labels declared in `locals ... endl` blocks are recorded automatically
  after the virtual local declaration creates the symbol, without modifying
  shared `macro/proc64.inc`. Verified in the PDB: PROC params and LOCALS
  declarations appear as `S_REGREL32`, `register = RSP`, with correct
  offsets. The original eager per-line interceptor also tagged ENDP/SECTION
  at offset == code size. The current `NEWCOFF.DEBUG > 5` tracker defers each
  line until it proves byte emission and coalesces equal offsets; the F2
  bounds checks remain as defense for explicit `cvline` markers.

- **Stage 4, data and constant symbols**: opt-in `cvconst` emits
  `S_CONSTANT` for integer equates; `cvdata`/`cvldata` emit `S_LDATA32`,
  and `cvgdata` emits `S_GDATA32` for data at the current position.
  Data symbols use SECREL32 + SECTION relocations and primitive CodeView
  type indices (default `T_UQUAD`, `CV_T_UINT4` in the test). Verified in
  `tests/newcoff/cv.asm`: `llvm-pdbutil dump -globals` shows
  `CV_EXIT_CODE` and `cv_global_exit_code`, while `dump -symbols` shows
  the module-local `cv_local_delta`. A sizing bug caught by `link`
  (`debugging information corrupt`) is now guarded by the PDB checks:
  data records include their null terminator in the padded record length.

## Next — needs a subsystem

### 5. `.debug$T`: real types from `macro/struct.inc`

The seductive one: `struct` definitions already know every field name,
offset and size — a bridge can emit `LF_FIELDLIST`/`LF_STRUCTURE`
(+ `LF_ARRAY`, `LF_POINTER`, `LF_MODIFIER`) from the same source of
truth, then `S_UDT` and typed data symbols make structures *expand* in
the debugger. Needs a type-index allocator (0x1000+) with dedup, and
record padding discipline (leaf records 4-aligned, `LF_PAD` bytes).
Design before code: this is its own module (`newcofftp.inc`?) with the
same records-in-POSTPONE shape.

Keep the two type spaces distinct in the design:

- predefined CodeView simple types are indices below `0x1000`; the low
  byte is the simple kind and pointer forms OR in a mode byte
  (`0x603` = 64-bit near pointer to `void`);
- `.debug$T` records allocate user/type-builder indices from `0x1000`
  upward.

That split lets stage-3 locals use named aliases such as `CV_T_UINT4` and
`CV_T_64PVOID` today, while future abstractions can map assembly concepts
(`STARTUPINFO`, `PROCESS_INFORMATION`, `HANDLE`, pointer-to-struct, array
fields, SDK typedef aliases) either to a predefined simple type or to a
deduplicated `.debug$T` record.

### 6. Unwind beyond static frames

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
4. **The anchor rule**: a namespace whose members are accessed with
   dotted names from other namespaces MUST have a searchable anchor —
   the idiom is a self-referential define, `define SHA256 SHA256`
   (exactly how the x86 package anchors `x86`/`SSE`/`AVX`; an area
   label like `NEWCOFF::` works too). An anchor auto-created by a bare
   NAMESPACE directive is invisible to identifier lookup, so
   `SHA256.calc` from inside another namespace becomes
   `CALLER.SHA256.calc` and fails — invocation, reads and writes alike.
   With a searchable anchor, all three resolve to the global namespace
   from anywhere. NAMESPACE by itself is symbol sugar, not scoping —
   until the anchor is made searchable.
5. Never open `namespace` on a macro-local label: its parent chain
   excludes the global scope, so even directives (`repeat`, `iterate`)
   stop resolving inside.
6. Corollary via struct.inc: anonymous struct instances attach to the
   *current parent label* — if a macro just defined a macro-local label
   (like a hash's scratch area), the next instance is born under a
   local and rule 5 bites. Re-anchor with a plain label first
   (`cv_reanchor:` in the F4 emitter).
