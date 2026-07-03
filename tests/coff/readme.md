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
- A non-associative COMDAT section with **no `public` at all** gets a
  synthetic static COMDAT symbol (named after the section) appended to the
  symbol table, keeping the object valid. Static symbols have no
  cross-object identity, so such a section is simply individually
  discardable by `/OPT:REF` (`optref_nosym.asm`). Selections other than the
  default `noduplicates` exist to match sections *across* objects by
  external symbol name, so they still require a `public`
  (`fail_any_no_symbol.asm`).
- `exactmatch` is rejected explicitly (`fail_exactmatch.asm`): it needs the
  aux-record `CheckSum` — a CRC-32 of the final section contents, which
  would have to be computed in `postpone` after all fixups. Left 0, MSVC
  would treat every same-named section as "matching". Use `samesize` or
  `any` instead.

## Rules the assembler now enforces

These come from the PE/COFF spec and are hard errors in MSVC `link`
(`LNK1143: invalid or corrupt file`) if violated, so `coffms.inc` rejects
them at assembly time:

- **No `public` into a COMDAT section may precede that `section`
  statement.** The section symbol must be the first symbol carrying its
  section number; MSVC rejects the object even when another proper `public`
  follows the section (`fail_public_first.asm`, `fail_public_early.asm`).
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
| `optref_nosym.asm` | No `public` at all: the synthesized static COMDAT symbol keeps the object valid; unreferenced section still discarded. |
| `optref_any_a/b.asm` | `comdat any`: duplicate definitions across objects deduplicate instead of erroring. |
| `optref_bss.asm` | Uninitialized (BSS-style) COMDAT section. |
| `optref_empty.asm` | Zero-length COMDAT section (and no spurious `LNK4078` attribute-mismatch warning). |
| `optref_pinned.asm` | Counter-example: a **non**-COMDAT `.pdata` referencing a COMDAT function pins it — `/OPT:REF` cannot discard it. This is why associative COMDAT exists. |
| `linkfail_dup_a/b.asm` | Default `NODUPLICATES`: same COMDAT in two objects must fail to link (LNK2005). |

## Gotchas observed while testing

- **Do not write `mov eax,[rip+sym]` for a relocatable `sym`.** Explicit
  `rip` is *manual* RIP-relative addressing (fasm 1 manual: `mov [rip+3],sil
  ; manual RIP-relative addressing`): the expression after `rip+` is the
  literal displacement, so the effective address is `next_instruction + sym`
  — fasm 1.73 produces the identical encoding. In MS COFF output that
  literal relocatable displacement becomes an `ADDR32` relocation on a
  RIP-relative encoding, which links without complaint and reads the wrong
  address at run time. To address `sym` itself, write `mov eax,[sym]`
  (automatic RIP-relative in long mode), or force the encoding with the
  `{rip}` decorator / `use rip` — never by naming `rip` in the address.
- `lld-link` is stricter than MSVC in some places (x86 objects need
  `@feat.00` or `/SAFESEH:NO`; x86 entry symbols must carry their `_`
  decoration) and looser in others (it tolerated COMDAT sections with no
  COMDAT symbol, which MSVC rejects with LNK1143).
- MSVC `link /VERBOSE:REF` prints `Discarded <symbol>` lines — handy to
  verify what `/OPT:REF` actually removed.
