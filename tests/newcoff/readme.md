# NEWCOFF test suite

Author: Rickey Bowers Jr. (bitRAKE). Co-developed with Claude (Anthropic).

One source per feature, exit-code-verified. Design documentation lives in
[`docs/newcoffms.md`](../../docs/newcoffms.md); the CodeView module in
[`docs/newcoffcv.md`](../../docs/newcoffcv.md); the guided example in
[`examples/hexer`](../../examples/hexer/README.md).

## Running

```
tests\newcoff\_build.cmd        (VS dev prompt, or LLVM on PATH)
```

(Aside for harness-style setups: fasmg does not search includes relative
to the *including* file — only the working directory and the INCLUDE
environment variable. With NEWCOFF in `include/format` this no longer
matters here.)

- **smoke.asm** — 64-bit: every relocation kind, an uninitialized section,
  an EXACT_MATCH COMDAT, out-of-order `public` declarations. Assembled
  through *both* backends (`-i"SMOKE_LEGACY=1"` selects the upstream
  legacy backend, COMDAT-free and 8-char section names), linked with
  `/OPT:REF`, must exit 97.
- **smoke32.asm** — the same backend producing an i386 object purely by
  `use32`; DIR32/DIR32NB paths, a 64-bit field relocated on its low dword.
  Links with no import libraries at all (return from entry exits with
  `eax`), so it runs from any prompt. Both backends, must exit 97.
- **fold_a.asm / fold_b.asm** — two objects each defining an identical
  `.rdata$tab` EXACT_MATCH COMDAT and reading it through their own
  reference; after the fold both must observe the survivor. Must exit 97.
- **weak_a/b/c.asm** — `weak_b` publishes `maybe_get` as a weak external
  whose alias tag is `weak_a`'s `real_get`; `weak_c` calls through the
  weak name. Must exit 97.
- **cv.asm** — `cvline` markers through `/DEBUG:FULL` to a PDB, plus
  automatic `S_REGREL32` harvest for PROC parameters and `locals`
  declarations. Also demonstrates an unreferenced COMDAT function dragging
  its associative `.debug$S` out of the image. Must exit 97.
- **ovfl.asm** — 65600 relocations in one section; overflow encoding
  accepted by both linkers. Must exit 97.
- **crc_vectors.asm** — byte-for-byte reproductions of clang-cl COMDAT
  sections; the EXACT_MATCH checksums must equal the values clang wrote
  (verified with `llvm-readobj` when available). Runs 42.
- **optref_any_a/b.asm** — duplicate `comdat any` definitions across two
  objects; the linker picks one. Runs 42.

Verified against `lld-link` and MSVC `link` (14.44), inspected with
`llvm-readobj`: identical EXACT_MATCH checksums from both backends, external
relocations for the folded table, section-symbol relocations elsewhere.
