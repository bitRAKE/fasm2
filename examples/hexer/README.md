# hexer — a COMDAT / `/OPT:REF` demonstration

Author: Rickey Bowers Jr. (bitRAKE). Co-developed with Claude (Anthropic).

A small console program that writes a file's bytes as hexadecimal. Its real
purpose is to exercise the MS64 COFF COMDAT features of `format MS64 COFF`:

- **`exactmatch` COMDAT** — two library objects each define the *same* `hextab`
  lookup table; the linker folds the identical copies into one.
- **`/OPT:REF` dead-section removal** — every ISA variant lives in its own
  COMDAT, so a build that resolves `u8_as_hex` to one variant discards the rest.
- **runtime dispatch** — a separate object detects the CPU and re-routes a
  stable entry point to the widest supported variant on first use.

It is a Win64 example: build from a Visual Studio developer prompt (for
`link.exe` and the SDK import libraries). `lld-link` is used automatically if
`link` is not found.

## Files

| File | Object | Contents |
| --- | --- | --- |
| `windows.inc` | — | thin shim: `format MS64 COFF`, `win64a.inc`, stable proc frames |
| `u8_as_hex.inc` | — | the shared `exactmatch` `hextab`, included by both libraries |
| `somehex.asm` | `somehex.obj` | `u8_as_hex_base`, `u8_as_hex_avx2` — one COMDAT each |
| `u8_as_hex_avx512.asm` | `u8_as_hex_avx512.obj` | `u8_as_hex_avx512`, and a second copy of `hextab` |
| `dispatch.asm` | `dispatch.obj` | CPUID detection + the `u8_as_hex` self-routing dispatcher |
| `hexer.asm` | `hexer.obj` | the console front end |

The library objects (`somehex`, `u8_as_hex_avx512`) include only
`u8_as_hex.inc`: `format MS64 COFF` is all they need — it even sets the output
`.obj` extension, so `fasm2 somehex.asm` names the object for you.

The front end uses **one** external symbol, `u8_as_hex`. Every variant honours
the same prototype, so they are interchangeable:

```
u8_as_hex(rcx = dst, rdx = src, r8 = len)  ->  rax = dst + 2*len
```

`_base` is a scalar nibble lookup; `_avx2`/`_avx512` do 32/64 bytes per pass with
`vpshufb` against the broadcast `hextab` and a lane-corrected interleave, with a
scalar tail. All three produce identical output (verified byte-for-byte).

## Building

```cmd
examples\hexer\_build.cmd            :: default: dispatch build
examples\hexer\_build.cmd base       :: u8_as_hex := u8_as_hex_base
examples\hexer\_build.cmd avx2
examples\hexer\_build.cmd avx512
examples\hexer\_build.cmd dispatch
examples\hexer\_build.cmd clean
```

Both libraries are always assembled and linked, so the `hextab` fold always
happens. A **single-ISA** build passes a string on the command line —

```cmd
fasm2 -i"HEXER_ISA='avx2'" hexer.asm
```

— and `hexer.asm` aliases its lone `u8_as_hex` import to `u8_as_hex_avx2`; no
`dispatch.obj` is linked, so the other variants are unreferenced. The
**dispatch** build defines no `HEXER_ISA`, imports the real `u8_as_hex` from
`dispatch.obj`, and that object references every variant to choose among them.

The image uses a dynamic base (the default — no `/FIXED`), so it is relocatable
and ASLR-compatible. Run it on any file:

```cmd
hexer.exe hexer.exe
```

## The self-routing dispatcher

`dispatch.obj` needs no separate init call. `u8_as_hex` is a `jmp
[u8_as_hex_impl]`, and `u8_as_hex_impl` starts pointing at the detector. The
first call lands in the detector, which runs CPUID + `XGETBV` (checking both the
feature bits and the XCR0 state bits), stores the chosen variant into
`u8_as_hex_impl`, and tail-calls it with the original arguments. Every later
call jumps straight to the selected variant.

## Seeing the COMDAT behaviour

Each build writes `hexer.map`. Two things to look for:

**`/OPT:REF` discards unreferenced variants.** In a single-ISA build the unused
variants' sections drop to zero length. From `_build.cmd base`:

```
.text$u8_as_hex_avx2    CODE   00000000H   <- discarded
.text$u8_as_hex_avx512  CODE   00000000H   <- discarded
.text$u8_as_hex_base    CODE   0000003bH   <- kept (referenced)
```

The `dispatch` build keeps all three, because `dispatch.obj` references them
all.

**`exactmatch` folds the duplicate table.** `somehex.obj` and
`u8_as_hex_avx512.obj` both define `.rdata$hextab` with identical bytes and
equal CRC-32 checksums, so the map lists a single surviving `hextab`. Perturb
one copy and the link fails with a duplicate-symbol error — that is what
`exactmatch` protects against, versus `any` (pick one) or `noduplicates`
(never allowed twice).

## A note on the shared table

For the fold to be *usable*, a reference into a folded COMDAT must survive the
fold. Each library references its own `hextab`; when the linker discards one
copy, that object's reference has to rebind to the survivor. That works because
the MS COFF backend relocates a reference against the section's external symbol
(`hextab`), not the static section symbol — the same thing `clang-cl` does for
`__declspec(selectany)` data. See
[`docs/coff_comdat_postmortem.md`](../../docs/coff_comdat_postmortem.md) and the
`tests/coff/optref_xref_*` regression test.
