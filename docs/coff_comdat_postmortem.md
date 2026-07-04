# MS COFF COMDAT and `/OPT:REF` — implementation notes

This is a post-mortem for the COMDAT support in
[`include/format/coffms.inc`](../include/format/coffms.inc) (the `MS COFF` and
`MS64 COFF` output formats) and its test suite in
[`tests/coff/`](../tests/coff). It records the linker behaviour that shaped the
design and the fasmg techniques that made it possible, so the next person does
not have to rediscover them empirically. Everything here was verified against
LLVM 22 (`lld-link`, `llvm-readobj`, `clang-cl`) and MSVC `link.exe` (VS 18).

## What the feature does

`/OPT:REF` lets the linker drop code and data that nothing references. It can
only do that at the granularity of a **COMDAT section**, so to benefit from it
each independently-removable function or datum must live in its own COMDAT.
The format exposes this through section attributes:

```
section '.text$name' code readable executable comdat
section '.text$name' code readable executable comdat any
section '.pdata$name' data readable comdat associative some_label
section '.rdata$name' data readable comdat exactmatch
```

- `comdat` sets `IMAGE_SCN_LNK_COMDAT` with the default `NODUPLICATES`
  selection.
- A selection keyword overrides it: `any`, `samesize`, `largest`,
  `noduplicates`, `exactmatch`.
- `associative <label>` ties the section's fate to the section containing
  `<label>` (used for `.pdata`/`.xdata` unwind metadata that must be discarded
  together with its function). The target must be defined earlier.

## Linker behaviour that shaped the design

These are the non-obvious rules; each is enforced or worked around in the
format, and each has a test.

1. **The section symbol must be the first symbol carrying its section number.**
   A COMDAT section needs a static symbol (with a section-definition aux
   record) as the anchor for its selection. If any other symbol carrying that
   section number precedes it — e.g. a `public` written *before* the `section`
   statement — MSVC `link` rejects the whole object with
   `LNK1143: invalid or corrupt file: no symbol for COMDAT section`. `lld-link`
   is more lenient (it silently discards the section, then errors on
   relocations into it). The format tracks a `PUBLICS_SEEN` bitmask and errors
   at the `section` statement rather than emit a corrupt object.

2. **A COMDAT with no `public` still needs a symbol.** If a COMDAT section
   exports nothing, the format synthesises a static symbol named after the
   section (appended to the symbol table in `postpone`). Static symbols have no
   cross-object identity, so this only makes the section individually
   `/OPT:REF`-discardable — which is exactly right for `NODUPLICATES`. The
   other selections match sections *across* objects by their external symbol
   name, so they still require a real `public`.

3. **Long section names have a 7-digit budget.** A name longer than 8 bytes is
   stored in the header as `/nnnnnnn`, an offset into the string table. The
   field holds at most 7 decimal digits, so the referenced string must start
   below offset 10,000,000. The format raises a clear error past that instead
   of letting fasmg emit an out-of-range value.

4. **Zero-length COMDATs must not be flagged uninitialised.** Marking an empty
   section `CNT_UNINITIALIZED_DATA` triggers MSVC `LNK4078` (multiple sections
   with different attributes) when it merges by name.

5. **Non-COMDAT metadata pins its target.** A plain `.pdata` entry that
   references a COMDAT function is a hard reference and defeats `/OPT:REF`;
   `optref_pinned.asm` demonstrates the failure that `associative` exists to
   fix.

## The EXACT_MATCH checksum

`exactmatch` folds two same-named COMDATs only when their contents are
identical, compared via the section-definition aux record's `CheckSum` field.
The parameters are **not** the familiar PNG/zlib CRC-32:

- reflected polynomial `0xEDB88320` (same as PNG),
- **seed `0`** (PNG seeds `0xFFFFFFFF`),
- **no final inversion** (PNG XORs the result with `0xFFFFFFFF`).

Two facts pinned this down from a `clang-cl` object: an empty COMDAT has
`CheckSum = 0` (rules out the `0xFFFFFFFF` seed, which would fold to
`0xFFFFFFFF` over zero bytes), and a six-byte `mov eax,42 / ret` section folds
to `0x7F8535B7` (matches seed 0, no inversion). The implementation reuses a
16-entry nibble table (two lookups per byte) rather than a 256-entry table.

**Cross-validation.** Because the checksum is a pure function of the raw section
bytes, it can be checked against LLVM's own CRC directly:
[`tests/coff/crc_vectors.asm`](../tests/coff/crc_vectors.asm) reproduces five
`clang-cl` COMDAT sections byte for byte (sizes 6, 7, 9, 13, 20 — deliberately
including odd counts to exercise the nibble folding) and carries clang's
`CheckSum` for each as a comment. fasm2 produces an identical value on every
one. Generation details do not interfere: identical bytes give identical
checksums.

## Reading the output area back in POSTPONE

The checksum has to be computed over the section's final bytes, which only
exist after the section is emitted. The idiomatic place is `postpone`, the same
stage where the format loads the relocation and symbol tables back out of their
`virtual` blocks and emits them. But section content is not in a `virtual` — it
is in the real output — and that turns out to require a specific set of fasmg
techniques.

**The output is a set of address spaces, not one flat buffer.** Each `section`
directive starts a fresh addressing space, and each COMDAT section is `org`'d to
a relocatable `element`. So there is no single anchor that spans all sections;
you cannot read section *N* by a file offset from the file start.

**To read a section's bytes in `postpone` you need two things:**

- a **space handle** — an inline `::` label placed inside the section's `org`
  space (`xm_area::`). A bare element or a plain label is *not* an accessible
  addressing area; only a `::` anchor is. It can sit anywhere in the section
  (start or end); what matters is that it belongs to that space.
- the **base address** — the section's `org` element (`SECTION_BASE`, i.e. the
  `element sym` the section was `org`'d to). The anchor's own numeric value is
  *not* the org base, so the two are distinct and both are needed:
  `load byte:1 from xm_area:(xm_base + i)`.

**Remembering an `element` across to POSTPONE.** The org base is an abstract
polynomial (`relocatable * (1+index) + symbol_index`); it must be captured so it
can be named later. The load-bearing subtlety:

- `X = sym` collapses it to a number and loses the address-space binding.
- `define LIST xm_area, sym, index` stores the symbolic references and resolves
  them in `postpone` with their bindings intact — for **both** the `::` anchor
  (as a usable space) and the base (as an address). This is what the format
  uses.
- `LIST =: sym` also remembers a base algebraically and accumulates an
  `irpv`-able list, and it evaluates immediately (so it can capture a shared
  mutable like `SECTION_BASE` without the delayed-evaluation trap below). But
  `=:` does **not** preserve a `::` anchor as an accessible space — it collapses
  the anchor to `.`. Since the space handle is the hard requirement here,
  `define` is the right tool, and one `define` record carries the anchor, the
  base, and the section index together.

**The delayed-evaluation trap.** `define` resolves its stored tokens in
`postpone`, not when written. If a record stores a *shared* variable
(`SECTION_BASE`, overwritten each section), every record resolves to the final
section's value. The fix is to store the section's **unique `local`** element
(`sym`), whose identity persists even though its name has gone out of scope.

**Capturing a number immediately.** Conversely, the per-section lengths must be
captured *now*, not deferred. `repeat 1, si:SECTION_INDEX` substitutes the value
textually, so `define exactmatch_list xm_area, sym, si` records the literal
section number rather than a reference that would drift.

**Iterating.** `irpv cell, exactmatch_list` walks the records (and iterates
nothing when the list was never defined, so no separate count guard is needed);
`match a =, b =, c, cell` splits each. Numbers that were captured immediately —
initialized length, tail length, aux index — live in the `xm_meta` virtual,
indexed by section number.

Putting it together, per exactmatch section: `section_start` drops the `::`
anchor and `define`s `(anchor, base, section#)`; the finalizer records
`(init_len, tail_len, aux_index)` in `xm_meta`; `postpone` walks the list and
folds the CRC reading `xm_area:(xm_base + i)` over the initialized bytes, then
folds the zero-padded tail, and stores the result in the aux `CheckSum`.

## An unrelated gotcha worth repeating

Do not write `mov eax,[rip+sym]` for a relocatable `sym`. Explicit `rip` is
*manual* RIP-relative addressing (fasm 1 manual: `mov [rip+3],sil`): the
expression after `rip+` is the literal displacement, so the effective address
becomes `next_instruction + sym`. fasm 1.73 produces the identical encoding, so
this is not a fasm2 bug — but in COFF output the relocatable displacement
becomes an `ADDR32` relocation on a RIP-relative encoding, which links cleanly
and reads the wrong address at run time. Use `mov eax,[sym]` (automatic
RIP-relative in long mode), or `{rip}` / `use rip` to force the encoding.

## Test map

`tests/coff/_build.cmd` assembles every case and, for the positive ones, links
with `/OPT:REF` under whichever of `lld-link` / MSVC `link` it finds; each
positive test exits `42`, negative tests must be rejected by the assembler or
the linker. See [`tests/coff/readme.md`](../tests/coff/readme.md) for the
per-file table.
