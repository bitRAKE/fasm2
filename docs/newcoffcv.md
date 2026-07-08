# CodeView debug support — scope and progression

Author: Rickey Bowers Jr. (bitRAKE). Co-developed with Claude (Anthropic).

[`newcoffcv.inc`](../include/format/newcoffcv.inc) gives NEWCOFF objects CodeView (C13) debug
information and x64 unwind data, built entirely with the backend's records
discipline: markers append staging records during the pass;
`codeview_synthesize` — called from the newcoffms POSTPONE after the machine
is decided and before the counts are taken — turns them into sections. A
debug section is just more records, so nothing here needed new machinery:
`.debug$S`, `.xdata` and `.pdata` are `SECTION_RECORD`s, their relocations
are `RELOC_RECORD`s with the existing semantic kinds, and COMDAT
associativity ties them to their code.

The proc64-specific debug wrapper lives separately in
[`newcoffproc64.inc`](../include/format/newcoffproc64.inc). That file owns
the NEWCOFF-only `proc` copy needed to harvest parameters and `locals`
declarations without changing the shared `macro/proc64.inc` used by other
formats.

## The markers

| Marker | Records |
| --- | --- |
| `cvline [line], [file]` | a source-line marker at the current position (defaults capture the invocation site — fasmg reports the *invocation* for `__LINE__`/`__FILE__` even inside a macro) |
| `cvproc name` | opens a procedure at the current position, before any prologue pushes |
| `cvframe size [, regs]` | placed right after the prologue: rsp allocation, prologue length, pushed nonvolatile registers in push order |
| `cvendp` | closes the procedure (length becomes known) |
| `cvlocal n [, n:type]` | rsp-relative locals/params of the open procedure (`S_REGREL32`): the offset is derived from the symbol itself, the display name is its own name; type is a CodeView index, default `CV_T_UQUAD`; named primitive aliases include `CV_T_INT4`, `CV_T_UINT4`, `CV_T_QUAD`, `CV_T_UQUAD`, and `CV_T_64PVOID` |
| `cvlabel name` | names the current position (`S_LABEL32`) |

The current implementation intentionally exposes only a curated subset of
CodeView simple type aliases. CodeView type indices below `0x1000` are
predefined "simple" types; the low byte is the simple kind and pointer
spellings OR in a mode byte. For example, `CV_T_64PVOID = 0x603` is
`NearPointer64 | Void`. This is the right substrate for today's `S_REGREL32`
locals, but it is not the whole type system. Future `.debug$T` work should
keep these predefined aliases separate from allocator-produced type records
starting at `0x1000`, then layer higher-level assembly concepts (`struct`
definitions, pointers to structs, arrays, handles, SDK aliases) on top.

With `NEWCOFF.DEBUG` defined non-zero, the unnamed-macro interceptor tags
**every line of the main source file** automatically:

```
macro ? &line&
	if __FILE__ = __SOURCE__
		cvline
	end if
	line
end macro
```

and the `static_rsp` wrappers make `proc`/`endp` mark themselves — frame
size *and* `uses` registers, taken straight from the prologue arguments —
while `newcoff_debug_procs` intercepts PROC to harvest the declaration's
parameter names into `S_REGREL32` records automatically (each name is
resolved through the proc symbol, since the declaration tokens carry the
pre-namespace context). The same installation also hooks proc64 `locals`
blocks: each declaration line is recorded after the virtual local label is
created, so `locals ... endl` entries become rsp-relative `S_REGREL32`
symbols without a manual `cvlocal`. This behavior is scoped to the
`newcoffproc64.inc` wrapper; the generic `proc64.inc` macros are unchanged.

```
prologue@proc	equ newcoff_debug_prologue
close@proc	equ newcoff_debug_close
newcoff_debug_procs
```

`newcoff_debug_prologue` reads `framebytes@proc` after expanding
`static_rsp_prologue`, so the recorded allocation is exactly what the frame
sub'd; push offsets are reconstructed from encoding sizes (`r8`–`r15` carry
a REX prefix). Nothing is annotated by hand in `examples/hexer`.

## What is emitted

- **One shared `.debug$S`** (always kept): `S_OBJNAME` + `S_COMPILE3`
  (producer identification), the F3 file-name string table and the F4 file
  checksum table — **SHA-256 (kind 3)** of each source's bytes, hashed at
  assembly time: `SHA256.calc file <name>` feeds the file straight into
  the hash (no copy), and `db SHA256.result` emits the 32-byte digest
  string ([`include/macro/sha256.inc`](../include/macro/sha256.inc);
  other hash functions can share the generator-in/string-out interface
  regardless of digest size). `NEWCOFF.NOCHECKSUM=1` skips the hashing
  (kind none, zeroed entries).
- **One `.debug$S` per CODE section with debug material** — line records
  from data sections are ignored — holding an F2 lines subsection and an F1
  symbols subsection (`S_GPROC32` + `S_FRAMEPROC` + `S_END` per procedure,
  scope pointers left 0 for the linker to rewrite; `S_LABEL32` per label).
  Bound to the code by SECREL32 + SECTION relocations; **COMDAT ASSOCIATIVE**
  to the code section when that is a COMDAT, so `/OPT:REF` discards code and
  debug data together (build hexer in `base` mode: the avx variants vanish
  along with their line tables — no dangling relocations).
- **`.xdata`/`.pdata` per code section with procedures** (AMD64 only):
  `UNWIND_INFO` synthesized from the cvframe facts — `UWOP_ALLOC_SMALL` /
  `UWOP_ALLOC_LARGE` for the rsp allocation, `UWOP_PUSH_NONVOL` per pushed
  register, codes in reverse prologue order; leaves get the valid empty
  info. `RUNTIME_FUNCTION` entries relocate through the existing `rva32`
  semantic kind. Same associativity rule as the debug sections.

Incidental but pleasing: the backend's fold-safe offset-0 redirection makes
the SECREL targets resolve to the function's *external* symbol, so tools
show a proper `LinkageName` without any extra work.

## Verified

`link /DEBUG:FULL` and `lld-link /DEBUG:FULL` both fold everything into a
PDB: `llvm-pdbutil dump -symbols` shows the module symbols above,
`dump -l` the line tables, `llvm-readobj --unwind` the exact prologue
reconstruction (e.g. hexer's `u8_as_hex_init`: `PUSH_NONVOL RBX` at +1,
`ALLOC_SMALL 32` at +5, prologue size 5).

## Debugger exploration cases

Build `tests\newcoff\hexer\_build.cmd dispatch`, open `hexer.exe` in the
debugger (x64dbg: make sure the PDB loads — check the symbol status in the
Modules view), run against any file, and check:

1. **Lines everywhere** — every instruction in the disassembly annotated
   with `file:line` (the per-line interceptor at work). Includes lines in
   `somehex.asm`/`u8_as_hex_avx512.asm`/`dispatch.asm`, each against its own
   file.
2. **Named frames** — break inside `u8_as_hex_base` (dispatch build on a
   machine that resolves to it, or `_build.cmd base`); the call stack should
   read `u8_as_hex_base` ← `mainCRTStartup` *by name*, courtesy of
   S_GPROC32 + working `.pdata`. Step out should return cleanly.
3. **Unwind through the frame proc** — break in `u8_as_hex_init` (first
   call only!) after the prologue; the stack walk must survive the pushed
   `rbx` + 32-byte allocation. Compare x64dbg's stack window with and
   without a `base` build to see heuristics vs real unwind data.
4. **The dispatcher label** — `u8_as_hex` (the `jmp [u8_as_hex_impl]` stub)
   should appear as a named symbol (S_LABEL32) in the symbol list /
   disassembly, distinct from the variant functions.
5. **Function list** — x64dbg's Symbols view (or WinDbg `x hexer!*`, or VS
   breakpoint-by-name) should list all four variants, `u8_as_hex_init`,
   `mainCRTStartup`.
6. **/OPT:REF interplay** — `_build.cmd base`, then confirm the debugger
   shows *only* `u8_as_hex_base` — no ghost symbols or lines from the
   discarded avx sections.
7. **VS or WinDbg source stepping** — with the source tree present, F10
   should step `hexer.asm` line by line, and VS should report the source
   as *matching* — the F4 entries carry the true SHA-256 of each file
   (`llvm-pdbutil dump -l hexer.pdb` shows the digests; verified equal to
   `Get-FileHash`).

## Progression

(The living forward log, with what each stage needs, is
[`newcoff-plan.md`](newcoff-plan.md); the table below tracks the stages.)

| Stage | Contents | State |
| --- | --- | --- |
| 1 | C13 lines (F2/F3/F4), per-section associative `.debug$S` | **done** |
| 2 | `S_GPROC32`/`S_FRAMEPROC`/`S_END`, `S_LABEL32`, `S_OBJNAME`/`S_COMPILE3`; `.pdata`/`.xdata` from the prologue facts | **done** |
| 3 | `S_REGREL32` locals/params: `cvlocal` marker + automatic PROC-parameter harvest + automatic proc64 `locals` harvest; primitive type indices, default `T_UQUAD` | **done** |
| 4 | `S_CONSTANT` for equates, `S_GDATA32`/`S_LDATA32` for data symbols incl. statics | planned |
| 5 | SHA-256 file checksums in F4 (`SHA256.calc` generator interface; `file` re-reads the source bytes in POSTPONE) | **done** |
| 6 | `.debug$T`: allocator-owned type records from `0x1000` upward (`LF_STRUCTURE`/`LF_ARRAY`/`LF_POINTER`/...) bridged from `macro/struct.inc` definitions and SDK aliases; `S_UDT`; typed data symbols | ambitious |
| 7 | `S_INLINESITE` modelling *macro expansions* as inline frames | speculative, uniquely fasm |

Known limits of the current stage: one open `cvproc` at a time (no nesting);
automatic `locals` harvest currently records proc64 declaration labels with
the default type index; unwind covers rsp-allocation + pushes (no
frame-pointer chaining, which `static_rsp` frames never need);
`S_GPROC32` uses `T_NOTYPE`. The 8-slot `reg_nibbles` field caps `uses` lists at 8
registers — matching the number of nonvolatile GPRs, so nothing real hits it.
