# Instruction-set extensions: current state and plan

This note records how `include/iset/` is built today, what is wrong with its
feature gating, which x86 extensions are missing, and a concrete plan for
each addition. It covers everything from the file name to the gate.

Two rules guide every addition.

**One scheme: every file is included and `use` is the switch.** `x86-2.inc`
includes every `include/iset` file, vendor-specific ones as well (`3dnow.inc`
is AMD-only and is included). Nothing reaches the user through a separate
include or a separate kind of setting. Availability is decided only by the
CPUID-derived `use` settings.

The second rule: **every addition must reject the uses it does not
support.** That means a missing feature bit, the wrong CPU mode, the wrong
operand forms, and every other mnemonic it might shadow. An addition that
only proves the valid forms assemble is not finished.

All encodings quoted below were checked against `llvm-mc` 22.1.4
(`llvm-mc -triple=x86_64 --show-encoding`), except where marked *APM* (AMD
manual) or *verify*. CPUID bits were checked against XED's `cpuid.xed.txt`
tables (§6.2).

---

## 1. How the layer works today

### 1.1 File roles and include order

`x86-2.inc` is the core encoder. It pulls in the `iset` files at three
points. In fasmg, a symbol defined exactly once (constant, macro, CALM
instruction, CALM helper reached by `call`) is global: it can be used
before its definition, and fasmg resolves it in a further pass. So include
order is not about visibility. It matters for:
- **symbols defined more than once**: each use sees the definition in
  effect at that point, and a use before the first definition fails;
- **assembly time**: each forward reference can cost an extra pass, so
  helpers are still placed before their users;
- **deliberate stacks**: a mnemonic extended by a later file (`movd`,
  `pextrw`, `cmpsd`, ...) must come after the definition it falls back to.

The trap is the opposite of a visibility error: **defining an existing
name a second time silently changes it for everything that follows**,
including user code after the includes (G12, G13, §2.4). See §1.3 rule 5.

| Point in `x86-2.inc`   | Files                                      | Available helpers |
|------------------------|--------------------------------------------|-------------------|
| after the core ALU/system set | `fpu.inc`                           | `x86.parse_operand@*`, `x86.store_instruction@*`, `x86.store_operand_prefix` |
| after SGX/system block | `vmx.inc`, `mpx.inc`, `cet.inc`, `uintr.inc`, `svm.inc` | the same (GPR/memory only) |
| end of file            | `mmx` → `sse` → `sse2` → `sse3` → `sse4` → `avx` → `bmi` → `sha`, `aes`, `kl`, `gfni`, `pclmulqdq` → `3dnow` → `amx` | `MMX.*`, `SSE.*`, `AVX.*`, `AVX_512.*`, register elements `xmm16+`, `ymm*`, `zmm*`, `k*` |
| very last              | `apx.inc`                                  | all of the above; later APX phases stack layers over earlier mnemonics here ([`apx-plan.md`](apx-plan.md)) |

Placement rule (for pass count and readability, not visibility):
- A file that touches XMM/YMM/ZMM/k registers, or that uses a VEX, EVEX or
  XOP prefix (including VEX-encoded GPR instructions such as BMI), goes
  after `avx.inc`.
- A file that only touches GPRs and memory belongs in the
  `vmx`/`mpx`/`cet`/`uintr` group.

### 1.2 Styles: CALM versus macro

| Layer | Style |
|-------|-------|
| `x86-2.inc` core mnemonics, `fpu.inc`, `mmx.inc`, `3dnow.inc` | `calminstruction` |
| Shared parsers and encoders: `x86.parse_operand@ctx`, `x86.store_instruction@ctx`, `SSE.parse_operand@ctx`, `AVX_512.parse_*_operand@ctx`, `AVX_512.store_instruction@ctx` | `calminstruction` |
| Shape helpers: `SSE.basic_instruction`, `AVX_512.basic_instruction_bcst`, … | `macro` |
| Nearly every mnemonic in `avx`, `sse*`, `bmi`, `aes`, `sha`, `gfni`, `kl`, `cet`, `vmx`, `amx`, … | `macro` |

Policy from now on: **new mnemonics are written as `calminstruction`.** They
call the CALM parsers and the CALM `store_instruction` directly. They do not
call the macro shape helpers, because a CALM body would have to `assemble`
text to reach a macro, which defeats the point.

[`xcalm.inc`](../xcalm.inc) supplies `init`, `initsym`, `asm` and `xcall`.
`xcall` passes literal arguments: `(expr)` for a number, `<text>` for
symbolic text.

### 1.3 CALM rules these files depend on

These are easy to get wrong. Each one was confirmed while writing this note.

1. **A label must not share a name with a CALM command.**
   `store:`, `emit:`, `check:`, `call:`, `exit:`, `jump:`, `match:`,
   `compute:`, `arrange:`, `assemble:`, `publish:`, `transform:`, `take:`,
   `err:`, `local:` all fail. For example, `store:` produces
   `Error: missing argument.` on the label line. Use names like `encode`,
   `invalid`, `done`, `supported`.
2. **The signature is `x86.store_instruction@ctx opcode*, reg*, imm_size:0, immediate`.**
   Multi-byte opcodes go in one `<…>` argument, as in
   `xcall x86.store_instruction@dest, <0Fh,0C7h>,(1)`. It always emits a
   ModRM byte built from the parsed operand. Do not call it for
   fixed-ModRM instructions such as `0F 01 D8`; write the bytes with `emit`.
3. **Handle optional operands with `match , operand` / `jyes …` before
   calling the parser.** The parser errors on empty text.
4. **Inside `iterate`, parameters are substituted when the
   `calminstruction` is defined**, so `emit 1, modrm` bakes the constant in.
5. **A second definition stacks; it does not just replace.** Defined once,
   a name is global and works everywhere, even before its definition.
   Defined again, it becomes a stack: each use gets the definition in
   effect at that point, and a use before the first one fails. Inside a
   stacked definition, the mnemonic's own name means the *earlier*
   definition (`purge` pops back to it). That is the extension technique:
   a later layer handles its new forms and, for anything else, says "not
   for me, try the earlier one" by calling its own name. It works between
   macros and CALM instructions in either direction (CALM reaches it with
   `arrange`/`assemble`). In a macro, "pass it on" is an `if`/`else`
   branch: `exit` exists only in CALM and is an illegal instruction in a
   macro.

   The trap is a later layer that does **not** pass on: it silently takes
   over every form for all code that follows. That is how `xtest` (G3),
   `vpmuldq` (G12), `vcvtph2ps`/`vcvtps2ph` (G13) and the MMX/SSE forms of
   `movd`/`movq`/`pinsrw`/`pextrw`/`pmovmskb` (G14) lost forms or gained
   the wrong gate. A layer must gate only its own forms. Constants are no
   different: a user source that assigns `VEX_GPR` or `XOP_08_W0` changes
   every instruction assembled after it. `tests/iset/_dupnames.py` (§3.3)
   lists every reviewed stack and fails on any new name defined twice,
   unless every later definition names itself in its body (a layer that
   passes on, as the APX layers in `apx.inc` do).
6. **`iterate` substitutes parameters inside dotted names too.** In
   `iterate <prefix,opcode>`, the global `x86.apx.prefix` becomes
   `x86.apx.lock`, `x86.apx.rep`, ... and the code silently reads and writes
   the wrong variable. Give globals used inside an `iterate` names that no
   parameter can match (`x86.apx.legacy_prefix`).
7. **A layer that re-enters its own mnemonic through a helper needs a
   guard.** Only inside the layer does the name mean the earlier
   definition; a helper (`APX.promote`) that assembles `add …` reaches the
   layer again. The APX layers pass straight down while `x86.apx.evex` is
   set.
8. **A file's namespace root needs `define NS NS` to be usable in CALM.**
   `x86-2.inc` starts with `define x86 x86`, `define AVX AVX`…, `mpx.inc`
   with `define MPX MPX`. Without it, `APX.NF` inside a CALM body (or an
   `xcall` argument) is "undefined or out of scope" although `APX.NF := 1`
   is defined just above.

---

## 2. Support gating

### 2.1 Mechanism

The `macro ?` table at the top of `x86-2.inc` turns each entry
`NAME: item op value[, …]` into three CALM procedures:
`x86.use.NAME`, `x86.require.NAME` and `x86.requireexact.NAME`.

| Operator | Meaning | `require` | `requireexact` |
|----------|---------|-----------|----------------|
| `=`  | sets a level (`mode`, `cpu`, `fpu`, `simd`, …) | `item >= value` | `item = value` |
| `\|` | sets bit `value` in `item` (in practice `ext`) | bit set | bit set |
| `+`  | ORs in a whole mask (presets)                  | all bits set | all bits set |

An item name that has not been seen before is created automatically and
starts at 0. A new *kind* of setting needs nothing beyond its table line.
Add one only when the hardware itself reports a level rather than a bit
(`avx10`, §4), not to build a parallel opt-in scheme.

How code calls it:
- Macros: `require NAME` checks exactly; `require NAME+` checks "this level
  or higher".
- CALM: `call x86.require.NAME`.
- On failure the message is `NAME required` or `NAME or higher required`.

`{settings} instr` (the `??` hook) applies `use settings` to one line only.

Users who need one specific encoding already have `{vex2}`, `{vex3}`,
`{evex}` and `{rex2}`. On one line each is a request, like a size on a jump:
an instruction that cannot have that encoding is an error (`{vex3} cannot
encode this instruction`; `x86.select_check` reads the first byte after
legacy prefixes; `tests/iset/encoding_request.asm`). Set with `use` for a
block, they stay preferences, dropped where they cannot apply.

### 2.2 Central VEX/EVEX gating

`AVX_512.store_instruction@ctx` (`avx.inc` ~507–690) makes the
prefix-family decisions from the `evex_f` flags:

- `x86.simd < AVX512F` adds `EVEX_FORBIDDEN`.
- A 256-bit operand with `EVEX_VL_AVX2` requires AVX2.
- EVEX-only operands (masks, `xmm16+`, broadcast, embedded rounding, or
  `EVEX_REQUIRED`) choose EVEX. EVEX then requires AVX512F, plus BW or DQ
  when the `EVEX_BW` / `EVEX_DQ` flags are set, and AVX512VL when the vector
  is narrower than 512 bits.
- Otherwise the encoder chooses VEX and requires AVX.

Feature-level choices sit *before* this, in the mnemonic. FMA and VAES pick
`EVEX_AS_VEX` or `EVEX_REQUIRED` with `if x86.ext and x86.FMA.ext`. That is
the pattern for dual-encoded families (§5.2).

### 2.3 CPUID windows in `x86.ext`

`x86.ext` is one large number; each CPUID register gets a 32-bit window.

| Window | Register | State |
|--------|----------|-------|
| 0   | 1:ECX               | ok |
| 32  | 7.0:EBX             | ok |
| 64  | 7.0:ECX             | ok |
| 96  | 7.0:EDX             | ok |
| 128 | 7.1:EAX             | HRESET. It used to share this window with 0Dh.1:EAX (G1, fixed). |
| 160 | 0Dh.1:EAX           | XSAVEOPT, XSAVEC, XSS (moved here by G1) |
| 224 | 80000001h:ECX       | SVM, LZCNT, PRFCHW, SKINIT |
| 256 | 80000001h:EDX       | ok |
| 320 | 8000001Fh:EAX       | SEV_ES |

`x86.ext` and these windows are internal, like the file names; users name
features, never numbers. The fixed relationship to CPUID is still useful:
CPUID check code can be generated straight from the window and bit.
`tests/iset/cpuid_windows.asm` pins every feature to its leaf, register and
bit.

**Cost.** Every gated instruction tests bits of this number, so fasmg does
a big-integer AND (already over 320 bits) on each use. A table indexed by
window and bit, needing two or three word lookups per check, may be faster
than one wide operation. It isn't needed yet. Revisit it if assembly
time of extension-heavy code becomes a problem; since users only name
features, the change would not affect sources.

### 2.4 Defects to fix before adding anything

G1–G24, G26 and G27 are fixed; G25 is open. Each fixed defect has a test
in `tests/iset/` that passes on this branch and fails on the tree before
the fix (see `tests/iset/readme.md`; `_on_tree.py` runs a test's cases with
another tree). G15–G24, G26 and G27 are upstream defects (present on
`master`); G25 is in this branch's own `msr.inc` (b3b0669). The GNU as
comparison (`tests/iset/_apx_gas.py`) found G23–G25. Repros and root causes
for an upstream report are in [`apx-plan.md`](apx-plan.md) §5.3. G15's class
is checked over every mnemonic by `tests/iset/_imm_probe.py`, G26/G27's by
`_internal_errors.py`.

| # | Defect | Effect | Fix | Test |
|---|--------|--------|-----|------|
| G1 | Window 128 mixes two leaves | Adding SHA512 (7.1 bit 0), SM3 (bit 1) or RAO_INT (bit 3) would make `use XSAVEOPT` enable SHA512, and so on | Move XSAVEOPT, XSAVEC and XSS to a new window at 160. Users only ever name the feature, never the number, so this is source-compatible. | `cpuid_windows.asm` |
| G2 | `lzcnt` requires `BMI1` (`bmi.inc:80`) | LZCNT is CPUID 80000001h:ECX[5] (ABM). AMD K10 has LZCNT without BMI1, and an Intel `use BMI1` alone wrongly allows it | Add `LZCNT: ext \| 224 + 5` and gate `lzcnt` on it. `tzcnt` stays on BMI1. Behaviour change: `use BMI1` alone no longer allows `lzcnt`. | `lzcnt.asm` |
| G3 | `xtest` is defined twice: once under HLE (`x86-2.inc:4123`), once under RTM (`:4166`) | The second definition wins, so a CPU with HLE but no RTM is refused | Keep one CALM `xtest` with an any-of gate (§3.5): HLE or RTM | `xtest.asm` |
| G4 | `prefetch`/`prefetchw` require `3DNow` (`3dnow.inc:31`) | Every Intel target (Broadwell+) is refused `prefetchw` | Add `PRFCHW: ext \| 224 + 8`. Gate `prefetchw` on any-of 3DNow or PRFCHW. `prefetch` stays 3DNow only. | `prefetchw.asm` |
| G5 | `AVX_512.parse_bcst_operand` only accepts `1to2`…`1to16` | Broadcasts of 16-bit elements (FP16, BF16 inputs) to 512 bits need `1to32` | Add a `1to32` branch. Unit sizes 4 and 8 still reject it, through the existing size-mismatch error. | `broadcast.asm` |
| G6 | VNNI and IFMA are hard-wired `EVEX_REQUIRED` (`avx.inc:4161`, `:4299`) | The VEX forms (AVX-VNNI, AVX-IFMA) cannot be emitted: `use AVX2, AVX512_VNNI` + `vpdpbusd ymm…` gives `invalid operand` | Move both families to `vnni.inc`/`ifma.inc`, which own both gates (§5.2) | `vnni_ifma.asm` |
| G7 | The shared VEX encoder requires `AVX` for every VEX instruction, including the VEX-encoded general-purpose ones (BMI1, BMI2, CMPccXADD) | `use x64, BMI1` + `andn eax, ecx, edx` gave `AVX or higher required`; some parts have BMI1/BMI2 without AVX | New flag `VEX_GPR` (`avx.inc`) skips the AVX check; `AVX.store_gpr_instruction@ctx` sets it. BMI1/BMI2 and CMPccXADD use it; vector VEX instructions still require AVX. | `bmi_gpr.asm` |
| G8 | `vp4dpwssd`/`vp4dpwssds` compressed disp8 by the 64-byte vector; their memory operand is 128 bits (Tuple1_4X), so the CPU scales disp8 by 16 | `[rax+40h]` encoded disp8 = 1, which addresses `[rax+10h]`: a wrong address with no error | `memsize = 16` in place in `avx.inc` | `avx512_4x.asm` |
| G9 | `avx.inc` spelled compare predicate 0Ch `neq_qq` | `vcmpneq_oqps/pd/ss/sd` (SDM names) were illegal instructions | Add `neq_oq`; keep `neq_qq` as an alias | `cmp_predicates.asm` |
| G10 | `vcvttps2qq`/`vcvttps2uqq` called `AVX_512.parse_sae @src,sae` (space before the context) | Their `{sae}` form failed with `illegal instruction` | `AVX_512.parse_sae@src sae` | `avx512_sae.asm` |
| G11 | `bndcl/bndcu/bndcn` called `x86.parse_operand @src,src` (same typo) | All three failed with `illegal instruction` in every form | `x86.parse_operand@src src` | `mpx_bndc.asm` |
| G12 | `avx.inc` listed `vpmuldq` in two `iterate` blocks; the later, EVEX-only one replaced the VEX/EVEX one | `vpmuldq xmm/ymm` (AVX/AVX2) refused | Remove it from the later block | `redefinition.asm` |
| G13 | `avx.inc` defined `vcvtph2ps`/`vcvtps2ph` twice; the later F16C VEX-only pair replaced the VEX+EVEX pair | AVX-512 forms (zmm, masks, `{sae}`) refused | One definition each: VEX needs F16C, EVEX needs AVX512F (`F16C.gate`) | `redefinition.asm` |
| G14 | `sse2.inc`/`sse4.inc` redefined `movd`, `movq`, `pinsrw`, `pextrw`, `pmovmskb` to add xmm (and memory) forms with one blanket `require SSE2+`/`SSE4.1+` and never passed on | Older forms got the newer gate: `movd mm1, eax` needed SSE2; `pextrw eax, mm1, 1` and `pextrw eax, xmm1, 1` needed SSE4.1 | Each layer gates only its own forms and passes the rest to the earlier definition (`movd`, `pinsrw`, SSE4.1 `pextrw`), or gates per form where it also added forms (`movq` mm↔r64, r64 destinations) | `stacking.asm` |
| G15 | `AVX_512.single_source_instruction_imm8` (`vroundpd/ps`, `vaeskeygenassist`, `vpcmp[ei]str[im]`) and `vcvtps2ph` never checked that the imm8 operand is an immediate | `vroundpd xmm0, xmm1, [rax]` assembled when an earlier line left `@aux.imm` defined, with that stale value as imm8: wrong bytes, no error | `@aux.type = 'imm'` in both conditions; `_imm_probe.py` finds no other helper (79,994 pairs) | `avx_imm8.asm` |
| G16 | `sse2.inc` reuses SSE's `iterate` list, so `rsqrt`/`rcp` also generate `rsqrtpd`, `rsqrtsd`, `rcppd`, `rcpsd` | Four instructions that do not exist (66h/F2h on 0F 52/53) assemble without error; XED refuses the bytes (#UD) | Dropped `rsqrt` and `rcp` from the SSE2 list | `sse2_rcp.asm` |
| G16b | `cvtss2si`/`cvttss2si`, `cvtps2pi`/`cvttps2pi` (`sse.inc`) and `pextrb` (`sse4.inc`) compared an operand's *size* with `'mem'`/`'mmreg'` (meant `.type`), so their size check never applied | `cvtss2si eax, mm0` assembled as `cvtss2si eax, xmm0`; `cvtps2pi mm0, mm1` as `cvtps2pi mm0, xmm1`; `pextrb dword [rax], xmm0, 1` took a dword destination (m8 only) | `.type` in the three comparisons | `sse_operand_size.asm` |
| G17 | `cmpxchg16b` wrote REX.W with `x86.store_operand_prefix` before `x86.store_instruction`'s own REX | `cmpxchg16b [r8]` was `48 41 0F C7 08`; only the REX next to the opcode counts, so it executed `cmpxchg8b [r8]` | `x86.select_operand_prefix@dest 8`, so W joins the one REX (or REX2) | `cmpxchg16b.asm` |
| G18 | `crc32 r64, r/m8` (`sse4.inc`) wrote no REX.W | `crc32 rbx, al` was `F2 0F 38 F0 D8`, the `crc32 ebx, al` encoding (same result, as the destination is zero-extended; not the SDM form) | W for a 64-bit destination | `crc32.asm` |
| G19 | `wrssd/wrssq/wrussd/wrussq` (`cet.inc`) accepted a register destination | `wrssq rax, r8` assembled with ModRM.mod = 11; only `m32/m64, r` exists | memory destination only | `cet_wrss.asm` |
| G20 | `vpinsrq` and `vlddqu` (`avx.inc`) parsed the operand they store with `x86.parse_operand`, which leaves `mask`, `evex_b`, `memsize` unset | first in a source: `invalid expression`; after another AVX instruction: whatever it left behind | `AVX_512.parse_operand` | `avx_operand_state.asm` |
| G21 | `vpbroadcastq` xmm/m64 form used W0 for EVEX too; `vpbroadcastd/q` lacked `EVEX_VL` | `vpbroadcastq zmm0, [rax]` decoded as `vbroadcasti32x2` (AVX512DQ, per-dword masking); masked xmm/ymm forms did not require AVX512VL | `EVEX_W1` for q, `EVEX_VL` for both | `vpbroadcastq.asm` |
| G22 | `vcompressps/vcompresspd` declared map 1 (`VEX_66_0F_…`) instead of map 2 | `vcompresspd [rax], xmm0` = `62 F1 FD 08 8A 00`, undecodable (#UD) | `VEX_66_0F38_…` | `vcompress.asm` |
| G23 | `lddqu` (`sse3.inc`) parsed its destination with `SSE.parse_operand@src` | first in a source: `invalid expression`; after `addps xmm5, xmm1`, `lddqu xmm0, [rax]` assembled as `lddqu xmm5, [rax]`: wrong register, no error | `SSE.parse_operand@dest dest` | `sse3_lddqu.asm` |
| G24 | `AMX.parse_sib_operand` (`amx.inc`) forced SIB with `take`/`compute`/`take` on `x86.usesib` | under a `{settings}` prefix (`{evex}`, `{vex3}`, `{imm8}` …) on tileloadd/tileloaddt1/tilestored, `x86.usesib` stayed 1: every later memory operand got a SIB byte (`lea eax, [rax]` = `8D 04 20`) | save and restore the value with `compute` | `amx_sib.asm` |
| G25 *(open)* | `urdmsr r64, r64` (`msr.inc`) puts the destination in ModRM.reg | `urdmsr rax, rcx` = `F2 0F 38 F8 C1`, which is `urdmsr rcx, rax` (SDM 093: Op/En MR; XED; GNU as `…C8`); llvm-mc does the same | destination in ModRM.rm, as the imm32 form already does; `msr.asm` and `apx_promoted.asm` expectations change | — |
| G26 | `aes.inc`: `vaeskeygenassist` passed `opcode`, its block's parameter is `supp` | `vaeskeygenassist` never assembled: "symbol 'opcode' is undefined" | `supp` (upstream d4be069 fixed the same swap in `vaesenc`/`vaesdec`) | `aes_forms.asm` |
| G27 | `aes.inc`: `aesimc` (SSE) passed `supp`, its block's parameter is `opcode` | `aesimc` never assembled: "symbol 'supp' is undefined" | `opcode` | `aes_forms.asm` |

---

## 3. Strategy checklist for every addition

Each addition must settle all of the following. The per-extension plans in
§5 fill in these fields.

### 3.1 File name
- Lower case and named for the feature family as the CPUID/table names it:
  `svm.inc`, `sev.inc`, `sha512.inc`.
- **One file holds one gate family.** That is a set of mnemonics that share
  a feature bit, or a set of tightly related bits that share one encoder
  path (for example `svm.inc` = SVM + SKINIT + SEV_ES, since `vmgexit` is
  `vmmcall` with an F3 prefix).
- **Single-instruction features go in a shared family file** (`amd.inc`,
  `cache.inc`, …) rather than a 10-line file each.
- **Vendor-specific, undocumented or historical sets are ordinary files.**
  They are included by `x86-2.inc` and gated like everything else (§3.4,
  §5.7).
- **Every file starts with its CPUID requirements**, so a reader can answer
  "I want these instructions; what do I have to enable?" without reading
  any code. The header lists the `use` setting, the CPUID leaf and bit
  from §4, the instructions it unlocks, and any mode restriction.
  Any-of and all-of gates are written out in full. `svm.inc` is the
  model:

  ```
  ; CPUID requirements ("use <setting>" to enable, or "use everything"):
  ;
  ;	setting	CPUID bit		instructions
  ;	SVM	8000_0001h:ECX[2]	vmrun vmload vmsave vmmcall clgi invlpga
  ;	SKINIT	8000_0001h:ECX[12]	skinit
  ;	SVM or SKINIT			stgi
  ;
  ; Valid in 16-, 32- and 64-bit code.
  ```

  Existing files (`vmx.inc`, `cet.inc`, `avx.inc`, …) have no such header
  yet. Add it when each file is next touched.

### 3.2 Include point
- Place the file after the files whose helpers it uses (§1.1: fewer
  passes; visibility is not the reason).
- If a file *depends* on another file's encoder change (XOP before FMA4,
  TBM and LWP), it is included after that file, and the dependency is
  stated in its header comment.
- **Every file is included by `x86-2.inc`.** Users never write
  `include 'iset/…'`. A file that `x86-2.inc` does not include is a bug,
  as `svm.inc` was until it was fixed (§5.1).

### 3.3 Mnemonic ownership
- Run the probe (§6) before writing anything, to see whether the mnemonic
  already exists.
- **If it exists, change it in place, in the file that owns it**, or
  stack a new layer over it that gates only its own forms and passes
  every other form to the earlier definition by calling its own name
  (§1.3 rule 5). Never replace it.
- **`python tests/iset/_dupnames.py` must pass.** It fails on any
  instruction name defined twice that is not one of the reviewed,
  deliberate stacks listed in it.
- **When a whole family changes owner, do it in one commit.** The family
  moves to a new file, and the old definitions are removed in the same
  change, as planned for VNNI, IFMA and BF16.
- **List byte-level collisions in the header comment.** These are a
  different mnemonic on the same opcode bytes, for example:
  - `urdmsr` and `enqcmd`: both `F2 0F 38 F8`, told apart by register vs
    memory ModRM.
  - `erets` and `clac`: both `0F 01 CA`, told apart by the prefix.
  - `extrq`/`insertq` and `vmread`/`vmwrite`: `0F 78/79`, told apart by the
    prefix.

  The operand check in the new instruction must keep its own form apart
  from the neighbour's.

### 3.4 Feature bit
- Add the table line in `x86-2.inc` in the window of the CPUID register that
  reports the feature (§4), with a comment naming the leaf.
- Names follow the existing style: SDM/APM names, upper case, `_` for
  spaces. Examples: `AVX_VNNI`, `AVX512_FP16`, `SEV_SNP`.
- **No CPUID bit exists?** Then the feature gets a bit in the synthetic
  `ext` window (§4). Vendor-undocumented sets are treated the same way.
- **Gates defined by CPU generation** (`umov`, `loadall`, `xbts`) use the
  existing `cpu` level with `requireexact`, not a new bit.
- **Presets must not change meaning.** `everything` keeps `ext = -1`, so
  it picks up every new `ext` bit automatically, including the synthetic
  ones. Their mnemonics are new names, so enabling them changes no
  existing instruction's bytes.

### 3.5 Gate code (first lines of the CALM body)
- **One feature:** `call x86.require.NAME`.
- **Any of several features** (STGI, XTEST, PREFETCHW):

  ```
  	check	x86.ext and (x86.SVM.ext or x86.SKINIT.ext)
  	jyes	supported
  	err	'SVM or SKINIT required'
      supported:
  ```

  The message uses the same `NAME required` wording as the generated gates.
- **64-bit only:** `call x86.require.bits64`. It produces the standard
  message and also covers the "16/32-bit mode" case.
- **Not valid in 64-bit mode:** `check x86.mode < 64` / `err 'instruction
  not allowed in long mode'`.
- **Exact CPU generation** (undocumented opcodes): `call
  x86.requireexact.80386`, etc., as `xbts`/`ibts` already do.

### 3.6 Operand validation
- Reject every operand form the instruction does not define, with an
  explicit `err`. The usual cases are:
  - the wrong register class
  - a fixed register that is not the expected one (rAX, ECX, EAX)
  - memory where only a register is allowed, or the reverse
  - the wrong width
  - masks, broadcast or `xmm16+` on VEX-only instructions
- **Implicit operands** (`vmrun rax`, `invlpga rax,ecx`, `skinit eax`,
  `monitorx`, `clzero`) are optional. When written, they must name exactly
  the architectural register.
- The rAX operand means *address size*: use `x86.store_address_prefix`,
  which emits `67h` and never `66h`/`REX.W` (§5.1).

### 3.7 Encoding path
- **Fixed bytes:** use `emit`.
- **Implicit register operands** (all optional; written ones are checked):
  `x86.address_operand` for a rAX that is an address (emits `67h` when
  narrower than the mode), and `xcall x86.implicit_operand, op, <ecx>`
  for a fixed register, named as written. The expected register is
  parsed like the operand, and a mismatch reports both:
  `invalid implicit operand: rcx, expected ecx`. Both helpers are in
  `x86-2.inc`, next to `x86.store_address_prefix`. In CALM, `call` passes
  only symbols; literal numbers need `xcall` with `(n)`.
- **ModRM with GPR or memory:** use `x86.store_instruction@ctx`. Set
  `@ctx.opcode_prefix` for mandatory `66`/`F2`/`F3`. Use
  `x86.select_operand_prefix@ctx` or `x86.store_operand_prefix` for width.
- **VEX/EVEX:** use `AVX.store_instruction@ctx` (VEX only: it adds
  `EVEX_FORBIDDEN`) or `AVX_512.store_instruction@ctx` with explicit
  `evex_f` flags.
- **New maps:** the map number goes in the low bits of `vex_mpw`.
  - Map 5 is confirmed working (`62 F5 6C 48 58 CB` = `vaddph zmm1,zmm2,zmm3`).
  - Maps 6 (FP16) and 7 (VEX, USER_MSR) use the same field; check them with
    a byte test.

### 3.8 Tests that must exist before merge
Each addition gets a self-contained source in `tests/iset/` (see its
`readme.md`). Byte checks run inside fasmg, and `; FAIL n: 'message'`
cases are run by `tests/iset/_run.ps1`. The test must fail on the tree
before the change. Each item below is an assembly to run and compare
with the reference or expected error:
1. **Positive bytes:** every operand form in every legal mode, compared
   byte-for-byte against `llvm-mc` (or the APM/SDM where llvm has no
   support).
2. **Feature off:** with the bit cleared, every mnemonic fails with
   `NAME required`.
3. **Mode:** wrong-mode use fails, for example 64-bit-only instructions under
   `use32`.
4. **Operands:** each rejected form from §3.6 fails.
5. **Neighbours:** the byte-collision neighbours from §3.3 still assemble
   to their old bytes.
6. **Presets:** the probe (§6) shows every new mnemonic defined under
   `use everything`, with no extra include.

---

## 4. Proposed `x86.ext` layout and new table lines

| Window | Register | Contents |
|--------|----------|----------|
| 0   | 1:ECX            | unchanged |
| 32  | 7.0:EBX          | + `CLWB` (24) |
| 64  | 7.0:ECX          | + `PREFETCHWT1` (0) |
| 96  | 7.0:EDX          | + `AVX512_4FMAPS` (3), `AVX512_VP2INTERSECT` (8), `PCONFIG` (18), `AVX512_FP16` (23) |
| 128 | 7.1:EAX          | `HRESET` (22, unchanged) + `SHA512` (0), `SM3` (1), `SM4` (2), `RAO_INT` (3), `AVX_VNNI` (4), `AVX512_BF16` (5), `CMPCCXADD` (7), `FRED` (17), `LKGS` (18), `WRMSRNS` (19), `AMX_FP16` (21), `AVX_IFMA` (23), `MSRLIST` (27) |
| 160 | 0Dh.1:EAX        | `XSAVEOPT` (0), `XSAVEC` (1), `XSS` (3): moved from 128 (G1) |
| 192 | 7.1:EDX          | `AVX_VNNI_INT8` (4), `AVX_NE_CONVERT` (5), `AMX_COMPLEX` (8), `AVX_VNNI_INT16` (10), `PREFETCHI` (14), `USER_MSR` (15) |
| 224 | 80000001h:ECX    | `SVM` (2), `LZCNT` (5), `SSE4A` (6), `PRFCHW` (8), `XOP` (11), `SKINIT` (12), `LWP` (15), `FMA4` (16), `TBM` (21), `MONITORX` (29). `SVM` and `SKINIT` are already defined. |
| 256 | 80000001h:EDX    | `3DNow` (31, unchanged) + `3DNowExt` (30) |
| 288 | 80000008h:EBX    | `CLZERO` (0), `INVLPGB` (3), `RDPRU` (4), `MCOMMIT` (8), `WBNOINVD` (9) |
| 320 | 8000001Fh:EAX    | `SEV_ES` (3, defined), `SEV_SNP` (4, XED `SNP`), `RMPQUERY` (6, *verify*: XED has no entry) |
| 352 | 14h.0:EBX        | `PTWRITE` (4). XED also requires Intel PT (7.0:EBX[25]), so add `INTEL_PT: ext \| 32 + 25` and gate on both. |
| 384 | 12h.0:EAX        | `ENCLV` (5, *verify*: XED's `sgx-enclv` has no CPUID entry) |
| 416 | C000_0001h:EDX   | VIA/Zhaoxin PadLock "present" bits: `PADLOCK_RNG` (2), `PADLOCK_ACE` (6), `PADLOCK_ACE2` (8), `PADLOCK_PHE` (10), `PADLOCK_PMM` (12) |
| 480 | synthetic        | Features with no CPUID bit: `TDX` (0), detected through SEAM/MSRs; `CYRIX` (1); `GEODE` (2) |

The one new kind of setting is for AVX10, which the hardware reports as a
version number (CPUID 24h:EBX[7:0]), not as a bit:

```
	AVX10.1:	avx10 = 1
	AVX10.2:	avx10 = 2
```

`VMFUNC` needs no bit: it is VMX-controlled, so it gates on `VMX`.

---

## 5. Plans by extension

Each entry gives: **file** · **include point** · **gate** · **modes** ·
**encoding** · **must reject** · **notes**.

### 5.1 AMD-V (SVM): `svm.inc`

- **Include point:** after `uintr.inc` (GPR group).
- **Gate:**
  - `SVM` for `vmrun`, `vmload`, `vmsave`, `vmmcall`, `clgi`, `invlpga`.
  - `SKINIT` for `skinit`.
  - Any-of `SVM`/`SKINIT` for `stgi`. The APM allows STGI when either bit
    is set.
  - `SEV_ES` for `vmgexit` (`F3 0F 01 D9`, *APM*). An SEV-ES guest uses it
    to exit to the hypervisor. It lives here because it is `vmmcall` with
    an F3 prefix; its gate is separate.
- **Modes:** all code modes. Protected mode is a run-time requirement, not
  something the assembler can check.
- **Encoding:** `0F 01 D8/DA/DB` (`vmrun`/`vmload`/`vmsave`), `D9`
  (`vmmcall`), `DC` (`stgi`), `DD` (`clgi`), `DE` (`skinit`), `DF`
  (`invlpga`).
- **Must reject:**
  - Any register other than rAX/eAX/AX.
  - A rAX width that is illegal in the current mode: `ax` in 64-bit mode,
    `rax` outside it.
  - Memory operands.
  - `skinit` with anything other than `eax`.
  - `invlpga` with an ASID operand other than `ecx`.

**Status: implemented on branch `win32`.**
- `bb403a8 fix SVM support` added `include/iset/svm.inc`, the `SVM` and
  `SKINIT` table lines, and `x86.store_address_prefix`.
- Follow-up commits on this branch close four gaps found in review, add
  `vmgexit`, and include the file from `x86-2.inc` (it was the only `iset`
  file left out):

| # | Case | Before | After |
|---|------|--------|-------|
| S1 | bare `invlpga` under `use32`/`use16` | `invalid operand size` (the default `addr:rax`) | `0F 01 DF` |
| S2 | bare `vmrun`/`vmload`/`vmsave` | `missing argument` | `0F 01 D8/DA/DB` |
| S3 | `skinit` gate | `SVM` | `SKINIT`. With only SVM set, `skinit` is now refused. |
| S4 | `stgi` gate | `SVM` | SVM **or** SKINIT |

The fix makes both rAX operands optional: when the operand is omitted,
`x86.store_address_prefix` is skipped and no `67h` is emitted.

Byte results, all matching `llvm-mc`:

| Input | Bytes |
|-------|-------|
| `vmrun`, `invlpga`: bare, in every mode | `0F 01 D8`, `0F 01 DF` |
| `use64` `vmrun rax` / `vmrun eax` | `0F 01 D8` / `67 0F 01 D8` |
| `use32` `vmrun eax` / `vmrun ax`  | `0F 01 D8` / `67 0F 01 D8` |
| `use16` `vmrun ax` / `vmrun eax`  | `0F 01 D8` / `67 0F 01 D8` |
| `vmload rax`, `vmsave eax` (64-bit) | `0F 01 DA`, `67 0F 01 DB` |
| `vmmcall`, `stgi`, `clgi`, `skinit` | `0F 01 D9`, `DC`, `DD`, `DE` |
| `invlpga rax,ecx`, `invlpga eax,ecx` (64-bit), `invlpga ax,ecx` (32-bit) | `0F 01 DF`, `67 0F 01 DF`, `67 0F 01 DF` |

Refused, as required:
- `vmrun ax` or `invlpga ax,ecx` (64-bit), `vmrun rax` (32-bit) and
  `vmrun al` → `invalid operand size`
- `vmrun rbx`, `vmrun [rax]` → `only rAX register supported`
- `skinit ax`/`rax` → `register EAX expected`
- `invlpga rax,rcx`, `invlpga rbx,ecx` → `invlpga operands must be rAX and ECX`

Gates checked:

| `x86.ext` | `stgi` | `skinit` | `clgi`/`vmrun` |
|-----------|--------|----------|----------------|
| SKINIT only | assembles | assembles | `SVM or higher required` |
| SVM only    | assembles | `SKINIT or higher required` | assemble |
| neither     | `SVM or SKINIT required` | refused | refused |

`vmgexit`: `F3 0F 01 D9` in all three modes, and `vmgexit rax` → `extra
characters on line`. With only SEV_ES set, `vmgexit` assembles and
`vmmcall` → `SVM or higher required`. With only SVM set, `vmgexit` →
`SEV_ES or higher required`. No local tool knows `vmgexit` (§6.1), so the
bytes come from the APM.

**Regression check:** `C:\git\_vm\svm_labs` (UEFI SVM bring-up labs) uses
`vmsave/vmrun/vmload rax`, `clgi`, `stgi` and `vmmcall`. All four stages
build byte-identical images with `bb403a8` and with this branch, even though
the labs still `include 'iset/svm.inc'` themselves: the second definition
only stacks. That line is now redundant and can be removed. The labs only
use explicit `rax` in 64-bit mode, so the bare, 16/32-bit and SKINIT-only
cases above rest on the byte tests, not on the labs.

### 5.2 Dual-encoded vector families (move and merge)

These features add a **VEX** form to a mnemonic that today exists only as
EVEX, or the reverse. Each family moves into one CALM file that owns both
gates, and its old definitions in `avx.inc` are removed in the same change.

The gate logic inside the mnemonic, as implemented in `vnni.inc`:

```
	local	evex_f
	check	x86.ext and x86.AVX_VNNI.ext
	jyes	vex_available
	check	x86.ext and x86.AVX512_VNNI.ext
	jyes	evex_only
	err	'AVX_VNNI or AVX512_VNNI required'
	exit
    evex_only:
	compute evex_f, EVEX_REQUIRED+EVEX_VL
	jump	encode
    vex_available:
	compute evex_f, EVEX_AS_VEX+EVEX_VL+EVEX_VL_AVX2
	check	x86.ext and x86.AVX512_VNNI.ext
	jyes	encode
	compute evex_f, EVEX_FORBIDDEN+EVEX_VL_AVX2	; VEX only: masks/zmm/xmm16+ refused
    encode:
	xcall	AVX_512.calm_basic_instruction_bcst, (VEX_66_0F38_W0),evex_f,(opcode),(4),dest,src,src2
```

`AVX_512.calm_basic_instruction_bcst` (in `avx.inc`) is the CALM form of
the `AVX_512.basic_instruction_bcst` macro, so CALM mnemonics can use it.
When the VEX feature is on but the AVX-512 one is off, `{evex}` cannot
force EVEX; the instruction stays VEX.

**Status:** `vnni.inc` and `ifma.inc` are done (G6, `tests/iset/vnni_ifma.asm`),
including AVX-VNNI-INT8/INT16. `bf16.inc` is done (`tests/iset/bf16.asm`): `vcvtneps2bf16` owns both gates; the six AVX-NE-CONVERT loads are memory-only and VEX-only.

| File | Mnemonics | VEX gate | EVEX gate | Reference bytes |
|------|-----------|----------|-----------|-----------------|
| `vnni.inc` | `vpdpbusd(s)`, `vpdpwssd(s)` | `AVX_VNNI` | `AVX512_VNNI` | `{vex} vpdpbusd ymm1,ymm2,ymm3` = `C4 E2 6D 50 CB` |
| `ifma.inc` | `vpmadd52luq`, `vpmadd52huq` | `AVX_IFMA` | `AVX512_IFMA` | `{vex} vpmadd52luq ymm1,ymm2,ymm3` = `C4 E2 ED B4 CB` |
| `bf16.inc` | `vcvtneps2bf16` (both forms); EVEX only: `vcvtne2ps2bf16`, `vdpbf16ps`; VEX only: `vbcstnebf162ps`, `vbcstnesh2ps`, `vcvtnee{bf16,ph}2ps`, `vcvtneo{bf16,ph}2ps` | `AVX_NE_CONVERT` | `AVX512_BF16` | `{vex} vcvtneps2bf16 xmm1,ymm2` = `C4 E2 7E 72 CA`; EVEX `62 F2 7E 48 72 CA`; `vdpbf16ps zmm` = `62 F2 6E 48 52 CB`; `vbcstnebf162ps ymm1,[rax]` = `C4 E2 7E B1 08` |

- `vnni.inc` also gets the VEX-only sibling families:
  - `AVX_VNNI_INT8`: `vpdpb{ss,su,uu}d(s)`, `F2/F3/NP 0F38 50/51 W0`
  - `AVX_VNNI_INT16`: `vpdpw{su,us,uu}d(s)`, `F3/66/NP 0F38 D2/D3 W0`

  Both are VEX-only (`EVEX_FORBIDDEN`) until AVX10.2 adds EVEX forms; that
  will be one more gate in the same file. Reference bytes:
  `vpdpbssd ymm1,ymm2,ymm3` = `C4 E2 6F 50 CB`;
  `vpdpwsud ymm1,ymm2,ymm3` = `C4 E2 6E D2 CB`.
- **Include point:** after `avx.inc`.
- **Behaviour change to accept:** under `use everything`, both bits are
  set, so `vpdpbusd xmm/ymm` without masks changes from EVEX to the shorter
  VEX encoding. Every other dual instruction already behaves this way
  (`vaddps`, FMA), and `{evex}` still forces EVEX.
- **Must reject:**
  - masks, `{1toN}`, `zmm` and `xmm16+` when only the VEX feature is on
  - any use when neither bit is on, with the message naming the VEX feature
  - memory sizes other than the vector or element size

### 5.3 New EVEX/VEX vector families

| File | Gate | Mnemonics | Encoding notes | Must reject |
|------|------|-----------|----------------|-------------|
| `avx512_fp16.inc` **(done, `tests/iset/avx512_fp16.asm`, `avx512_fp16_cvt.asm`)** | `AVX512_FP16` (+ VL for xmm/ymm through the central gate) | the full `…ph`/`…sh` set, `vmovw`, `vfcmaddcph`, `vfmaddcph`, … | EVEX **map 5 and 6** in `vex_mpw` (map 5 confirmed). Needs G5 (`1to32`). Unit 2. | VEX encodings (`EVEX_REQUIRED` everywhere); `1to32` on non-word units; for complex FMA, destination equal to a source (SDM #UD) |
| `vp2intersect.inc` **(done, `tests/iset/vp2intersect.asm`)** | `AVX512_VP2INTERSECT` | `vp2intersectd`, `vp2intersectq` | `EVEX.F2.0F38 68` W0/W1; destination is a k-register **pair**, written as the even register (`vp2intersectd k2,zmm2,zmm3` = `62 F2 6F 48 68 D3`) | odd k-register destination; masking of the destination |
| `sha512.inc` **(done, `tests/iset/sha512.asm`)** | `SHA512` | `vsha512msg1`, `vsha512msg2`, `vsha512rnds2` | `VEX.256.F2.0F38 CC/CD/CB W0` (`C4 E2 7F CC CA`, …) | xmm destination; memory (all forms are register-only); EVEX features |
| `sm.inc` **(done, `tests/iset/sm.asm`)** | `SM3`, `SM4` (separate gates in one ShangMi file) | `vsm3msg1/2`, `vsm3rnds2`, `vsm4key4`, `vsm4rnds4` | SM3: `VEX.128 0F38 DA` (NP/66), `0F3A DE ib`; SM4: `VEX.{128,256} F3/F2 0F38 DA` | 256-bit SM3; EVEX features; immediate wider than a byte |
| `amx.inc` (in place) **(done, `tests/iset/amx.asm`)** | `AMX_FP16`, `AMX_COMPLEX` | `tdpfp16ps`, `tcmmimfp16ps`, `tcmmrlfp16ps` | Copy the `tdpbf16ps` shape: `F2/66/NP 0F38 5C/6C` (`C4 E2 63 5C CA`) | the same tile register used twice (existing AMX rule); non-tile operands |
| `avx.inc` (in place, next to `vp4dpwssd`) **(done, `tests/iset/avx512_4x.asm`)** | `AVX512_4FMAPS` | `v4fmaddps`, `v4fnmaddps`, `v4fmaddss`, `v4fnmaddss` | Copy the `vp4dpwssd` register-block syntax; `EVEX.F2.0F38 9A/AA/9B/AB` | non-`+3` register block; non-memory source. This is Xeon Phi only, so low priority. |

`sm.inc` was smoke-tested in CALM form. `vsm3msg1 xmm1,xmm2,xmm3` →
`C4 E2 68 DA CB`. `ymm`, `xmm17`, `{k1}` and `qword [mem]` are all
rejected. That proves the CALM → `AVX.store_instruction@src2` route for
new VEX instructions.

### 5.4 Scalar and system: Intel

| File | Include point | Gate | Mnemonics → bytes | Modes | Must reject / collisions |
|------|---------------|------|-------------------|-------|--------------------------|
| `cache.inc` **(done, `tests/iset/cache.asm`)** | GPR group | `CLWB` · `PREFETCHWT1` · `PREFETCHI` · `WBNOINVD` | `clwb m8` → `66 0F AE /6`; `prefetchwt1 m8` → `0F 0D /2`; `prefetchit0/1 m8` → `0F 18 /7`, `/6`; `wbnoinvd` → `F3 0F 09` | `prefetchi*`: 64-bit, **RIP-relative only** (other addressing is a reserved NOP) | `clwb` with a register operand. `66 0F AE /6` with a register ModRM is `tpause`; the probe must still show `tpause` unchanged. Non-RIP memory for `prefetchi*`. |
| `system.inc` **(done, `tests/iset/system.asm`)** | GPR group | `PTWRITE` · `PCONFIG` · `ENCLV` · `VMX`(`vmfunc`) | `ptwrite r/m32/64` → `F3 [REX.W] 0F AE /4`; `pconfig` → `0F 01 C5`; `enclv` → `0F 01 C0`; `vmfunc` → `0F 01 D4` | `ptwrite` 64-bit width only in long mode | `ptwrite` with 8- or 16-bit operands. `vmfunc` could also live in `vmx.inc` (same gate); pick one place. |
| `cmpccxadd.inc` **(done, `tests/iset/cmpccxadd.asm`)** | after `bmi.inc` (VEX GPR via `AVX.store_gpr_instruction`, G7) | `CMPCCXADD` | `cmp<cc>xadd m, r, r` → `VEX.128.66.0F38 E0+cc`, W by width (`cmpbexadd [rdx],ecx,eax` = `C4 E2 79 E6 0A`) | **64-bit only** | a register first operand; mismatched widths; 8- or 16-bit widths. Define the alias mnemonics (`cmpcxadd`, `cmpaexadd`, `cmpzxadd`/`cmpexadd`, …) from a single `iterate` over `<cc,code>`. |
| `rao.inc` **(done, `tests/iset/rao.asm`)** | GPR group | `RAO_INT` | `aadd/aand/aor/axor m, r` → `NP/66/F2/F3 0F 38 FC /r` (+`REX.W`) | 64-bit width in long mode | register destination; 8- or 16-bit widths |
| `msr.inc` **(done, `tests/iset/msr.asm`)** | GPR group (the map-7 VEX bytes are emitted directly, so no AVX is needed) | `MSRLIST` · `WRMSRNS` · `USER_MSR` | `rdmsrlist`/`wrmsrlist` → `F2/F3 0F 01 C6`; `wrmsrns` → `0F 01 C6`; `urdmsr r64,r64` → `F2 0F 38 F8 /r`; `uwrmsr r64,r64` → `F3 0F 38 F8 /r`; `urdmsr r64,imm32` → `C4 E7 7B F8 /0 id`; `uwrmsr imm32,r64` → `C4 E7 7A F8 /0 id` | all 64-bit only (MSRLIST/USER_MSR) | `urdmsr` with memory: `F2 0F 38 F8` with memory is **`enqcmd`**, so the register-only check keeps them apart. Non-64-bit registers. |
| `fred.inc` **(done, `tests/iset/fred_tdx.asm`)** | GPR group | `FRED` · `LKGS` | `erets` → `F2 0F 01 CA`; `eretu` → `F3 0F 01 CA`; `lkgs r/m16` → `F2 0F 00 /6` | 64-bit only | `lkgs` with a non-16-bit operand. Collision: `clac` is `0F 01 CA`. |
| `tdx.inc` **(done, `tests/iset/fred_tdx.asm`)** | GPR group | `TDX` (synthetic) | `tdcall`/`seamret`/`seamops`/`seamcall` → `66 0F 01 CC/CD/CE/CF` | 64-bit only | any operand |

### 5.5 AMD scalar and system

| File | Include point | Gate | Mnemonics → bytes | Must reject |
|------|---------------|------|-------------------|-------------|
| `sev.inc` | GPR group | `SEV_SNP` · `RMPQUERY` | **Done** (`tests/iset/sev.asm`). `pvalidate` → `F2 0F 01 FF` (32/64-bit, rAX is an address); `rmpadjust` → `F3 0F 01 FE`; `rmpupdate` → `F2 0F 01 FE`; `psmash` → `F3 0F 01 FF`; `rmpquery` → `F3 0F 01 FD` (all four 64-bit only, RAX is a value) | Wrong implicit registers; `eax` where RAX is a value; 16/32-bit use of the 64-bit-only ones. Collisions: `invlpgb`/`tlbsync`/`rdpru` (same bytes, no prefix). `vmgexit` is in `svm.inc`. |
| `amd.inc` | GPR group | `MONITORX` · `CLZERO` · `RDPRU` · `MCOMMIT` · `INVLPGB` | **Done** (`tests/iset/amd.asm`). `monitorx` → `0F 01 FA`; `mwaitx` → `0F 01 FB`; `clzero` → `0F 01 FC`; `rdpru` → `0F 01 FD`; `mcommit` → `F3 0F 01 FA` (*APM*); `invlpgb` → `0F 01 FE`; `tlbsync` → `0F 01 FF` | Wrong implicit registers (zydis-db: `monitorx` rAX,ECX,EDX; `mwaitx` EAX,ECX; `invlpgb` rAX,EDX,ECX); one feature enabling another's instruction. Note: the existing `monitor`/`mwait` accept only `eax,ecx,edx` and never emit `67h`, so `monitor rax,ecx,edx` is refused in 64-bit code; they could move to the same helpers. |
| `sse4a.inc` **(done, `tests/iset/sse4a.asm`)** | after `sse4.inc` | `SSE4A` | `extrq xmm,imm8,imm8` → `66 0F 78 /0 ib ib`; `extrq xmm,xmm` → `66 0F 79`; `insertq xmm,xmm,imm8,imm8` → `F2 0F 78`; `insertq xmm,xmm` → `F2 0F 79`; `movntsd m64,xmm` → `F2 0F 2B`; `movntss m32,xmm` → `F3 0F 2B` | Memory for `extrq`/`insertq`; registers for `movnt*`. Collision: `0F 78/79` without a prefix are `vmread`/`vmwrite`. |
| `3dnow.inc` (in place) **(done, `tests/iset/3dnowext.asm`)** | — | `3DNowExt` | add `pswapd`, `pfnacc`, `pfpnacc`, `pf2iw` (`0F 0F /r` + suffix `BB/8A/8E/1C`). Move the existing `pi2fw` (`0C`) out of the 3DNow `iterate` and onto 3DNowExt. This is a behaviour change: it is refused under plain `use 3DNow`. | Non-MMX operands |
| `lzcnt`, `prefetchw` | in place | G2, G4 | — | — |

### 5.6 AMD XOP family

**Done** (`tests/iset/xop.asm`, `fma4.asm`, `tbm_lwp.asm`). The encoder
emits `8Fh` for any map >= 8; `XOP_08/09/0A_W0/W1` are in `avx.inc`. XOP
and FMA4 share `XOP.is4_instruction` (in `xop.inc`). TBM and LWP use the
no-AVX GPR store (G7). The plan below is kept as the record.

1. **Encoder (in `avx.inc`, `AVX_512.store_instruction`):**
   - When `(vex and 11111b) >= 8`, emit `8Fh` instead of `C4h` and force the
     3-byte form.
   - Add constants `XOP_08_W0`, `XOP_09_W0`, `XOP_0A_W0` (and the `W1`
     versions).

   No existing encoding changes, because no current `vex_mpw` has a map
   of 8 or above.
2. Then add the files below, each included after `avx.inc` and after the
   encoder change:

| File | Gate | Notes |
|------|------|-------|
| `xop.inc` | `XOP` | `vpcmov` (`8F E8 70 A2 C2 30`, is4 like `vblendvps`), `vprot*`, `vpsha*`, `vpshl*`, `vpcom*`, `vpmacs*`, `vpperm`, `vfrcz*`, `vphadd*`, `vphsub*`, `vpermil2p*`. VEX.W swaps which source may be memory; reject memory in the other slot. |
| `fma4.inc` | `FMA4` | `vfmadd{ps,pd,ss,sd}`, `vfmsub…`, `vfnmadd…`, `vfnmsub…`, `vfmaddsub…`, `vfmsubadd…` (`C4 E3 F1 68 C3 20`). Uses VEX map 0F3A with is4. The names differ from FMA3's `…132/213/231` forms, so there are no clashes. |
| `tbm.inc` | `TBM` | `blcfill`, `blci`, `blcic`, `blcmsk`, `blcs`, `blsfill`, `blsic`, `t1mskc`, `tzmsk` (XOP map 9). **`bextr r, r/m, imm32` (XOP map A, `8F EA 78 10 … id`) is an extra form of BMI1's `bextr`.** Add it in place in `bmi.inc`: an immediate third operand requires `TBM`, a register requires `BMI1`. Do not redefine it in `tbm.inc`. |
| `lwp.inc` | `LWP` | `llwpcb r`, `slwpcb r` (XOP.9 12 /0 /1), `lwpins`, `lwpval r, r/m32, imm32` (XOP.A 12 /0 /1) |

### 5.7 Historical and vendor sets

These files are included by `x86-2.inc` like any other (§3.2). They are
gated by synthetic `ext` bits, by real CPUID bits, or by exact CPU level.
Their opcode bytes overlap modern instructions, but their mnemonics do
not, so having them defined changes nothing for existing code. The
"Collides with" column lists the neighbours that the §3.8 neighbour tests
must keep stable.

| File | Gate | Contents | Collides with |
|------|------|----------|---------------|
| `cyrix.inc` | `CYRIX` (synthetic) | `smint`, `dmint`, `rdm`, `rdshr`, `wrshr`, `svdc`, `rsdc`, `svldt`, `rsldt`, `svts`, `rsts`, `cpu_read`, `cpu_write`; EMMI `paveb`, `paddsiw`, `pmagw`, `pdistib`, `pmachriw`, `pmvzb`, … | the `0F 38`/`0F 3A` escapes, VMX `0F 78/79`, the SSE `0F 5x` space |
| `cyrix.inc` (Geode section) | `GEODE` (synthetic) | `pfrcpv`, `pfrsqrtv` (`0F 0F /r 86/87`) | 3DNow suffix space |
| `padlock.inc` **(done, `tests/iset/padlock.asm`; four XED groups RNG/ACE/PHE/PMM)** | `PADLOCK_*` (C000_0001h:EDX) | `xstore`/`xstorerng` → `0F A7 C0`; `xcrypt{ecb,cbc,ctr,cfb,ofb}` → `F3 0F A7 C8/D0/D8/E0/E8`; `xsha1`/`xsha256` → `F3 0F A6 C8/D0`; `montmul` → `F3 0F A6 C0`; Zhaoxin GMI `ccs_hash`/`ccs_encrypt` (*verify*) | `xbts`/`ibts` (`0F A6/A7`) |
| `undoc.inc` | exact CPU level (`requireexact 80386`/`80486`) for `umov`; `fpu` for the aliases | `umov` (`0F 10–13`); FPU alias opcodes `fstp1`, `fstp8`, `fstp9`, `fxch4`, `fxch7`, `fcom2`, `fcomp3`, `fcomp5` | SSE `movups`/`movlps`, … |

**Blocked on a reference:** `cyrix.inc` and `undoc.inc`. No local tool (llvm-mc,
XED, Zydis, zydis-db) knows these opcodes, so their bytes could only come
from secondary sources; per §3.8 they wait for an authoritative listing
(Cyrix/Geode data books, Intel's 386/486 notes for `umov`).

`loadall` already exists (gated with `requireexact` on 80286/80386). The
probe in §6 reported it missing only because `everything` is not an 80286
or 80386.

### 5.8 Deferred and out of scope

**Out of scope:** encodings that are not x86-2's. Knights Corner's MVEX
and Itanium (IA-64 native code, and `jmpe`, which only switches into it)
belong to separate encoders, not to `x86-2.inc` or `iset`.

**Deferred: APX and AVX10.2.** These will probably layer over `x86-2.inc`
rather than replace it. Choose among the options when the work starts.
What is known now:
- **APX** (`push2`/`pop2`, REX2, NDD/NF forms of existing ALU instructions,
  `ccmp`/`ctest`, `jmpabs`, 32 GPRs) touches the core GPR encoder and the
  register elements: a REX2 prefix path and EVEX promotion of legacy maps.
  Gate: `APX_F` (7.1:EDX[21]) in window 192. The phased plan, starting with
  refusing r16–r31 everywhere before the names exist, is in
  [`apx-plan.md`](apx-plan.md).
- **AVX10.2** adds EVEX forms of VNNI-INT8/16, BF16 arithmetic, saturating
  conversions, and more. These fit the family files above as one more gate
  each, keyed on `avx10 >= 2`. The AVX10.1 versioning itself is just the
  `avx10` level item (§4); for gating, `AVX10.1` equals the AVX-512
  feature set it lists.

---

## 6. Verification tools

**Mnemonic probe.** Assemble `use64` followed by the bare mnemonic with
`fasmg -iInclude('fasm2.inc')`.
- `illegal instruction` means the mnemonic is undefined.
- Any other error (missing argument, invalid operand, `… required`) means it
  is defined.
- Mode-gated instructions such as `loadall` show up as false negatives under
  `use everything`.

**Byte tests.** `tests/iset/` holds the regression tests, one source per
fix, run by `tests/iset/_run.ps1` (§3.8). Gate cases select features with
the public settings (`{i386, SKINIT} stgi`, `use i386, SVM`) and expect the
`… required` message.

### 6.1 Assemblers and disassemblers

| Tool | Path | Use |
|------|------|-----|
| llvm-mc 22.1.4 (release) | `C:\Program Files\LLVM\bin\llvm-mc.exe` | Reference encodings: `-triple=x86_64 --show-encoding` (also `-triple=i386`, and `-triple=i386 -code16`). Add `-disassemble` to decode hex bytes. |
| LLVM 23.0.0git (local build, assertions on) | `C:\git\llvm-project\build-native\bin` | The same tools at the current upstream level: `llvm-mc`, `llvm-objdump`, `llvm-tblgen`, `llvm-mca`, `llvm-exegesis`, `llc`, `clang`, `lld-link`. Prefer it when an extension is new enough that the release may lag. `llvm-tblgen` can dump X86 instruction records for whole-family comparisons. |
| XED | `C:\third_party\xed\obj\wkit\bin\xed.exe` | Intel's decoder. `xed -64 -d <hex>` gives ICLASS, category, ISA set and extension. Its `-e` encoder needs full operand syntax; bare mnemonics fail. |
| ZydisInfo | `C:\git\zyantific\zydis\build\llvm-native-release\ZydisInfo.exe` | `ZydisInfo -64 <hex bytes…>` gives a full decode with map, opcode, effective operand/address size and per-flag actions. It is a second, independent decoder for checking fasm2 output. |

None of these covers:
- `vmgexit` (`F3 0F 01 D9`): both decoders report it as `vmmcall`, and neither LLVM version accepts it.
- `mcommit` assembly. Zydis does decode `F3 0F 01 FA` as `mcommit`.
- `rmpread`
- Cyrix and other historical sets

Use the APM/SDM for those.

### 6.2 Data sources: `C:\git\x86-cvb\data`

`x86-cvb` mirrors upstream instruction databases. Each has a pinned commit
or SHA-256 in `data\sources.lock.json`; refresh them with
`python tools\fetch_sources.py`.

| Source | Location | What it gives iset work |
|--------|----------|-------------------------|
| **XED datafiles** (intelxed/xed) | `data\src\xed\datafiles\<ext>\` | This is the CPUID truth for gating. Each `cpuid*.xed.txt` maps an ISA set to `name.leaf.subleaf.reg.bit`, e.g. `XED_ISA_SET_LZCNT: lzcnt.80000001.0.ecx.5`. It also lists **multi-bit requirements**, for example: `AMX_FP16` = `amx_tiles.7.0.edx.24` + `amx_fp16.7.1.eax.21`; `SM3` = `sm3.7.1.eax.1` + `avx.1.0.ecx.28`; `PTWRITE` = Intel PT + `14.0.ebx.4`; AVX512_FP16 128/256 = `avx512_fp16` + `avx512vl`. The `*-isa.xed.txt` files hold encoding patterns per form. There are about 100 extension folders, including `apx-f`, `avx10-*`, `amd`, `via` and `evex-map5-6`. |
| **zydis-db** | `data\src\zydis-db\datafiles\instructions.json` | One record per encodable form: mnemonic, encoding (legacy/VEX/EVEX/XOP/3DNow/MVEX), map, opcode, prefixes, operand specs, ISA set/extension, CPU flags and exception class. It is the best source for generating test cases that cover every form. |
| **Zydis** source | `data\src\zydis` | The decoder and encoder library that ZydisInfo is built from. Its tests are a corpus of tricky encodings. |
| **sail-x86-from-acl2** | `data\src\sail-x86-from-acl2` | Formal semantics. It is not needed for encoding, but it is the reference when an instruction's side effects matter. |
| `x86.csv` (golang/arch) | `data\x86.csv` | A compact table of Intel syntax, Go syntax, encoding and CPUID feature. It is easy to `grep` for one mnemonic. |
| `uops-instructions.xml` (uops.info) | `data\uops-instructions.xml` | Measured latency and throughput per form and microarchitecture, plus an IACA-style ISA extension label. It is large (about 140 MB). |

How each source feeds the work:
1. **§4 table lines:** take the bit from XED's `cpuid*.xed.txt`. Where XED
   lists several bits, the gate checks all of them. Where XED has `n/a`
   (SVM, TDX, INVLPGB), cite the APM/SDM in the table comment.
2. **Test cases:** generate every form from zydis-db for the extension's
   ISA set. Assemble each form with fasm2 and with `llvm-mc` and compare
   the bytes. Then decode fasm2's bytes with `xed -d` and `ZydisInfo`, and
   check that the ISA set matches the gate.
3. **New gaps:** compare XED's extension folders with `include/iset`
   to find extensions this document doesn't yet list, such as `movrs`,
   `msr-imm`, `pbndkb`, `amx-dmr`, `sm4-evex` and `avx10-2`.

---

## 7. Order of work

1. **Gating repairs:** done, with tests: G1–G5, and G6–G11 found while
   implementing (§2.4). Behaviour changes to record in the changelog: G2
   (`use BMI1` no longer allows `lzcnt`), G6 (unmasked VNNI/IFMA xmm/ymm
   become VEX under `use everything`), G7 (BMI no longer needs AVX), and
   `pi2fw` moving to 3DNowExt.
2. **`svm.inc`, `sev.inc`, `amd.inc`**: done, with tests. They are
   GPR-only and established the CALM style and the implicit-operand
   helpers (§3.7).
3. **`vnni.inc`, `ifma.inc`, `bf16.inc`, `sha512.inc`, `sm.inc`,
   `vp2intersect.inc`, AMX-FP16/COMPLEX**: done, with tests. AMX also
   needs AVX, through the VEX encoder.
4. **Intel scalar files:** done, with tests: `cache.inc`, `system.inc`
   (and `vmfunc` in `vmx.inc`), `cmpccxadd.inc`, `rao.inc`, `msr.inc`,
   `fred.inc`, `tdx.inc`, plus fix G7. Modes follow XED's non-APX
   patterns. Not implemented: the APX EVEX forms (map 4) of RAO-INT,
   USER_MSR and CMPccXADD, and the MSR-IMM `rdmsr`/`wrmsrns` immediates.
5. **`avx512_fp16.inc`**: arithmetic, FMA, complex, compare, classify and
   moves, and all 34 conversions done, with tests, as thin macros over
   avx.inc's shape helpers (the helpers are macros; converting them to
   CALM is a separate job). Note: for `{sae}` with register operands fasm2 writes
   EVEX.L'L = 512 where llvm-mc writes 00; the CPU ignores L'L there.
6. `sse4a.inc` and the `3dnow.inc` additions: done, with tests (`use AMD64`
   now includes 3DNowExt). Then the XOP encoder change and
   `xop.inc`, `fma4.inc`, `tbm.inc`, `lwp.inc` (bextr's TBM form in place in
   `bmi.inc`): done, with tests.
7. **Historical and vendor files** (§5.7): `padlock.inc` and 4FMAPS done;
   `cyrix.inc`/`undoc.inc` blocked on an authoritative reference.
8. **APX / AVX10.2**, as a separate design (§5.8). APX: done
   ([`apx-plan.md`](apx-plan.md)). AVX10.2: not started.

**Status (end of this pass).** Every addition in §5 that has a reference
is implemented and tested: `tests/iset/_run.ps1` runs every source there
(53 on 2026-11-03), each of which fails on the tree before its change.
APX is done since (its own plan, [`apx-plan.md`](apx-plan.md)). Still open:
- `cyrix.inc`, `undoc.inc`: no reference available locally (§5.7);
- AVX10.2: deferred (§5.8);
- RMPQUERY and ENCLV bits are marked *verify* in §4 (no XED entry);
- not yet planned, from XED's folder list (§6.2): MOVRS, MSR-IMM,
  PBNDKB, AMX-DMR, SM4-EVEX;
- CALM versions of avx.inc's AVX-512 shape helpers (FP16 uses the macros);
- `monitor`/`mwait` could move to the implicit-operand helpers (§5.5).
