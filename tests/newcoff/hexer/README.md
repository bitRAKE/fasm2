# hexer — a walk-through of NEWCOFF, feature by feature

Author: Rickey Bowers Jr. (bitRAKE). Co-developed with Claude (Anthropic).

hexer is a small console program — it writes a file's bytes as hexadecimal —
built to show, one at a time, what `format MS64 NEWCOFF` gives you: COMDAT
sections, linker dead-code removal, identical-data folding, runtime CPU
dispatch, and source-level debugging in x64dbg / WinDbg / Visual Studio.
If you are new to NEWCOFF, build this and follow the sections below in order.

## 0. Build it

From a Visual Studio developer prompt (for `link.exe` and the SDK import
libraries; `lld-link` is picked up automatically if `link` is absent):

```cmd
tests\newcoff\hexer\_build.cmd            :: default: dispatch build
tests\newcoff\hexer\_build.cmd base       :: u8_as_hex := u8_as_hex_base
tests\newcoff\hexer\_build.cmd avx2
tests\newcoff\hexer\_build.cmd avx512
tests\newcoff\hexer\_build.cmd clean

hexer.exe hexer.exe                       :: hex-dump anything
```

Every build is a debug build: the script passes `-iNEWCOFF.DEBUG:=1` and the
linker gets `/DEBUG:FULL`, producing `hexer.pdb` alongside the exe.

## 1. The pieces

| File | Contents |
| --- | --- |
| `windows.inc` | the model shim: `format MS64 NEWCOFF`, `win64a.inc`, stable `static_rsp` proc frames — and, under `NEWCOFF.DEBUG`, the self-marking prologue wrappers |
| `u8_as_hex.inc` | the shared `hextab` lookup table in an EXACT_MATCH COMDAT |
| `somehex.asm` | `u8_as_hex_base` (scalar) and `u8_as_hex_avx2` — one COMDAT section each |
| `u8_as_hex_avx512.asm` | `u8_as_hex_avx512`, plus a *second copy* of `hextab` |
| `dispatch.asm` | CPUID/XGETBV detection + the self-routing `u8_as_hex` dispatcher |
| `hexer.asm` | the console front end — uses exactly **one** external symbol, `u8_as_hex` |

Every variant honours one prototype, which is what makes them
interchangeable:

```
u8_as_hex(rcx = dst, rdx = src, r8 = len)  ->  rax = dst + 2*len
```

Note what is *not* here: no harness include, no special setup. NEWCOFF is a
first-class format — `format MS64 NEWCOFF` is the only line that differs
from a classic COFF source. (It emits the modern *big object* container;
see [`../readme.md`](../readme.md) for what that means and which linkers
consume it.)

## 2. COMDAT sections and `/OPT:REF`

Each ISA variant lives in a COMDAT section named after it
(`.text$u8_as_hex_base`, …). A **single-ISA build** aliases the front end's
one import at assembly time —

```cmd
fasm2 -i"HEXER_ISA='avx2'" hexer.asm
```

— so only that variant is referenced, and `/OPT:REF` throws the others
away. See it in `hexer.map` after `_build.cmd base`:

```
.text$u8_as_hex_avx2    CODE   00000000H   <- discarded
.text$u8_as_hex_avx512  CODE   00000000H   <- discarded
.text$u8_as_hex_base    CODE   0000003bH   <- kept (referenced)
```

The **dispatch** build defines no `HEXER_ISA`; `dispatch.obj` references
every variant to choose among them at runtime, so all three stay.

## 3. EXACT_MATCH folding

`somehex.obj` and `u8_as_hex_avx512.obj` *both* define `hextab` — same
section name, same bytes, `comdat exactmatch`. The assembler stamps each
copy with a CRC-32 of its contents (the same parameters clang uses,
cross-validated in `../crc_vectors.asm`); the linker checks the checksums
match and keeps one copy. The map shows a single surviving `hextab`.
Perturb one copy by a byte and the link fails — that is the *exact-match*
guarantee, versus `any` (pick one, no questions) which `../optref_any_*`
demonstrates.

The subtle part: each object *references* its own copy, and the loser's
reference must rebind to the survivor. NEWCOFF relocates references
against the section's offset-0 external symbol precisely so this works.

## 4. Runtime dispatch

`dispatch.obj` needs no init call. `u8_as_hex` is a `jmp [u8_as_hex_impl]`,
and `u8_as_hex_impl` starts out pointing at the *detector*. The first call
runs CPUID + `XGETBV` (feature bits *and* XCR0 state bits), stores the
widest supported variant into `u8_as_hex_impl`, and tail-calls it with the
original arguments. Every later call jumps straight through.

## 5. Source-level debugging

This is where NEWCOFF earns the "new". With `NEWCOFF.DEBUG` set (the build
script always sets it):

- **every line** of each main source file is tagged automatically (an
  unnamed-macro interceptor calls `cvline` per line);
- **every `proc`/`endp`** marks itself via the `static_rsp` wrappers in
  `windows.inc` — procedure symbols *plus* frame size and `uses` registers,
  from which real x64 **unwind info** (`.pdata`/`.xdata`) is synthesized;
- the raw (non-proc) variants carry explicit `cvproc`/`cvendp` markers, and
  the dispatcher stub a `cvlabel`.

Things to try (details and more cases in [`../newcoffcv.md`](../newcoffcv.md)):

1. **x64dbg**: open `hexer.exe` (dispatch build), run against a file —
   every instruction shows its `file:line`. Break inside a variant: the
   call stack reads `u8_as_hex_*` ← `mainCRTStartup` *by name*, and
   step-out returns cleanly — real unwind data, not stack heuristics.
2. **Visual Studio**: open the exe as a project (File → Open → Project),
   set a breakpoint by function name (`u8_as_hex_init`), F5, and step
   through the assembly *as source*.
3. Break in `u8_as_hex_init` (first call only!) after the prologue and
   walk the stack through the pushed `rbx` + 32-byte allocation.
4. Rebuild with `_build.cmd base` and confirm the debugger sees *only*
   the base variant — the discarded COMDATs took their debug sections
   with them (they are COMDAT-associative to the code they describe).

Debug info costs nothing at runtime — it lives in the PDB — and a build
without `NEWCOFF.DEBUG` emits none at all.

## 6. Where to go next

- [`../readme.md`](../readme.md) — the NEWCOFF design: records-in-POSTPONE,
  canonical symbol order, semantic relocations, and why it is bigobj-only.
- [`../newcoffcv.md`](../newcoffcv.md) — the CodeView module: markers,
  what is emitted, the debugger exploration cases, and the progression
  plan (locals, data symbols, types).
- `../smoke.asm`, `../fold_*.asm`, `../weak_*.asm`, `../cv.asm` — each
  feature in isolation, exit-code-verified by `../_build.cmd`.
