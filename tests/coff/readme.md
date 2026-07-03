# MS COFF `/OPT:REF` tests

Small objects that exercise the COMDAT and long-section-name support in
`include/format/coffms.inc` (`format MS COFF` / `format MS64 COFF`) against
both `lld-link` (LLVM) and MSVC `link.exe`. Each positive test links with
`/OPT:REF`, runs, and must exit with code `42` — the value only survives if
the linker kept exactly the sections it was supposed to keep.

## Running

Start from a Visual Studio developer prompt (for `link.exe`); `lld-link` is
picked up from `PATH` or `C:\Program Files\LLVM\bin`. Either linker alone is
enough — the runner tests whichever it finds.

```cmd
tests\coff\_build.cmd
tests\coff\_build.cmd clean
```

## Section syntax

```
section '.text$name' code readable executable comdat
section '.text$name' code readable executable comdat any
section '.pdata$name' data readable comdat associative some_label
```

- `comdat` marks the section `IMAGE_SCN_LNK_COMDAT` with selection
  `NODUPLICATES`: a second definition anywhere in the link is an error.
- A selection keyword after `comdat` overrides that: `any`, `samesize`,
  `largest`, `noduplicates`.
- `comdat associative <label>` ties the section's fate to the section that
  contains `<label>`: when the linker discards that section, this one goes
  with it. This is how unwind/exception metadata (`.pdata$x`, `.xdata$x`)
  follows its function (see `optref_assoc.asm`). The associated section must
  be defined **earlier** in the source.
- `exactmatch` is deliberately not offered: MSVC compares COMDAT contents by
  the aux-record `CheckSum`, which this format leaves 0, so two different
  sections would silently "match". Use `samesize` or `any` instead.

## Rules the assembler now enforces

These come from the PE/COFF spec and are hard errors in MSVC `link`
(`LNK1143: invalid or corrupt file`) if violated, so `coffms.inc` rejects
them at assembly time:

- **Every non-associative COMDAT section needs a symbol declared after its
  `section` statement.** Use `public name`, or `public static name` when
  nothing should be exported (see `optref_static.asm`). A `public` placed
  *before* the `section` statement lands in front of the section symbol in
  the symbol table and corrupts the COMDAT record (`fail_public_first.asm`,
  `fail_no_symbol.asm`).
- **Long section names have a string-table budget.** A section name longer
  than 8 bytes is stored as `/offset` in the header; the offset field caps
  at 7 decimal digits, so the referenced string must start below offset
  10,000,000 (`fail_bigstrtab.asm`).

## Positive tests

| Test | Shows |
| --- | --- |
| `optref64.asm` | Baseline: `/OPT:REF` discards unreferenced COMDAT code+data, keeps referenced ones, long section names resolve. |
| `optref32.asm` | Same in 32-bit `format MS COFF`; also emits `@feat.00` (SafeSEH marker, required by `lld-link` for x86) and a decorated `_mainCRTStartup`. |
| `optref_assoc.asm` | `comdat associative`: `.pdata`/`.xdata` COMDATs are discarded together with their function. |
| `optref_static.asm` | `public static` provides the COMDAT symbol without exporting; unreferenced section still discarded. |
| `optref_any_a/b.asm` | `comdat any`: duplicate definitions across objects deduplicate instead of erroring. |
| `optref_bss.asm` | Uninitialized (BSS-style) COMDAT section. |
| `optref_empty.asm` | Zero-length COMDAT section (and no spurious `LNK4078` attribute-mismatch warning). |
| `optref_pinned.asm` | Counter-example: a **non**-COMDAT `.pdata` referencing a COMDAT function pins it — `/OPT:REF` cannot discard it. This is why associative COMDAT exists. |
| `linkfail_dup_a/b.asm` | Default `NODUPLICATES`: same COMDAT in two objects must fail to link (LNK2005). |

## Gotchas observed while testing

- **Do not write `mov eax,[rip+sym]` for a relocatable `sym`.** Explicit
  `rip` addressing emits the raw displacement with an `ADDR32` relocation on
  a RIP-relative encoding — it links without complaint and reads the wrong
  address at run time. Plain `mov eax,[sym]` selects RIP-relative encoding
  *and* the matching `REL32` relocation automatically in long mode.
- `lld-link` is stricter than MSVC in some places (x86 objects need
  `@feat.00` or `/SAFESEH:NO`; x86 entry symbols must carry their `_`
  decoration) and looser in others (it tolerated COMDAT sections with no
  COMDAT symbol, which MSVC rejects with LNK1143).
- MSVC `link /VERBOSE:REF` prints `Discarded <symbol>` lines — handy to
  verify what `/OPT:REF` actually removed.
