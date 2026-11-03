# Rebuilding the x86 encoder from XED's data

Status: survey done, design proposed, nothing generated yet. This is the
post-APX reengineering that [`apx-plan.md`](apx-plan.md) §5 deferred.
The tools are in `tools/x86db/` (`xed_peek.py ICLASS [text]` prints XED's blocks); numbers below are from XED v2026.08.23
(2026-11-03).

## 1. Why, and the policy that shapes it

fasm2's x86 encoder is ~40 hand-written `iset/*.inc` files. Every defect
found during APX (G15–G22) was a hand-transcription error: a wrong map
(G22), a missing W bit (G21), a reused list (G16), a missing operand check
(G15, G19). Each was invisible until a differential sweep found it. The
data that would have prevented them already exists, machine-readable, in
XED.

The policy decided during APX becomes the design's requirements:

| Policy | Requirement on the design |
|---|---|
| Authority: Intel docs > XED > LLVM > GCC/binutils > others; XED is the programmatic source of truth | Encodings come from XED's tables, not from transcription. Every generated row is verified by decoding it with XED. |
| fasm2 emits no #UD instruction | XED's own constraints (`GPR64_N_NORSP`, `mode64`, `MOD=3`, `norex2_prefix`, `nolock_prefix`, `NO_REG_MATCH` ...) become checks in the generated encoder, not hand-written exceptions. |
| A one-line encoding request is honoured or refused | Each row knows its encoding space (legacy/REX2/VEX/EVEX/XOP), so `{vex3}`/`{evex}`/`{rex2}` select among rows instead of being checked after the fact (`x86.select_check` can then go). |
| fasm syntax first: options after the mnemonic (`{nf}`, `{zu}`, `{dfv=}`), selection before it | The operand parser and option syntax stay fasm2's; only the form table is generated. |
| Cost is measured (apx-plan §5.1: APX added 8–10%) | Generated code must not be slower than today; the representation is chosen by measurement (§5, P1). |

## 2. The data

XED's build merges `datafiles/*/` (one directory per extension, 117 of
them) into `obj-wkit/dgen/all-enc-instructions.txt`: one block per
instruction, each with one or more `PATTERN`/`OPERANDS`/`IFORM` triples:

```
ICLASS:      PUSH2
EXTENSION:   APXEVEX
ISA_SET:     APX_F_N3
ATTRIBUTES:  APX_NDD
PATTERN:     EVV 0xFF VNP MAP4 MOD[0b11] MOD=3 UBIT=1 REG[0b110] RM[nnn] ND=1 NF=0 W0 VL128 mode64 ZEROING=0 EVAPX() FORCE64()
OPERANDS:    REG0=GPR64_N_NORSP():r:q:u64 REG1=GPR64_B_NORSP():r:q:u64 REG2=XED_REG_STACKPUSH:rw:SUPP:spw2
IFORM:       PUSH2_GPR64u64_GPR64u64_APX_N3
```

The #UD rule for push2 (no rsp) is the nonterminal `GPR64_*_NORSP`: the
policy is in the data. `idata.txt` maps iform → ISA set, `cdata.txt` chip →
ISA sets, `dgen/all-cpuid.txt` ISA set → CPUID bits (the `require` gates).

`tools/x86db/xed_extract.py` turns the table into a flat record list
(`build/xed.json`, not checked in): **11,013 forms, 1,995 instruction
classes, 5,855 iforms, 253 ISA sets**; by space: EVEX 6,843, legacy 2,256,
VEX 1,718, XOP 196. Pattern vocabulary: 207 distinct tokens.

### 2.1 How far it coalesces

Taken whole (`x86db_stats.py`), patterns barely coalesce: 5,392
(pattern, operand-list) templates for 11,013 forms. They mix what varies per
instruction with the structure of the encoding. Factored
(`x86db_fold.py`), each form becomes a **row** (mnemonic + parameters:
space, map, pp, opcode, W, L, ModRM digit, tuple type, ISA set,
constraints) and a **template** (operand roles and encoding structure):

| Fold | Rows | Templates | Templates for 90% of forms |
|---|---:|---:|---:|
| none (whole patterns) | 11,013 | 5,392 | 4,291 |
| split parameters from structure; operands as role:class | 11,009 | 606 | 227 |
| register and memory form → one r/m row | 6,507 | 297 | 100 |
| xmm/ymm/zmm (L) → one row | 5,043 | 243 | 72 |
| GPR width / W → one row | 4,944 | 204 | 55 |

Most common templates after all folds (forms covered):

```
1914  EVEX  R:VEC K:MASK N:VEC BM:RM        vaddpd zmm1{k1}, zmm2, zmm3/m512/bcst
 970  EVEX  R:VEC K:MASK BM:RM              vcvtdq2pd
 841  VEX   R:VEC N:VEC BM:RM               vaddpd ymm1, ymm2, ymm3/m256
 415  LEG   R:VEC BM:RM                     movhlps / SSE
 362  EVEX  apx BM:RM R:GPR                 adc (APX map 4)
 287  LEG   (no operands)                   push imm / implicit
 270  LEG   digit BM:RM                     fadd m32 (/digit)
```

Folds still open, each visible in the data: operand order as a row
parameter (`R,BM` vs `BM,R` are one template), the 2,318 APX map-4 forms
(they repeat legacy rows: a promotion rule instead of rows, as
`APX.promote` does now), the ~730 `{er}`/`{sae}` forms (`FIX_ROUND_LEN*`,
`BCRC=1`, `SAE()`: a row flag), and XED's split classes (`ADD_LOCK`,
`REP_MOVSB`: one mnemonic with a prefix flag). Estimate after those:
**~3,500 rows, ~120 templates**.

### 2.2 Coverage against fasm2 (`x86db_cover.py`)

XED 1,942 mnemonics (after `_LOCK`/`REP_` folding); fasm2 2,233 names;
1,763 in both.

- **XED only, 179**: what generation would add for free. AVX10.2 (~110:
  BF16 arithmetic, `vminmax*`, saturating converts `vcvtt*s`, FP8
  `vcvt*bf8/hf8`, `vmovrs*`), AMX FP8/TF32/MOVRS/row moves (`tdpbf8ps`,
  `tcvtrow*`, `tileloaddrs`), ACE, MOVRS, MSR-IMM `rdmsr`/`wrmsrns imm`,
  PBNDKB, IBHF, `vpermi2ps` etc. Some are XED spellings to map, not gaps
  (`nop2`–`nop9`, `call_near`, `cmpsd_xmm`, `pcmpestri64`, `bextr_xop`).
- **fasm2 only**: syntax XED does not model, so it stays hand-written:
  condition-code synonyms (`je`/`jz`, `cmovnae`, `setpe`), compare
  predicates (`vcmpeq_ospd`, `cmpltss`, `vpcomeqb`), size-suffixed forms
  (`pushw`, `retfq`, `fsavew`, `loopd`), prefixes as instructions (`rep`,
  `lock`, `cs`, `xacquire`), and undocumented/legacy instructions
  (`loadall`, `ibts`, `xbts`, `setalc`, `icebp`).

## 3. Design

```
XED datafiles ──xed_extract──▶ records ──normalize──▶ rows + templates ──emit──▶ include/x86/*.inc
                                  ▲            ▲                                      │
                       mnemonic map, alias    fold rules                      verify each row:
                       layer, fasm2 syntax                                     XED decode = iform
```

### 3.1 Three layers in the generated include

1. **Rows: the flat table.** One line per row, all parameters packed into
   integers, so a mnemonic is data, not code:

   ```
   x86.row vaddpd, EVEX_RVM_K, 58h, MAP_0F or PP_66 or W1 or L_ALL or BCST or TUPLE_FV, AVX512F
   x86.row vaddpd, VEX_RVM,    58h, MAP_0F or PP_66 or L_128_256, AVX
   x86.row vaddpd, LEG_RM,     58h, MAP_0F or PP_66, SSE2       ; addpd
   ```

   The generator chooses the bit layout from the measured field ranges
   (space 3 bits, map 4, pp 2, W 2, L 3, digit 4, tuple 5, constraint
   flags). ~3,500 rows; an ISA set is an index into the gate table.

2. **Templates: ~120 encoder routines**, one per (encoding space,
   operand roles), each a plain calminstruction taking the packed row and
   the parsed operands. 55 of them cover 90% of forms.

3. **Support commands: `calminstruction calminstruction?.x86.*`.** CALM
   commands run when a calminstruction that uses them is *defined*, and
   emit CALM code into it (as `asm`, `init` and `xcall` in `xcalm.inc` do).
   So `x86.classify`, `x86.match_row`, `x86.emit_modrm`,
   `x86.emit_evex` ... are inlined code generators, not calls: shared
   source, no call overhead, and each template gets code specialized to
   its row bits where they are constants. This is where the speed of the
   flat design comes from.

### 3.2 Operand classification

One classifier turns each operand into a class bitmask: register class
and size (gpr8 ... zmm, k, bnd, tmm, sreg, cr/dr), r16–r31 (EGPR),
memory size or unsized, VSIB, broadcast, immediate width, `{k}`, `{z}`,
`{er}`/`{sae}`. A row stores the expected mask per operand role; matching
a row is one AND per operand. The current parsers (`x86.parse_operand`,
`AVX_512.parse_operand`, `MPX.parse_operand`, the VSIB parser) become one,
which also removes the class of bug that G20 was (operand state left
undefined by the wrong parser).

### 3.3 Choosing among valid encodings

XED lists every valid encoding; an assembler must pick one,
deterministically. Rule, in order: the row the line requested (`{vex3}`,
`{evex}`, `{rex2}`, `{nf}`); otherwise the shortest; on a tie the one
llvm-mc and GNU as write (e.g. `01 /r` over `03 /r` for `add r, r`). The
rule is a generator decision recorded per mnemonic group, so it is tested,
not implied by row order.

Cases already measured (`tests/IEV/pass/boundaries.ref`, against llvm-mc): fasm2
is shorter in 20 address/moffs/segment forms and must stay so. Open: the
store-direction opcode for a register move when it allows VEX2
(`vmovaps xmm0,xmm8`, 1 byte), `90` for `xchg eax,eax` in 32-bit code and
for `xchg rax,rax` (1 byte each). Not to take from llvm-mc: swapping the
sources of a "commutative" floating-point operation to reach VEX2, which
changes which NaN the result carries.

### 3.4 Hand-written parts that stay

- The alias layer (2.2: condition-code synonyms, predicates, size
  suffixes, prefixes) as a small table the generator also reads.
- fasm2 syntax: `[base, index]` MIB/AMX, options after the mnemonic,
  `use`/`require` settings.
- Jump relaxation (short/near across passes) and size inference/defaults.
- Instructions XED does not have (undocumented, VIA PadLock if absent).
- AVX10 gating, once AVX10.2 decides what "EVEX enabled" means.

## 4. Verification (XED as the source of truth)

Each generated row yields canonical operand instances (lowest and edge
registers r16/r20/r21/r31, each memory form, each immediate width). The
generator assembles them with fasm2, decodes with `_xedcheck` and
requires **the same iform** back, not only "decodable". That is a
stronger check than the APX sweep: it catches G15/G16b-style wrong-but-
valid encodings, which XED decoding alone cannot. llvm-mc and GNU as
remain differential checks, and the existing tests and sweep must stay
byte-identical where today's output is right.

## 5. Phases

| Phase | Work | Done when |
|---|---|---|
| P0 | Data pipeline (`xed_extract`, `x86db_stats`, `x86db_fold`, `x86db_cover`) | **done** (this survey) |
| P1 | Remaining folds (operand order, APX promotion rule, `{er}`/`{sae}`, `_LOCK`/`REP_`); mnemonic map; bit layout; emit rows for one slice (AVX-512F arithmetic, or the BMI/APX GPR set) with its templates and `x86.*` commands. **Measure** include time and per-instruction time against today on fasmg's self-build | slice byte-identical with today on the sweep and tests; cost ≤ today, or the representation changes before going on |
| P2 | Row verifier (§4) for the slice; then for all of XED | every row round-trips to its iform |
| P3 | Replace `iset/*.inc` slice by slice; old files stay as the reference until each slice matches | test suite, APX sweep, LLVM corpus and XED check unchanged |
| P4 | Gaps closed by generation: AVX10.2, AMX FP8/TF32/MOVRS, MSR-IMM, PBNDKB | rows verified; tests from the LLVM corpus |
| P5 | Settle compatibility: which `x86.*` / `AVX_512.*` names user macros may rely on; upstream report of the approach | decided and documented |

## 6. Decisions for later

1. Generated output checked in (reviewable diffs, no XED needed to build
   fasm2) or generated at build time. Recommendation: checked in, with the
   XED version in its header.
2. Granularity of `use`/`require` gates: XED's 253 ISA sets vs fasm2's
   extension names; the mapping table is part of P1.
3. Whether templates are expanded per mnemonic at definition time (fast
   per instruction, slower include) or shared and called (smaller, one
   call per instruction). P1 measures both.

## 7. Change log

- 2026-11-03: survey: XED v2026.08.23 extracted (11,013 forms); folding
  measured (4,944 rows / 204 templates); coverage against fasm2 (179
  XED-only mnemonics). Plan written.
