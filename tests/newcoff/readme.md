# newcoff — re-engineering the MS COFF backend from the ground up

Author: Rickey Bowers Jr. (bitRAKE). Co-developed with Claude (Anthropic).

> **Modern-only, by design.** newcoff always writes the **big object
> format**: `ANON_OBJECT_HEADER_BIGOBJ` + `IMAGE_SYMBOL_EX` — 20-byte
> symbols, 32-bit section numbers, no 32767-section ceiling, no truncated
> COMDAT associations. There is **no classic-COFF emission path**.
> Consumers must understand bigobj: MSVC `link`, `lld-link`, LLVM tools,
> and GNU binutils ≥ 2.25 all do; older third-party linkers (GoLink,
> older Pelles `polink`, pre-2014 MinGW) do **not** — for those, the
> untouched legacy `format MS/MS64 COFF` remains the supported path.
> Leaning forward is the point: one emission path, sized for the next
> twenty years of features (debug sections, wide associations, large
> objects) rather than the last thirty of compatibility.

This directory is a development harness for a from-scratch rewrite of the
MS COFF backend, informed by the 2026 COMDAT work on
[`include/format/coffms.inc`](../../include/format/coffms.inc) (EXACT_MATCH
checksums, `/OPT:REF` support, fold-safe relocations — see
[`docs/coff_comdat_postmortem.md`](../../docs/coff_comdat_postmortem.md) and
[`docs/coff_comdat.md`](../../docs/coff_comdat.md)). The legacy backend stays
fully usable; the rewrite lives beside it and is selected per source file.

## The interception mechanism

fasmg macro definitions *stack*: a new `macro format?.MS64?` shadows the one
installed by `fasm2.inc`, and invoking the macro's own name inside its body
reaches the previous definition. [`newcoff.inc`](newcoff.inc) exploits this:

```
macro format?.MS64? variant
	match =NEWCOFF?, variant
		if ~ definite format
			format binary as 'obj'
		end if
		include 'newcoffms.inc'	; the rewrite
		use64
	else
		format.MS64 variant	; forward to the legacy handler
	end match
end macro
```

A source that does `include 'newcoff.inc'` before its FORMAT statement can
then choose `format MS NEWCOFF` / `format MS64 NEWCOFF` (the rewrite) or
`format MS COFF` / `format MS64 COFF` (forwarded, byte-for-byte legacy
behaviour). One tree, both backends, A/B comparison for free — this is how a
format can be re-engineered *in place* without destabilizing anything that
ships.

**The machine is not a format variant.** The two intercepts differ only in
the initial USE mode; the backend reads `x86.mode` in POSTPONE (`use32` →
i386, `use64` → AMD64) and configures the object accordingly. No
`Settings` plumbing, no per-machine includes.

## Why re-engineer

The legacy backend builds the object file *incrementally*: symbol table
entries are stored the moment `public`/`extrn`/`section` execute, section
headers are patched at section close, relocations capture their final symbol
index at emit time. Every COMDAT feature added in 2026 had to fight that
architecture, and each fight left a scar:

- **Four copies of the relocation tail.** `dword?`/`qword?` for i386 and
  AMD64 each carry an identical `add_relocation:` block; the fold-safe
  redirection fix had to be applied in four places (and once, an editor
  reverting one of them cost a debugging session).
- **Sidecar registries.** Because the symbol table is written eagerly,
  later-needed facts had to be squirreled away in bolted-on virtuals:
  `section_ref_sym` (offset-0 externals), `comdat_symbols` (deferred
  synthetic statics), `xm_meta` + `exactmatch_list` (checksum descriptors).
  Four parallel data structures, each invented under pressure.
- **Ordering rules enforced by errors.** The linker requires a COMDAT
  section's symbol to be the first symbol carrying its section number
  (MSVC LNK1143). With eager emission the only defense is detecting the
  violation and erroring: *"PUBLIC symbol for a COMDAT section must be
  declared after its SECTION statement."* The constraint is real; making the
  user schedule declarations around it is not.
- **Latent dead corners.** The legacy AMD64 `qword?` references
  `IMAGE_REL_AMD64_ADDR64NB` — a relocation type that does not exist in the
  PE/COFF specification (and is not defined in the file, so `dq RVA x`
  dies on an undefined symbol). Incremental patching accumulates such
  corners; a rewrite retires them.

## The design rules

[`newcoffms.inc`](newcoffms.inc) is built on what the retrofits taught:

**1. Records, not stores.** During the pass, each entity appends one
fixed-size record to a staging virtual — `section_records` (64 bytes),
`public_records` (32), `extern_records` (16), `reloc_records` (16). Nothing
emits file structures mid-pass. POSTPONE reads the records and lays out the
entire object once: relocations, symbol table, string table, then the section
headers back-patched into the block reserved at file start. There is exactly
one source of truth and one emission site; the four sidecar registries
collapse into fields of the section record.

**2. Canonical symbol order by construction.** POSTPONE emits the symbol
table as: every section's static symbol + section-definition aux record,
then publics (declaration order), then synthesized COMDAT statics, then
externs. Indices are arithmetic — section *i* is `2*i`, public *p* is
`2*NSEC + p`, extern *e* is `2*NSEC + NPUB + e`. LNK1143 is satisfied
structurally: the smoke tests declare their `public` statements *before*
the sections they name, which the legacy backend must reject. Every section
gets an aux record (as MSVC and clang emit), so checksums and associations
have a uniform home.

**3. Symbolic relocation targets — in both axes.** A relocation record
stores *what* it targets (kind 0: a section index, kind 1: an extern
ordinal — decoded from element metadata: sections carry scale `1+index`,
externs scale `-1` with the ordinal as constant term) and *what it means*
(a semantic kind: `abs32`, `rva32`, `rel32`, `abs64`, `rva64` — the
classifiers never see an `IMAGE_REL_*` number). POSTPONE resolves both in
one place: target → symbol index, applying the fold-safe rule (a section
target resolves to the section's offset-0 external when one exists, so
references into folded COMDATs rebind to the survivor); kind + machine →
relocation type (i386 applies its 32-bit types to the low dword of 64-bit
fields, exactly as legacy did; `rva64` on AMD64 is the one impossible
combination and errs). One CALM engine (`NEWCOFF.record`) does all
recording; `dword?`/`qword?` only classify the addressing form and `call`
it. The machine-specific surface of the whole backend is one small mapping
table in POSTPONE.

Two supporting idioms carried over from the retrofit work:

- **CALM first** for everything on the per-byte hot path (the relocation
  engine); the finalization loops run once and stay in the classic macro
  language for legibility.
- **Read back from the output area in place.** The EXACT_MATCH CRC-32 is
  computed in POSTPONE from the section's own ORG space through a remembered
  anchor + base element — no staged copy of the bytes (reflected CRC-32,
  polynomial `0xEDB88320`, seed 0, *no* final inversion, zero-padded
  uninitialized tail included).

One pass-discipline lesson surfaced while testing: the staging virtuals are
sized by forward-referenced constants, so loads from them are unstable in
early passes. Narrow emissions (`dw`, `db`) of loaded values must be masked
(`and 0xFFFF` / `and 0xFF`) or fasmg aborts on a transient out-of-range value
before it can converge. Bulk `load`/`db byte_sequence` dumps — the legacy
approach — dodge this silently, which is worth knowing when choosing between
per-field and bulk emission.

## What's implemented

| Area | State |
| --- | --- |
| big object container (`ANON_OBJECT_HEADER_BIGOBJ`, `IMAGE_SYMBOL_EX`) | always — the only emission path |
| machine selection | automatic from `x86.mode` (use32 → i386, use64 → AMD64) |
| sections, attributes, `align` | done (mirrors legacy syntax) |
| COMDAT selections | `noduplicates` `any` `samesize` `largest` `exactmatch` `associative` + `newest` (accepted; deprecated by reproducible builds) |
| EXACT_MATCH aux CheckSum | done, matches legacy/clang bit-for-bit |
| `public` (external / `static` / `as` / absolute) | done |
| `extrn` (`as`, `:size`) | done |
| relocations | semantic kinds abs32/rva32/rel32/abs64/rva64, mapped per machine in POSTPONE; fold-safe offset-0 redirection |
| synthetic static for public-less NODUPLICATES COMDAT | done (appended in POSTPONE) |
| weak externals (`public` of an extern value) | **not yet** — errs; needs a per-public aux count in the layout arithmetic |
| >65535 relocations/section (`NRELOC_OVFL`) | **not yet** — errs |
| CodeView debug (`.debug$S`/`$T`), SECREL/SECTION relocs | roadmap — the records model was shaped for this: synthetic sections and their relocations can be appended in POSTPONE, where sizes and symbol indices already exist |

## Testing

```
tests\newcoff\_build.cmd        (VS dev prompt, or LLVM on PATH)
```

- **smoke.asm** — 64-bit: every relocation kind, an uninitialized section,
  an EXACT_MATCH COMDAT, out-of-order `public` declarations. Assembled
  through *both* backends (`-i"SMOKE_LEGACY=1"` selects the forwarded
  legacy path), linked with `/OPT:REF`, must exit 97.
- **smoke32.asm** — the same backend producing an i386 object purely by
  `use32`; DIR32/DIR32NB paths, a 64-bit field relocated on its low dword.
  Links with no import libraries at all (return from entry exits with
  `eax`), so it runs from any prompt. Both backends, must exit 97.
- **fold_a.asm / fold_b.asm** — two objects each defining an identical
  `.rdata$tab` EXACT_MATCH COMDAT and reading it through their own
  reference; after the fold both must observe the survivor. Must exit 97.

Verified against `lld-link` and MSVC `link` (14.44), inspected with
`llvm-readobj`: identical EXACT_MATCH checksums from both backends, external
relocations for the folded table, section-symbol relocations elsewhere.
