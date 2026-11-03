# APX plan: steps to pivot from

Intel APX (Advanced Performance Extensions) adds 16 more general-purpose
registers, the REX2 prefix, and EVEX forms of legacy GPR instructions. This
is a working plan, not a specification. Each phase ends with a **discovery
point**: what we expect to learn there and which later steps it may change.
Update this file as answers come in, and keep the history short.
`EXTENSIONS.md` §5.8 points here.

Context (2026-11): processors with APX are not yet public; the first parts
are expected around the end of the year. Encodings can be checked now, but
running the code needs emulation (§2).

---

## 1. Ground truth so far

**Encoding authority, in order** (user decision, revised 2026-11-03).
When sources disagree, the higher one wins, and the disagreement is noted
in the test source:
1. Intel's APX documents (`C:\git\_DOC\APX\`, below).
2. XED, the latest: Intel's decoder, with a history of accuracy and
   independent of LLVM. The programmatic source of truth: what XED
   refuses is #UD, and fasm2 does not emit it (`_apx_xed.py`).
3. LLVM source: the X86 target and its `llvm/test/MC/X86/apx/` tests,
   largely written by Intel engineers. `*-intel.s` (63 files) are an
   Intel-syntax corpus with expected bytes, one file per mnemonic group.
   It encodes forms that #UD (§5.3), so it is the byte reference for
   valid forms only.
4. GCC/binutils (GNU as, in WSL): an independent encoder with a history
   of accuracy, to compare against. Binutils 2.47 is built for this
   (table below); `_apx_gas.py` runs the comparison.
5. Others: Zydis/zydis-db, NASM, MSVC output.

Execution would outrank all of these on real hardware. Emulators
(QEMU, SDE) are not trusted as a source of truth; XED stands in until
hardware is available.

LLVM state: the build in `build-native` is at 4d58566 (2026-07-11). The
checkout was updated to fc16f15 (2026-11-02); the 18k commits between
contain APX **codegen** changes only (PPX CSR cost, SHLD/SHRD NF
commuting, BLSI/BLSMSK i8 with EGPR, CCMP miscompile) and one
Intel-syntax print fix (`vscatterpf0dps` mask position). No APX encoding
changed, so the existing build stays a valid reference.

**References that already encode/decode APX** (checked):

| Tool | APX status |
|------|------------|
| LLVM 23 `llvm-mc` (`C:\git\llvm-project\build-native\bin`) | Encodes every form tried with `-triple=x86_64 --show-encoding -mattr=+egpr,+push2pop2,+ppx,+ndd,+ccmp,+nf,+cf,+zu` (plus `+avx512f`, `+bmi` for those forms). |
| Intel oneAPI 2026.2 clang (build 20261001; checked 2026-11-03, then uninstalled) | Intel's shipping LLVM fork. It has no `llvm-mc`, but `clang -cc1as -triple x86_64 -filetype asm -show-encoding -output-asm-variant=1 -target-feature +egpr …` prints the same encodings (the variant option takes `=`). It has every APX feature (egpr, push2pop2, ppx, ndd, ccmp, nf, cf, zu, jmpabs) plus `prefer-ndd-mem`, `prefer-setzucc` and `inline-asm-use-gpr32`. `-march=diamondrapids` and `novalake` define `__APX_F__` and AVX10.2. `core-apx` defines the separate APX macros but not `__APX_F__` or `__JMPABS__`. 2026-11-03: it reproduces all 6,680 encodings in LLVM's `test/MC/X86/apx` (Intel and AT&T syntax), and the full sweep is identical to upstream llvm-mc's: 34,776 same, the same 963 llvm-only forms, 0 failures. So Intel's product and upstream LLVM agree, and this is no independent second reference. |
| XED v2025.12.14; v2026.08.23 at `C:\git\_Intel\xed` (with mbuild beside it) | Decodes REX2 and EVEX map 4 (`xed -64 -d D54801C0` → ADD). `data\src\xed\datafiles\apx-f\` holds the patterns and `cpuid*.xed.txt` the gates. Build: run `python mfile.py --host-cpu=x86-64 --build-dir=obj-wkit examples` from a vcvars64 shell (25 s); the kit is in `obj-wkit\wkit`. XED is Intel's and independent of LLVM. It **refuses** the forms the spec says #UD, with a reason instead of a decode: `BAD_REGISTER` (push2/pop2 with rsp), `BAD_REG_MATCH` (`pop2 rax, rax`), `GENERAL_ERROR` (REX2 before 0F 38 or on xsave/xrstor), `BAD_LOCK_PREFIX`. `tests/iset/_xedcheck.c` (built into the kit's `bin`) decodes many lines in one process for `_apx_xed.py`. |
| ZydisInfo | Decodes REX2 (`D5 48 01 C0` → `add rax, r16`). |
| zydis-db (`C:\git\zyantific\zydis-db\datafiles`, 2026-06) | APX forms are in `instructions.json`, for generating test cases. |
| NASM 3.02rc7 (`C:\git\nasm\nasm.exe`) | Intel-syntax reference: assembles REX2, NDD, `{nf}` (before or after the mnemonic), `push2`, `ccmp {dfv=…}`, `{zu} setb`. Doc: `doc\syntax.src` §`apx_*`. Rejects Intel's `setzuCC` mnemonic. |
| MSVC 14.51 `cl /feature:APX` | Accepted; emits APX when it helps (a trivial function did not). A source of real compiler output (`/FA`) to assemble and compare once Phase 3 exists. |
| GNU binutils 2.47 (`as`, `objdump`; WSL Ubuntu 24.04, `~/opt/binutils-latest`) | Built 2026-11-03 from the signed release tarball (`binutils-2.47.tar.xz`, Nick Clifton's key 13FCEF89DD9E3C4F; `as --version`: 2.47.20260726) with `--disable-nls --disable-werror` and gas/binutils only; the distro's 2.42 stays as `/usr/bin/as`. It has all of APX_F: REX2, NDD, `{nf}`, `{evex}`, push2/pop2(p), pushp/popp, ccmp/ctest with `{dfv=}`, cfcmov, `setzu<cc>`, `imulzu`, jmpabs, and the promoted map 2/4 forms. Independent of LLVM (its own encoder, `gas/config/tc-i386.c`). `tests/iset/_apx_gas.py` compares it with fasm2 on the sweep and LLVM corpus forms: 2026-11-03, 299,295 forms, **36,352 same, 1,552 DIFF, 737 fasm-only, 14,704 gas-only**, 245,950 both refuse. Every DIFF and fasm-only form is explained (`REVIEWED`, §5.3). It found G23–G25. |
| YASM, ASMC | No APX (checked: no REX2/EGPR in the trees). |

**Documents** (`C:\git\_DOC\APX\`):
- `355828-009-intel-apx-arch-spec.pdf`: the architecture spec (encoding, REX2 reserved rows, which instructions take NDD/NF/ZU).
- `356112-intel-apx-sw-enabling.pdf`: OS/compiler enabling (XSAVE state, CPUID).
- `817241-apx-asm-syntax.pdf`: Intel's assembly-syntax recommendations (summarised in §1.1).

### 1.1 Syntax: fasm's choices first, Intel's recommendations next

User decision: align with fasm/fasmg x86 syntax; learn from the processor
manuals and Intel's syntax recommendations (817241) where fasm has no
precedent.

**Placement rule (decided).** Braces have two roles, kept apart by
position:
- **Before the mnemonic:** encoding selection only, through the existing
  `{settings} instr` hook: `{rex2}`, `{evex}`. These never change what
  the instruction does.
- **After the mnemonic:** instruction options that change what it does:
  `{nf}`, `{zu}`, `{dfv=…}`. The instruction's own CALM code splits them
  off the operand text. Start with after-only; Intel's and NASM's
  prefix spelling (`{nf} add`) is not accepted, and can be added later if
  wanted.

Intel's recommendations, with fasm's position:

| Feature | Intel (817241) | NASM 3.02 | fasm2 plan |
|---------|----------------|-----------|------------|
| EGPR names | `r16`–`r31`, `r16d/w/b` | same | same |
| NDD | destination first: `sub eax, [rbx], ecx` | same | same |
| NF | `{nf}` pseudo-prefix (preferred); `addnf` suffix if no pseudo-prefixes | `{nf} add`, `add {nf}`, `add{nf}` | `add {nf} rax, rbx` (after the mnemonic only), split off before the first operand is examined. No `addnf` names. |
| ZU | `setzuCC`, `imulzu` mnemonics | `{zu}` pseudo-prefix; `setb eax/rax` | `setb {zu} al`, `imul {zu} ax, [rbx], 0xab`: same rule as NF. No `setzuCC`/`imulzu` names. |
| PUSH2/POP2 | `push2 rax, rbx` pushes RAX first; `pop2 rbx, rax` | same (first operand → `vvvv`) | same |
| PPX | `pushp`, `popp`, `push2p`, `pop2p` | same | same |
| CCMP/CTEST | `ccmpCC {dfv=sf,cf,of} …`, SCC `t`/`f` for 1010/1011; immediate form `ccmpnz 13, …` | same, comma after `{dfv=}` optional, `pf` alias for `cf` | `{dfv=…}` set, and the immediate form |
| Prefix selection | `{rex2}`, `{evex}` | same | `{rex2} add rax, rbx`, before the mnemonic (settings hook); `{evex}` already exists (`vexsize = 4`) |

The NF list (Intel 817241 §4): `inc dec neg add sub and or xor sal sar shl
shr rol ror shld shrd imul idiv mul div lzcnt tzcnt popcnt andn bextr blsi
blsmsk blsr bzhi`. ADC, SBB, RCL, RCR and NOT take NDD but **not** NF.

Test note: llvm-mc picks opcode `01` for `add r, r`, NASM `03`. Both are
valid. fasm2 already uses `01`, so compare against llvm-mc bytes, and use
NASM as a second check of operand order and syntax, not of exact bytes.

**Reference encodings** (llvm-mc 23, AT&T operand order):

| Source | Bytes | Shows |
|--------|-------|-------|
| `add %r16,%rax` | `D5 48 01 C0` | REX2 (`D5` + payload `M0 R4 X4 B4 W R3 X3 B3`) |
| `add %rbx,%rax,%r17` | `62 F4 F4 10 01 D8` | NDD: EVEX map 4, destination in `vvvv`/V4, ND bit set |
| `{nf} add %rbx,%rax` | `62 F4 FC 0C 01 D8` | NF bit (P2 bit 2) |
| `push2 %rbx,%r16` | `62 F4 7C 10 FF F3` | PUSH2: map 4 FF /6, ND = 1 |
| `pushp %rbx` | `D5 08 53` | PPX hint = REX2.W on push/pop |
| `jmpabs $imm64` | `D5 00 A1 imm64` | REX2 map 0, opcode A1 |
| `ccmpb {dfv=of,cf} %rax,%rbx` | `62 F4 CC 02 39 C3` | DFV in `vvvv`, condition in P2 low bits |
| `setzub %r16b` | `62 FC 7F 18 42 C0` | ZU form of SETcc |
| `andn %r16,%r17,%r18` | `62 EA F4 00 F2 D0` | promoted VEX-GPR (map 2) |
| `vpaddd (%r16),%zmm1,%zmm2` | `62 F9 75 48 FE 10` | EGPR in vector addressing: B4 = P0 bit 3 |
| `popcnt %r16,%rax` | `F3 D5 98 B8 C0` | REX2 with map 1 (M0 = 1), mandatory prefix before REX2 |
| `{nf} popcnt %rbx,%rax` | `62 F4 FC 0C 88 C3` | map 4 form of a map-1 opcode |

**The hazard that orders this plan.** Defining r16 today, with
`element r16? : x86.r64 + 16`, makes existing encoders produce **wrong bytes
with no error**, because they mask the register number to 3 or 4 bits:

| Source | fasm2 today | Actually is |
|--------|-------------|-------------|
| `add rax, r16` | `48 01 C0` | `add rax, rax` |
| `push r16` | `60` | `pusha` |
| `bswap r16` | `48 0F C8` | `bswap rax` |
| `vpaddd zmm2, zmm1, [r16]` | wrong base register | — |

fasmg elements are global the moment they exist (defined-once rule), so
there is no "only inside apx.inc" for register names. Refusal must be in
place before the names are.

**MPX conflict.** QEMU warns that `apxf` conflicts with `mpx`: APX's
extended-GPR state reuses the XSAVE area that MPX's bound registers had.
The CPUs that have APX have no MPX. This does not touch encoding (MPX's
opcodes stay as they are), but a test or CPUID check program should not
expect both.

---

## 2. Executing APX code before hardware

*Superseded 2026-11-03:* emulators are not trusted as a source of truth
(authority order, §1); XED stands in until hardware exists, and no
execution path was chosen. The table records what was found.

Encoding checks (bytes vs. llvm-mc, decode with XED/Zydis) need nothing
more. Execution tests do: flag results of NF forms, CCMP flag
substitution, PUSH2/POP2 alignment faults, upper-32 zeroing of ZU forms.

| Option | State here | Notes |
|--------|------------|-------|
| **QEMU** 11.1-rc2 (`C:\third_party\qemu`) | Has `apxf` and `apx-nci-ndd-nf` CPUID flags and a `DiamondRapids` model. `-accel tcg -cpu max,+apxf` is accepted with no "TCG doesn't support" warning (only the MPX conflict above). WHPX: "host doesn't support". | *Verify:* that TCG actually executes REX2/map 4, by booting a minimal guest (or a UEFI app) that runs one APX instruction and reports. If it does, this is the main execution path: system emulation, so it also covers kernel-mode code. |
| **Intel SDE** | Not installed. | Intel's emulator for future ISA; runs user-mode Windows/Linux programs directly, which suits fasm2's PE output. *Verify:* current version and its APX/Diamond Rapids option. Download from intel.com (license click-through), so it needs the user. |
| GDB / binutils | Not checked. | objdump/gas support APX in recent binutils; useful only as another decoder. |

**Discovery point E.** Pick one execution path before Phase 3 (the first
phase whose semantics, not just bytes, can be wrong). Until then, tests are
byte tests only. *Outcome:* none picked; all APX tests are byte tests,
checked against llvm-mc and decoded by XED.

---

## 3. Phases

Each phase is in a separate commit with its tests, and each lists what may
make us change direction.

### Phase 0 — refuse extended registers everywhere

Goal: after this phase, **no** instruction can silently mis-encode r16–r31,
whether or not APX is enabled.

1. Add the elements in `x86-2.inc` next to r8–r15:
   `r16`–`r31`, `r16d`–`r31d`, `r16w`–`r31w`, `r16b`–`r31b`.
2. Add a central refusal where register numbers enter an encoding:
   `x86.store_instruction` (ModRM reg/rm, SIB base/index),
   `x86.store_operand_prefix`, and `AVX_512.store_instruction` (VEX and
   EVEX register and address fields).
3. Find and fix the sites that emit register bits directly
   (`emit 1, base + rm and 111b` and similar: `push`/`pop`, `bswap`,
   `mov r, imm`, `xchg rax, r`, `inc/dec` in 16/32-bit code, …). Search
   for `and 111b` and `and 7` across `x86-2.inc` and `include/iset`.
4. Message: `'APX_F or higher required'` while APX is off, so the error
   already reads as the gate it will become.

Tests (`tests/iset/apx_refuse.asm` + a sweep):
- the four hazard cases above must fail;
- **sweep**: a script (sibling of `_dupnames.py`) that, for every
  instruction name defined after `use everything`, tries the register
  forms with r16/r16d/r16w/r16b and a `[r16]` / `[rax+r16*2]` address,
  and fails if any assembles. It is re-run after every phase, with an
  allowlist that grows only as phases add real APX forms.

**Discovery point 0.** How many direct-emit sites are there? If there are
many, a register-validation helper (`x86.check_register`) called by each is
better than fixing them one by one. Also measure the cost of the check on a
large source (`tests` corpus, fasm2 self-build). The check is a compare on
every register operand.

**Phase 0 result (2026-11-02, done).** The refusal went into the operand
parsers, not the encoders (steps 2–3 above were the guess before looking):
every GPR number, as a register or as an address base/index, enters
through one of three parsers, and the direct-emit sites (`50h+rm`,
`B8h+rm`, `90h+rm`, `0Fh,C8h+rm`) all get `rm` from `x86.parse_operand`.
- `x86.parse_operand` (registers and addresses; also reached by
  `AVX_512.parse_operand`, `AMX.parse_operand`, `MMX`/`SSE` parsers and
  the jump-operand parser), `AVX_512.parse_vsib_operand` (gather/scatter
  base) and `MPX.parse_sib_operand` (bndldx/bndstx and AMX's
  `[base, index]` form; MPX and APX never coexist).
- Message: `extended register not supported: <operand>`, not
  `APX_F or higher required`. That message would be wrong under
  `use everything` and stays wrong for instructions APX cannot encode;
  this one is true in every phase.
- Elements: `r16`–`r31`, `r16d`, `r16w`, `r16b` and `r16l` (as r8l–r15l).
- Tests: `tests/iset/apx_refuse.asm` (20 refused forms across all three
  parsers and both modes; r8–r15 neighbours keep llvm's bytes) and
  `tests/iset/_apx_sweep.py`: 2,073 mnemonics × 117 operand patterns =
  242,541 lines, 0 leaks, 76 s. A control run confirms it reports lines
  that assemble.
- Cost: fasmg self-build 368 → 378 ms, fasmgw 871 → 884 ms (best of 3),
  identical output bytes.

**What this changes for Phase 1** *(superseded: see the Phase 1 result).*
The refusal is in the parser, so
lifting it there would admit r16+ to every encoder at once, the hazard
again. Phase 1 instead keeps the parser as the single place that decides,
and makes it ask: the parser accepts 16–31 only when the calling encoder
has said it can store them (a flag the caller sets before parsing, cleared
by the parser), otherwise it refuses as now. `x86.store_instruction`'s
callers then opt in group by group, each with tests, and the sweep's
ALLOWED set grows by exactly those forms. The direct-emit sites (`push`,
`pop`, `bswap`, `mov r, imm`, `xchg`) need their own REX2 path when they
opt in. The compact `push rax rbx` register list (`x86.compact`) needs
r16–r31 then too.

### Phase 1 — REX2 for legacy maps 0 and 1

1. Gate: add `APX_F` to the table: 7.1:EDX[21], window 192 (where
   USER_MSR etc. already live). 64-bit mode only.
2. In the central encoder: when any register field is ≥ 16, emit `D5` +
   payload instead of REX. Payload bit M0 selects map 0 or map 1 (0F).
   Mandatory prefixes (66/F2/F3) stay before it.
3. Refuse REX2 where it is undefined: map 2/3 (0F 38, 0F 3A), and the
   opcode rows REX2 reserves (70–7F, A0–A3 except `jmpabs`, E0–E3, map 1
   80–8F, …; take the list from the SDM/XED). These keep the Phase 0 error.
4. Optional `{rex2}` setting through the existing `{settings} instr` hook
   to force REX2 for low registers (matches llvm's `{rex2}` pseudo-prefix).

Tests: every ALU/mov/shift/test form with each of r16–r31 in reg, rm,
base and index positions vs. llvm-mc; 8-bit `r16b`–`r31b` combined with
`ah`–`bh` must be refused (as `spl` with `ah` is); 32-bit mode refused.

**Discovery point 1.** Does this cover most of the GPR instructions
without touching their definitions? If many macros bypass the central
encoder, revise Phase 0's helper so they call it instead.

**Phase 1 result (2026-11-02, done).**
- *Pivot.* Intel 355828 §3.1.2.1 makes REX2 apply to **every** map 0/1
  instruction with a GPR or memory operand: x87, MMX and SSE memory
  forms and `mov cr/dr` included. The exceptions are XSAVE*/XRSTOR*, maps 2/3
  and the reserved rows (map 0: 4x, 7x, Ax except `jmpabs`, Ex; map 1: 3x,
  8x). A per-instruction opt-in flag would have meant editing hundreds of
  definitions for a rule that belongs to the encoder. So the parser
  accepts and the *encoders* decide, the original design, now with the tool
  that was missing: a byte comparison against llvm-mc instead of a leak
  count.
- Parser: r16–r31 require 64-bit code and APX_F (`x86.require_egpr`), then
  set `egpr` on the operand; a register number also gets the `x86.EGPR`
  marker (400h, beside REX_REQUIRED/REX_FORBIDDEN), so an encoder can tell a
  GPR 16–31 from zmm16–31. Address checks sit where `base`/`index` are
  computed (32/64-bit addressing only).
- `x86.store_instruction`: R4/X4/B4 from bit 4 of each number; REX2 is
  `D5` + payload after the legacy/mandatory prefixes. The map comes from
  the opcode bytes (assembled in `virtual` on the REX2 path only); `0Fh` is
  dropped for map 1; maps 2/3 and reserved rows refuse an extended
  register. `x86.store_operand_prefix` got the same for opcode-register
  forms and a `map` argument (bswap). push/pop now use it instead of their
  own `41h`.
- Refused by encoder: `AVX_512.store_instruction` (VEX, EVEX, XOP, AMX: until
  Phase 5), msr.inc map 7, XSAVE*/XRSTOR*, MPX (every MPX parse path now
  goes through `MPX.parse_operand`).
- `{rex2}` (`rexsize = 2`) is a preference like `{vex3}`: ignored where
  REX2 is not allowed and outside 64-bit code. XSAVE* suspends it.
  *Revised 2026-11-03:* on one line, `{rex2}` (like `{vex2}`, `{vex3}`,
  `{evex}`) is a request and an error where it cannot apply; only
  `use rex2` stays a preference (§5.3).
- **Authority in practice:** llvm-mc writes `{rex2} movbe` as `D5 00 0F 38
  F0` and `{rex2} xsave` as `D5 80 AE 20`; Intel says both #UD. fasm2
  follows Intel; `apx_rex2.asm` records it.
- Tests: `apx_rex2.asm` (68 byte checks from llvm-mc, 19 refusals);
  `_apx_sweep.py` now differential: 14,427 extended-register forms
  byte-identical with llvm-mc, 0 unreviewed differences; 13 REVIEWED
  entries, each the same with r8 (fasm2/llvm syntax or size leniency).
  The forms only llvm-mc has are all NDD/NF (Phase 3), map 2/3 and VEX
  promotion (Phase 4) or EVEX (Phase 5), plus pre-existing fasm2
  strictness (`shl r8` without a count, `call [r8]` unsized).
- Found on the way (not APX): G17 `cmpxchg16b [r8]` executed as
  `cmpxchg8b` (fixed, `cmpxchg16b.asm`); G15 stale imm8 in AVX `*_imm8`
  helpers, G16 nonexistent `rcppd`/`rcpsd`/`rsqrtpd`/`rsqrtsd`, G16b
  `cvtss2si r, mm` (open, EXTENSIONS.md §2.4).
- Cost, Phases 0–1 together against win32: fasmg self-build 367 → 387 ms,
  fasmgw 876 → 922 ms (about 5%, best of 5), identical bytes. The REX2 bit
  computation is behind one test of bit 4; the remaining cost is a few CALM
  checks per instruction and per operand.
- Compact lists (`push rax r16`) are done in Phase 2.

### Phase 2 — new mnemonics: `iset/apx.inc`

CALM, new names only:
- `push2` / `pop2` (and their PPX forms `push2p` / `pop2p`);
- `pushp` / `popp` (REX2.W hint on 50+r / 58+r);
- `jmpabs imm64`.

Header lists the CPUID requirement (APX_F, 64-bit). File is included by
`x86-2.inc` like every other iset file; `use APX_F` is the opt-in.

Tests: bytes vs. llvm-mc; gate off → `APX_F or higher required`; `pop2`
with the same register twice is refused (#UD).

**Phase 2 result (2026-11-02, done).**
- `include/iset/apx.inc`, included last by `x86-2.inc` (Phase 3's layers
  will live there). CALM throughout; gates `bits64` then `APX_F` first.
- push2/pop2(p) emit the 4-byte EVEX directly (register-only, so no
  ModRM/SIB logic is needed): P0 = F4h with B3 inverted at bit 5 and **B4
  not inverted** at bit 3; P1 = W, ~vvvv, U=1; P2 = ND, ~V4. Operand roles
  per spec Table 3.4: first operand in vvvv, pushed/popped first.
- **Authority in practice, again:** llvm-mc encodes `push2 rsp, rax`,
  `push2 rax, rsp` and `pop2 rax, rax`; spec 3.1.3.1.1 says #UD. fasm2
  refuses (`rsp not allowed`, `pop2 registers must differ`).
  `push2 rax, rax` is valid (only pop2 needs two registers).
- pushp/popp force REX2 (the hint is REX2.W) by pushing `x86.rexsize = 2`
  around `x86.store_operand_prefix`, and take register lists like push/pop.
  `x86.compact` now lists r16–r31 (64/32/16-bit names), so `push`, `pop`,
  `pushw`… lists take them too.
- jmpabs: `D5 00 A1` + qword (relocatable through `call qword`).
- Tests: `apx_instructions.asm` (38 byte checks from llvm-mc, 13 refusals);
  the sweep picks the new mnemonics up by itself: 14,497 forms identical,
  0 failures. No measurable cost beyond Phase 1.
- For Phase 3: the map-4 EVEX for NDD/NF needs memory operands, so it
  cannot reuse push2's direct emission. Plan: an `x86.store_instruction`
  variant (or a mode of it) that writes the APX EVEX prefix where it now
  writes REX/REX2, keeping its ModRM/SIB/displacement code.

### Phase 3 — EVEX map 4: NDD and NF forms of existing mnemonics

This is the part that uses fasmg stacking. `apx.inc` defines, for the
promotable group (`add or adc sbb and sub xor` `inc dec neg not`
`shl sal shr sar rol ror rcl rcr shld shrd` `imul mul div idiv`
`popcnt lzcnt tzcnt` `cmovcc` `adcx adox` …; take the list from XED
`apx-f` and check NF/NDD per instruction against §1.1), a **layer** with
the same name:

```
calminstruction add? operands&
	; cheap textual tests first: three operands, or {nf}/{evex} in effect
	...
	jyes	apx
	; not an APX form: "not for us, try the earlier one"
	arrange	operands, =add operands
	assemble operands
	exit
    apx:
	; parse and encode EVEX map 4
end calminstruction
```

Rules from what we learned about fasmg:
- Inside the layer, `add` means the earlier definition. This works across
  macro ↔ CALM, so the older layer can stay as it is.
- The decision before falling back must be **textual** (`match` on the
  operand count, a check of the settings variable), not a full parse: every
  ordinary `add` in every program passes through this layer.
- `apx.inc` is included **last** in `x86-2.inc`, so it stacks over every
  earlier layer of these names (including the G14-style SSE layers).
- Every layer is a reviewed duplicate in `_dupnames.py`. Extend the script
  so a later definition of a name must reference that name in its body
  (i.e., it passes on); a layer that doesn't is the G12/G13/G14 bug.
- New constants are namespaced (`APX.ND`, `APX.NF`, …). A once-defined
  global like `nf` would be visible to (and silently overridden by) user
  code.

Syntax:
- NDD: three operands, `add rax, rbx, rcx` (destination first, Intel order).
- `add {nf} rax, rbx` (decided): after the mnemonic only. The layer's
  first textual `match` splits the brace options (`{nf}`, `{zu}`) off the
  operand text before any operand is examined; their presence is itself
  the cheap test that selects the APX path. Options are per instruction:
  there is no `use nf`.
- `{evex} add rax, rbx`, `{rex2} add rax, rbx`: encoding selection before
  the mnemonic, through the settings hook (`{evex}` is the existing
  `vexsize = 4`; `{rex2}` is new, Phase 1).

Tests: bytes for each form; NF without APX_F refused; NDD in 32-bit mode
refused; `{nf}` on an instruction with no NF form refused; `{nf} add`
(option before the mnemonic) is an error, never silently ignored by the
settings hook; and the
**timing check**: assembly time of a large non-APX source before and after
including the layer.

**Discovery point 3.** If the layer costs too much, alternatives in order:
(a) one shared CALM dispatcher that every layer `call`s, so the match code
is compiled once; (b) put the map-4 path into the existing definitions
directly (change in place instead of stacking); (c) only define the
layer under an `APX` include guard (last resort: it would be a second
scheme, which we are avoiding).

**Phase 3 result (2026-11-02, done).**
- *Mechanism.* One layer per mnemonic (28, one `iterate`) in `apx.inc`.
  Its fast path is two `match`es and a few constant `check`s, then the
  earlier definition by name. A form with `{nf}`, `{evex}` or one operand
  more (NDD) goes to the shared CALM helper `APX.promote`. The helper strips
  `{nf}`, splits off the NDD register (with its size check, and the size
  keyword for an unsized memory source), sets `x86.apx.evex/nd/nf/v`, and
  assembles the legacy form again. All operand logic is the legacy
  definition's own. `x86.store_instruction` writes the EVEX prefix in
  place of REX/REX2 (map 4, with the six map-1 opcode moves) and sets
  `x86.apx.done`; the helper errors if no path did. The layer passes
  straight down while `x86.apx.evex` is set, because the helper's own call
  re-enters it (EXTENSIONS.md §1.3 rule 7).
- *Legacy changes:* the ALU accumulator short forms (04h/05h…) are skipped
  in EVEX mode (no ModRM); the lock/rep/segment prefix instructions record
  their byte in `x86.apx.legacy_prefix`, so EVEX refuses LOCK/REP. That
  variable was first named `x86.apx.prefix` and was silently renamed by
  the `iterate <prefix,…>` it is used in (§1.3 rule 6).
- *Scope.* NDD + NF: add or and sub xor inc dec neg shl sal shr sar rol ror
  shld shrd imul(AF). NDD only: adc sbb not rcl rcr. NF only: mul div idiv
  imul (F6/F7, 69/6B) popcnt lzcnt tzcnt. Left for Phase 4: cmovcc NDD
  (its NF bit means CFCMOV), setcc/imul `{zu}`, ccmp/ctest, maps 2/3.
- *Verification.* 72/72 valid forms from a hand list and 11 more
  edge forms byte-identical with llvm-mc; the sweep: 15,750 forms
  identical, 0 failures (llvm-only forms down from 3,170 to 2,991).
  `apx_ndd.asm`: 87 byte checks, 13 refusals.
- `_dupnames.py` now follows `include` lines (the order fasmg sees
  definitions; `apx.inc` sorts first but is included last), and accepts a
  stack whose later definitions all name themselves in their body: the 28
  APX layers pass, a G12-style redefinition still fails.
- **Discovery point 3, measured.** Phases 0–3 against win32: fasmg
  self-build 384 → 414 ms (+7.8%), fasmgw 915 → 975 ms (+6.6%), identical
  bytes; the layers alone are about +2.5% (414 vs 404 ms), the cost of the
  second dispatch on every ALU/shift/inc/dec/imul instruction. Not yet
  acted on. If that is too much, option (b) above moves the fast-path test
  into the legacy definitions (no second dispatch) at the price of editing
  them in place.

### Phase 4 — the rest of APX_F

- `ccmpSCC` / `ctestSCC` with `{dfv=of,sf,zf,cf}` as the first operand
  (comma after it optional, as NASM), or an immediate 0–15 (OF,SF,ZF,CF =
  bits 3–0). SCC names are the condition codes plus `t`/`f` for
  1010/1011. Like `{nf}`, the braces sit after the mnemonic and are split
  off the operand text by the instruction.
- `cfcmovcc` (load/store forms, and NDD form).
- ZU forms: `setCC {zu} r8`, `imul {zu} r, r/m, imm` (§1.1). Check whether
  `{nf}` and `{zu}` may appear together on `imul` (Intel's `imulnfzu`);
  if so, accept them in either order.
- EVEX promotion of the VEX-encoded GPR instructions so they can take
  r16+: BMI1/BMI2 (`andn`, `bextr`, `bzhi`, `mulx`, `pdep`, `pext`, `rorx`,
  `sarx`, `shlx`, `shrx`, `blsi`…), CMPccXADD, RAO-INT, USER_MSR,
  `movbe`, `crc32`, `invept`/`invvpid`/`invpcid`, `kmov` with GPRs.
  This extends `AVX.store_gpr_instruction` (adds R4/X4/B4 and the EVEX
  map-4 path) and `msr.inc`'s map-7 emitter.
  `cmpccxadd.inc`, `rao.inc`, `msr.inc` already note these forms as missing
  (EXTENSIONS.md §7 item 4).

Gates: APX_F **and** the instruction's own feature (XED's
`apx-f/cpuid*.xed.txt` lists them as pairs).

**Phase 4a result (2026-11-02, done): map 4 conditionals and ZU.**
- *New test oracle:* `tests/iset/_apx_corpus.py` assembles LLVM's 63
  `llvm/test/MC/X86/apx/*-intel.s` files (authority 2) after translating
  their syntax (`ptr` dropped, `{nf}` after the mnemonic, `setzuCC` →
  `setCC {zu}`, `imulzu` → `imul {zu}`). Before 4a: 1,535 same, 228 DIFF,
  1,477 refused. After: 2,692 same, 137 DIFF (all `{evex}` forms of
  map 2/3 and VEX instructions: 4b, 4c), 394 refused (180 are shifts with
  an implied count, `shl bl`, `shl dx, dx`: fasm2 requires the count, by
  design), 17 reviewed (`lea r, [1*rN]`: fasm2 makes it a base register).
- cmov<cc> NDD (ND=1); `{evex} cmov<cc>` is refused: ND=0 NF=0 in row 4 is
  cfcmov's load form (llvm-mc refuses it too). cfcmov<cc> (new, apx.inc)
  maps its three forms onto the legacy cmov<cc> through `APX.promote`
  (store and register forms swap the operands and force NF).
- set<cc>: map 4 row 4 with pp = F2; `{zu}` = ND. imul `{zu}` only on the
  imm form; `{nf}` with `{zu}` is accepted (Intel 817241 4.1 shows it;
  llvm-mc has no such form).
- ccmp<scc>/ctest<scc> (new): a CCMP mode in `x86.store_instruction` puts
  the DFV in P1.vvvv uninverted and the SCC in P2[3:0]; legacy cmp/test do
  the operands (their A8h/A9h short forms are skipped in EVEX mode, like
  the ALU's). `{dfv=…}` with optional comma, or Intel's numeric form
  (817241 7.2; llvm-mc lacks it). `{evex} cmp/test` = ccmpt/ctestt
  `{dfv=}`, as llvm-mc encodes them.
- `APX.promote` takes flags (APX.NF, NDD, ZU, SIZED, NDD_ONLY, FORCE_NF)
  and an option loop ({nf}/{zu} in any order).
- *Trap found:* apx.inc needs `define APX APX`; without it `APX.NF` is
  "undefined or out of scope" inside every CALM body (EXTENSIONS.md §1.3
  rule 8).
- Tests: `apx_conditional.asm` (48 byte checks, 16 refusals); corpus files
  ccmp 440, ctest 272, cfcmov 210, cmov 96, setcc 32, setzucc 46,
  imulzu 12 all byte-identical; sweep 21,840 same, 0 failures.

**Phase 4b result (2026-11-02, done): legacy maps 2/3.**
- *Design pivot:* no layers. Legacy maps 2/3 have no REX2, and spec rule
  2e promotes all their GPR instructions, so `x86.store_instruction`
  itself switches to EVEX map 4 when r16–r31 (or `{evex}` with APX_F on)
  meet a map 2 opcode. The test sits before any prefix byte is emitted
  (EVEX absorbs 66/F2/F3 into pp) and captures the opcode only on that
  path. No instruction definition changed for it: movbe, crc32, invept,
  invvpid, invpcid, wrss/wruss, enqcmd(s), movdir64b, movdiri, RAO-INT,
  urdmsr/uwrmsr gained r16–r31 at once.
- Map 4 opcode/pp table taken from llvm-mc's encodings (the spec's table
  did not survive text extraction): movbe F0/F1 → 60/61; crc32 F0/F1
  (F2 dropped; 66 for 16-bit); 80–82 → F0–F2 with pp=F3; F5 → 65; F6 → 66;
  F8, F9, FC unchanged with their prefix as pp.
- Layers only where the form needs them: adcx/adox (NDD). movbe gained
  the register form (EVEX only, F1 direction). urdmsr/uwrmsr imm32 take
  EVEX map 7 in msr.inc.
- *Regression found and fixed before commit:* `{evex}` is `vexsize = 4`,
  which AVX-512 code may set globally (`use evex`). Without a gate every
  `add`/`crc32`/`cmp` would then demand APX_F. `{evex}` now promotes only
  when APX_F is on (layers, map-2 path, msr.inc); `apx_promoted.asm`
  checks `use AMD64, AVX512, evex` keeps legacy bytes.
- Found on the way: G18 (`crc32 r64, r/m8` without REX.W) and G19 (WRSS
  with a register destination), both pre-existing, fixed with tests.
- Tests: `apx_promoted.asm` (58 byte checks, 6 refusals), `crc32.asm`,
  `cet_wrss.asm`; corpus: adx 16, cet 8, crc32 22, enqcmd 8, invpcid 2,
  movbe 18, movdir64b 4, movdiri 4, rao-int 16, user-msr 8, vmx 4 all
  identical (whole corpus: 2,802 same, 95 DIFF, all VEX `{evex}` forms for
  4c); sweep 22,505 same, 0 failures.
- Not done: MSR-IMM (`rdmsr`/`wrmsrns` with an immediate, its own CPUID
  feature) is not implemented at all (4 corpus lines refused).

**Phase 4c result (2026-11-02, done): promoted VEX instructions.**
- `AVX_512.store_instruction` writes the APX EVEX form (spec Figure 3.4)
  for instructions marked `APX_PROMOTED`: `VEX_GPR` implies it (BMI1/2,
  CMPccXADD; not XOP, maps 8–10), and KMOV* and AMX's ldtilecfg/
  sttilecfg/tileload*/tilestored pass it through the new
  `AVX.store_apx_instruction`. It is taken for r16–r31, for `{nf}` (layers
  over andn, bextr, blsi, blsmsk, blsr, bzhi) and for `{evex}` with APX_F
  on. B4 (P0 bit 3), X4 (U cleared), NF (P2 bit 2) added; R4/V4 already
  came from bit 4 of the register numbers; the vector X bit no longer
  fires for a GPR in r/m; disp8 is not scaled (memsize 1).
- AMX's `[base, index]` form takes r16–r31 now (`MPX.parse_sib_operand`
  marks `egpr`; the MPX instructions themselves refuse it).
- Tests: `apx_vex.asm` (43 byte checks, 5 refusals). Corpus: **3,028 same,
  0 DIFF**, 195 refused (180 implied-count shifts by design, 11 vector
  forms for Phase 5, 4 MSR-IMM). Sweep 23,590 same, 0 failures. No
  measurable cost (branch 421–422 ms / 982–992 ms, as after 4b).
- Rough edge, recorded: `{nf}`/`{zu}` on a mnemonic that has no layer
  (e.g. `mulx {nf} …`, which has no NF form) reaches that instruction's
  operand parser and gives a parser error ("invalid expression") instead
  of "{nf} not supported". Layers with no flags would fix it at layer
  cost; left for the post-APX review (§5). *Fixed 2026-11-03:*
  `x86.parse_operand` reports an option it is handed (`{nf} not supported
  by this instruction`, also `{zu}`, `{dfv}`); the layers take theirs
  before parsing, so no layer is needed. (The `??` line hook cannot do
  it: it sees only lines that do not start with an instruction.)

**Phase 4 complete.** Every APX form in LLVM's Intel-syntax tests that
does not involve a vector register with r16–r31 assembles to the same
bytes as llvm-mc (or, where Intel's spec disagrees, to the spec).

### Phase 5 — extended GPRs in vector instructions

- EVEX: put B4 (P0 bit 3) and X4 (inverted P1 bit 2, the old `U` bit) in
  `AVX_512.store_instruction`; base/index r16–r31.
- VEX has no room for them: a VEX instruction with r16+ in the address is
  promoted to EVEX when the EVEX form exists and is enabled (AVX-512/AVX10);
  otherwise refused.
- Instructions with GPR operands in vector maps (`vmovd`, `vpextr*`,
  `vpinsr*`, `vcvtsi2ss`, …) likewise.

**Discovery point 5.** AVX10 interaction: AVX10.2 may change what
"enabled" means here. Revisit with the AVX10.2 plan.

**Phase 5 result (2026-11-02, done).**
- `AVX_512.store_instruction`: for an instruction that is not
  APX_PROMOTED, r16–r31 now force its EVEX form (`apx = 2`) instead of
  being refused, unless it has none (EVEX_FORBIDDEN: VEX-only such as
  vaesimc and the VEX gathers; XOP), which still refuses. The EVEX form
  keeps its usual gates (AVX512F, AVX512VL for xmm/ymm; "AVX512F or higher
  required" when AVX-512 is off). B4/X4 reuse the 4c code; X4 is set only
  for a GPR index (`egpr = 1`): the VSIB parser marks a base r16–r31 with
  `egpr = 2`, so a vector index keeps U = 1. disp8 keeps EVEX scaling.
- "Enabled" for now means `x86.simd >= AVX512F` (+ VL); AVX10 is not
  modelled separately (discovery point 5 stands for the AVX10.2 plan).
- Found on the way, all pre-existing (upstream) and fixed with tests:
  G20 vpinsrq/vlddqu unset operand state, G21 vpbroadcastq EVEX W0
  (= vbroadcasti32x2) and missing AVX512VL gate, G22 vcompressps/pd in
  map 1 (#UD). G21 and G22 were silent misencodes.
- Tests: `apx_vector.asm` (38 byte checks, 4 refusals),
  `avx_operand_state.asm`, `vpbroadcastq.asm`, `vcompress.asm`. Corpus:
  **3,039 same, 0 DIFF**; the 184 refused are the 180 implied-count
  shifts (by design) and MSR-IMM (4, not implemented). Sweep 34,776
  same, 0 failures (llvm-only forms 2,561 → 963: the rest are llvm-mc
  accepting a vector index in non-VSIB instructions, `[r16+zmm1]`, and an
  extra immediate after vcvt*2si, plus fasm2's stricter size rules).
- No measurable cost.

**APX complete** for everything LLVM's tests cover, except MSR-IMM, which
is its own extension (`MSR_IMM`, not planned yet).

---

## 4. Open questions

1. ~~`{nf}` as hook-only vs. a normal `use` setting~~ Decided: options
   (`{nf}`, `{zu}`, `{dfv=…}`) after the mnemonic only; encoding
   selection (`{rex2}`, `{evex}`) before it (§1.1).
2. ~~`{dfv=…}` and ZU syntax~~ Decided (§1.1, Phase 4).
3. ~~Which execution emulator~~ Decided: none; emulators are not trusted
   (§1, §2).
4. ~~Whether `x86.ext` needs a new window~~ Decided: no. APX_F is in
   7.1:EDX (window 192). XED v2026.08.23 gates every APX form on APX_F plus
   the instruction's own feature (`APX_F_ADX`, `APX_F_BMI1`, ...); it has
   no NDD/NF sub-feature, so fasm2's gating matches.
5. ~~Syntax in other assemblers~~ Decided: fasm's choices first, then the
   manuals (§1.1). NASM 3.02 is the only other Intel-syntax assembler here
   with APX.

## 5. Post-APX review (to revisit when APX is complete)

The layering stays for now (user decision, 2026-11-02). This section
collects what to reengineer with the whole picture in view.

### 5.1 Cost findings

Best of five runs, against the win32 tree before Phase 0 (output bytes
identical in every row):

| After | fasmg self-build | fasmgw | Source of the added cost |
|-------|------------------|--------|--------------------------|
| Phase 0 | +10 ms (+2.7%) | +13 ms (+1.5%) | a register-number check per register operand and per address register term (moved later to where base/index are computed) |
| Phase 1 | +20 ms (+5.4%) | +46 ms (+5.3%) | per operand: `compute egpr, 0` and one compare; per instruction in `x86.store_instruction`: the REX2 bit test (behind a single test of bit 4), `check x86.rexsize`, the duplicated `prefix = 48h` test |
| Phase 2 | no change | no change | new mnemonics only |
| Phase 3 | +30 ms (+7.8%) | +60 ms (+6.6%) | the layers: a second dispatch (`arrange` + `assemble`) for every add/or/and/sub/xor/adc/sbb/inc/dec/neg/not/mul/div/idiv/imul/shift/shld/shrd/popcnt/lzcnt/tzcnt, plus two `match`es and constant checks; `check x86.apx.evex` in `x86.store_instruction` |
| Phase 4a | +28 ms (+7.1%) | +73 ms (+8.0%) | layers also over set<cc>, cmov<cc> and (for `{evex}` only, one check) cmp/test; within noise of Phase 3 (±1%) |
| Phase 4b | +40 ms (+10.5%) | +75 ms (+8.3%) | one more check in `x86.store_instruction` (r16+ or `{evex}` on the map-2 path); the branch itself is unchanged (420–423 ms, 977–980 ms in both measurements); the ratio moves with the baseline, which drifted 378–395 ms between sessions |

| Phase 4c | +36 ms (+9.4%) | +70 ms (+7.6%) | none on the plain path (the APX test in `AVX_512.store_instruction` runs only for APX_PROMOTED instructions); branch time unchanged from 4b |
| Phase 5 | +34 ms (+8.7%) | +69 ms (+7.5%) | none measurable: branch 424 ms / 982–986 ms, as after 4c |

Measurement note: best of 5–7 runs, but the baseline tree varies by ±2–4%
between sessions; compare branch times measured in one session (interleaved
runs, as for 4b) before reading a phase's cost.

The ALU/shift-heavy sources pay most. Per-instruction work dominates:
the code paths that only APX uses (REX2 opcode capture in `virtual`, EVEX
emission, `APX.promote`) are not on the plain path at all.

### 5.2 Alternatives, cheapest change first

1. **One dispatcher instead of 28 layer bodies** (plan option a): the match
   logic is already shared through `APX.promote`; the fast path is what
   costs, so this saves little. Low value.
2. **Test in the legacy definitions** (option b): put the `{`/operand-count
   test at the top of each legacy CALM body (the ALU `iterate`, the shift
   `iterate`, inc/dec, not/neg/mul/div/idiv, imul, shld/shrd, and the
   popcnt/lzcnt/tzcnt macros) and drop the layers. Removes the second
   dispatch (most of Phase 3's cost); the legacy files then know about
   APX, the opposite of today's separation. `_dupnames.py` would lose 28
   reviewed layers.
3. **Opt-out setting**: `use noapx` (or APX off in the settings) could
   skip layer and EGPR checks wholesale. Rejected so far: it is a second
   scheme beside the CPUID gates, and gates must not change encodings.
4. **Cheaper REX/REX2/EVEX selection**: fold `prefix = 48h`, `rexsize`
   and `apx.evex` into one precomputed mode value per instruction (set by
   the parser and the layers), so `x86.store_instruction` tests once.
5. **Extension table instead of `x86.ext` big integers** (raised before
   APX): independent of APX, but measured in the same way; worth doing in
   the same reengineering pass if the gate checks show up in a profile.

Before choosing: profile with a source that is mostly ALU/shift (where 2
matters) and one that is mostly vector (where 4 and 5 matter).

### 5.3 Errors found, and where to report them

fasm2 upstream (tgrysztar/fasm2; all present on `master` aab8758,
2026-06-22). Fixed here only where APX needed it:

| # | Defect | Repro | Root cause | Here |
|---|--------|-------|------------|------|
| G17 | `cmpxchg16b [r8]` assembles `48 41 0F C7 08`; the CPU ignores the 48h and runs cmpxchg8b | `use64` / `cmpxchg16b [r8]` | `x86.store_operand_prefix, (8)` emits REX.W as its own byte before `x86.store_instruction`'s REX | fixed, `tests/iset/cmpxchg16b.asm` |
| G15 | AVX `*_imm8` helpers accept a memory operand as the imm8 and use a stale value | `vroundpd xmm0, xmm1, 3` then `vroundpd xmm0, xmm1, [rax]`: both `C4 E3 79 09 C1 03`, no error | `AVX_512.single_source_instruction_imm8` and `vcvtps2ph` never check `@aux.type = 'imm'` | fixed, `tests/iset/avx_imm8.asm` |
| G16 | `rcppd`, `rcpsd`, `rsqrtpd`, `rsqrtsd` assemble (no such instructions; XED: GENERAL_ERROR) | `rcppd xmm0, xmm1` | `sse2.inc` reuses SSE's iterate list including `rsqrt`/`rcp` | fixed, `tests/iset/sse2_rcp.asm` |
| G16b | `cvtss2si eax, mm0` assembles as `cvtss2si eax, xmm0`; same for `cvtps2pi mm0, mm1`; `pextrb dword [rax], xmm0, 1` accepted | as shown | `sse.inc`, `sse4.inc`: `@src.size = 'mem'` / `'mmreg'` compare the *size* with a type string (meant `.type`), so the size check never applies | fixed, `tests/iset/sse_operand_size.asm` |
| G18 | `crc32 rbx, al` assembles `F2 0F 38 F0 D8` (the `crc32 ebx, al` encoding; same result) | as shown | `sse4.inc` crc32: no `select_operand_prefix` for an 8-bit source | fixed, `tests/iset/crc32.asm` |
| G19 | `wrssq rax, r8` assembles (`4C 0F 38 F6 C0`, Mod=11); WRSS/WRUSS are `m, r` only | as shown | `cet.inc`: `@dest.type = 'reg' \| @dest.type = 'mem'` | fixed, `tests/iset/cet_wrss.asm` |
| G20 | `vpinsrq xmm0, xmm1, rax, 1` and `vlddqu xmm0, [rax]` fail with "invalid expression" when first in a source | either line alone after `use64` | `avx.inc`: the stored operand is parsed with `x86.parse_operand`, leaving `mask`/`evex_b`/`memsize` undefined (or stale) | fixed, `tests/iset/avx_operand_state.asm` |
| G21 | `vpbroadcastq zmm0, [rax]` assembles `62 F2 7D 48 59 00`, which XED and Zydis decode as `vbroadcasti32x2` | as shown | `avx.inc`: VEX_66_0F38_W0 without EVEX_W1 for the q form; no EVEX_VL for d/q | fixed, `tests/iset/vpbroadcastq.asm` |
| G22 | `vcompresspd [rax], xmm0` assembles `62 F1 FD 08 8A 00`, which XED cannot decode | as shown | `avx.inc`: `VEX_66_0F_W0/W1` (map 1) for vcompressps/pd instead of `VEX_66_0F38` | fixed, `tests/iset/vcompress.asm` |
| G23 | `lddqu` uses the previous SSE instruction's destination: `addps xmm5, xmm1` / `lddqu xmm0, [rax]` = `F2 0F F0 28` (`lddqu xmm5, [rax]`), no error; first in a source, `invalid expression` | as shown | `sse3.inc`: `SSE.parse_operand@src dest` (meant `@dest`) | fixed, `tests/iset/sse3_lddqu.asm` |
| G24 | after a `{settings}` prefix on tileloadd/tileloaddt1/tilestored, every later memory operand gets a SIB byte: `{vex3} tileloadd tmm1, [rax]` / `lea eax, [rax]` = `8D 04 20` | as shown (also `{evex}`, `{imm8}`) | `amx.inc` `AMX.parse_sib_operand`: `take`/`compute`/`take` on `x86.usesib` pops the copy the prefix pushed and overwrites the saved value with 1 | fixed (save and restore with `compute`), `tests/iset/amx_sib.asm` |
| G25 (this branch, `msr.inc` from b3b0669; not upstream) | `urdmsr rax, rcx` assembles `F2 0F 38 F8 C1`, which is `urdmsr rcx, rax` (XED; GNU as writes `…C8`) | as shown, also the APX EVEX form | `msr.inc` puts the destination in ModRM.reg; SDM 093 URDMSR is Op/En **MR** (operand 1 = ModRM.rm, "load into register bbb the MSR at address in rrr"), XED's `REG0=GPR64_B():w`. The imm32 form already uses ModRM.rm. uwrmsr (RM) is right | **open**: fix in `msr.inc`, `msr.asm`, `apx_promoted.asm`; it changes bytes llvm-mc also writes (below), so decide first |
| G26 | `vaeskeygenassist` never assembles ("symbol 'opcode' is undefined") | `vaeskeygenassist xmm0, xmm1, 1` | `aes.inc`: the macro names the other iterate block's parameter; d4be069 fixed the same swap in `vaesenc`/`vaesdec` only | fixed, `tests/iset/aes_forms.asm` |
| G27 | `aesimc` (SSE) never assembles ("symbol 'supp' is undefined") | `aesimc xmm0, xmm1` | `aes.inc`: as G26, the other way round | fixed, `tests/iset/aes_forms.asm` |

Found by GNU as (`_apx_gas.py`): G23 and G24 hid in the sweep because each
form is assembled in a 20,000-line source, and the line before decides
what they do. Both the LLVM comparison and `_apx_corpus.py` got the
expected bytes by chance. The GNU run put LLVM's corpus after the sweep
lines in one source, where `{evex} tilestored` (G24) changed the bytes of
`lea r16d, [rax]` and 62 more corpus lines that follow it.

LLVM (llvm-project, X86 AsmParser/MCCodeEmitter), all on LLVM 23 (build
4d58566 and source fc16f15, no tests cover them). Intel's spec says each
form #UDs; llvm-mc encodes it without a diagnostic:

| Form | llvm-mc bytes | Spec 355828-009 |
|------|---------------|-----------------|
| `{rex2} movbe eax, [rcx]` | `D5 00 0F 38 F0 01` | 3.1.2.1: REX2 followed by 0x0F triggers #UD; maps 2/3 have no REX2 |
| `{rex2} xsave [rax]` | `D5 80 AE 20` | 3.1.2.1: REX2 on XSAVE*/XRSTOR* triggers #UD |
| `push2 rsp, rax` / `push2 rax, rsp` | `62 F4 5C 18 FF F0` / `…FF F4` | 3.1.3.1.1: neither register may be RSP |
| `pop2 rax, rax` | `62 F4 7C 18 8F C0` | 3.1.3.1.1: POP2's two registers must differ |
| `urdmsr rax, rcx` (and the EVEX form) | `F2 0F 38 F8 C1` | not #UD but the other instruction: SDM 093 URDMSR is MR, so these bytes read the MSR at rax into rcx (G25; LLVM's `user-msr-intel.s` tests encode it the same way) |

Intel (spec 355828-009, possible erratum): 3.1.2.3.1 says "EVEX.X4 is
defined only when ModRM.Mod = 3", but X4 extends the SIB index, which
exists only when Mod ≠ 3. LLVM and fasm2 both encode X4 for the index
(`add rax, [rax+r20*4], rcx` = `62 F4 F8 …`, U = 0). Ask before relying on
the sentence.

XED v2026.08.23 confirms every row above: it refuses each of llvm-mc's
bytes (`BAD_REGISTER`, `BAD_REG_MATCH`, `GENERAL_ERROR`). Intel's oneAPI
2026.2 assembler produces the same bytes as llvm-mc (shared MC code), so a
fix upstream reaches it too.

**Policy: fasm2 assembles no instruction that raises #UD** unless the
source asks for those bytes outright (`db`, or a future explicit option).
`tests/iset/_apx_xed.py` checks it. XED must decode every form fasm2
accepts in the sweep (r16, r8 and the edge registers) and in LLVM's APX
corpus: 2026-11-03, 44,560 forms, all decoded (G16, fixed, was the
only failure). XED shows #UD; it does not show a wrong but valid
instruction (G15, G16b), which is what the comparison with llvm-mc is for.
**Policy: an encoding requested for one line is honoured or refused**
(user decision 2026-11-03). `{vex2}`, `{vex3}`, `{evex}`, `{rex2}` before
an instruction are a request, as a size on a jump is (`jz byte target`
fails when out of range): `{rex2} movbe`, `{evex} vaesimc`, `{vex3} add`
are errors (`{evex} cannot encode this instruction`), where fasmg drops
them silently. They are used to control length (padding, alignment), so
a silent fallback breaks layout unseen. `use vex3` etc. for a block stay
preferences, since a block mixes instructions with and without the
encoding. `x86.select_check` (x86-2.inc) reads the first byte after
legacy prefixes; tests: `encoding_request.asm`, `apx_rex2.asm` FAIL
20–23, `apx_promoted.asm` FAIL 7–8. Upstream difference from fasmg.

GNU as 2.47 (binutils), from `_apx_gas.py -x` with XED deciding:
- **Defect:** REX2 on 3DNow! (`0F 0F … imm8`): `pfadd mm0, [r16]` =
  `D5 90 0F 00 9E`, which XED refuses (GENERAL_ERROR): all 23 3DNow!
  mnemonics, 161 forms. fasm2 refuses r16–r31 there. AMD-only ISA, so
  low priority for a report.
- Lenient, not wrong: wrong string or xlat operands (`movs byte [r16],
  byte [rax]` = `A4`) and fasm-unknown spellings are accepted with a
  warning or an alias (`movq r16, rax` = mov, `shrd r16d` = shr with a
  `d` suffix, `setz r16` = setzuz, implied accumulator `idiv rax, r16`,
  `lar r16, rax`). These make up most of the 14,704 gas-only forms.
- Encoding choices where both are valid (XED decodes them the same):
  register cfcmov<cc> with NF=0 (fasm2 and llvm-mc: NF=1, 1,303 forms),
  EVEX movbe register form `60` (fasm2: `61`), EVEX vmovq m64 as
  `66 0F 6E/7E W1` (fasm2: `F3 0F 7E` / `66 0F D6`), xchg's register
  order, 67h before 66h, and REX.W dropped from lar/lsl/sldt/str with a
  64-bit destination (zero-extended, so the same result; also without
  APX).
- Forms it refuses that XED decodes: `imulzu` with 32/64-bit operands,
  `setzu<cc> m8` (both in LLVM's corpus), m512 with a `zmmword` size
  (enqcmd(s), movdir64b).
- Forms only GNU as and llvm-mc have, which fasm2 lacks (no wrong bytes):
  ccmp/ctest without `{dfv=}` (GNU defaults to none), prefetchit0/1
  without RIP-relative (a NOP there), AVX10.2 EVEX forms (vpdpw*/vpdpb*,
  vsm4*, vmpsadbw) and MSR_IMM (rdmsr/wrmsrns imm32), plus three
  non-APX AVX-512 gaps: `vbroadcasti32x2 xmm, m64`, `vpermq/vpermpd ymm,
  ymm, ymm/m256`, and the memory forms of the shift-by-imm8 instructions
  (`vpsrlw xmm0, [rax], 1`). Open.

NASM 3.02rc7: rejects Intel's recommended `setzuCC` mnemonic (817241 §3)
and accepts only `{zu}`; not a defect (fasm2 chose `{zu}` too), noted for
syntax comparisons.

## 6. Change log

- 2026-11-02: plan written from the LLVM 23 reference encodings and the
  r16 hazard test. QEMU 11.1-rc2 found to know `apxf` (execution untested).
- 2026-11-02: syntax decisions (`{nf}` before or after the mnemonic; fasm
  first, manuals next). Added Intel 817241, NASM 3.02rc7 and MSVC
  `/feature:APX` as references; NF list corrected from 817241.
- 2026-11-02: placement rule: options after the mnemonic only (`{nf}`,
  `{zu}`, `{dfv=…}`), encoding selection before it (`{rex2}`, `{evex}`).
  ZU as `{zu}`, not `setzuCC`/`imulzu`.
- 2026-11-02: Phases 4a–4c and 5 done (see each result). LLVM's APX
  corpus: 3,039 forms identical, 0 different. G18–G22 found and fixed.
- 2026-11-02: encoding authority order recorded (Intel docs, LLVM, XED,
  others); LLVM checkout update has no APX encoding changes. Phase 0
  done: refusal in the three operand parsers, sweep 0 leaks; Phase 1
  revised to an opt-in flag on the parser.
- 2026-11-02: Phase 1 done. Intel's "every map 0/1 instruction" rule
  turned the opt-in back into encoder decisions, verified by a differential
  sweep against llvm-mc. Intel overrules llvm-mc on `{rex2}` movbe/xsave.
  G17 fixed; G15/G16/G16b recorded.
- 2026-11-02: Phase 2 done (`apx.inc`: push2/pop2(p), pushp/popp, jmpabs,
  r16–r31 in register lists). Intel overrules llvm-mc on rsp and pop2
  with one register twice.
- 2026-11-02: Phase 3 done: NDD/{nf}/{evex} for the legacy integer set
  through layers and `APX.promote`; ~2.5% cost for the layers, recorded
  at discovery point 3 for a decision.
- 2026-11-03: Intel oneAPI 2026.2's assembler checked as a reference
  (through `clang -cc1as`; the toolkit was then uninstalled). It agrees with upstream llvm-mc on the whole LLVM APX
  corpus and on the sweep, so authorities 2 and "Intel's product" agree
  for every form tested.
- 2026-11-03: XED v2026.08.23 built; it refuses every #UD form llvm-mc
  encodes. Policy: fasm2 emits no #UD instruction; `_apx_xed.py` decodes
  all 44,560 accepted forms with XED. G16 fixed (its only failure).
- 2026-11-03: authority order revised: Intel docs, XED, LLVM, GCC/binutils,
  others. Hardware execution outranks all; emulators are not trusted.
- 2026-11-03: one-line `{vex2}`/`{vex3}`/`{evex}`/`{rex2}` are requests:
  an error where they cannot apply (`x86.select_check`); `use` keeps
  them preferences. Corpus, sweep and XED check unchanged (0 DIFF,
  0 failures, 44,560 decoded).
- 2026-11-03: pre-review pass: G15, G16b fixed (and the same size/type
  mix-up in cvtps2pi, pextrb), G26/G27 (aes.inc) found and fixed; `{nf}`
  etc. after a mnemonic without that form now says so; `[a,b]` outside
  MPX/AMX is "invalid address". IEV (Tomasz Grysztar's form) added in
  tests/IEV with the iset checks and a boundary set; the boundary set made
  fasm2 swap an r13/r21/r29 base with its index like rbp's (1 byte).
- 2026-11-03: GNU binutils 2.47 built in WSL (`~/opt/binutils-latest`)
  as the second, LLVM-independent encoder; `_apx_gas.py` compares it with
  fasm2 on the sweep and LLVM corpus forms: 299,295 forms, 36,352 same,
  1,552 DIFF, 737 fasm-only, 14,704 gas-only; every DIFF and fasm-only
  form explained (valid alternatives, GNU syntax, G15/G16b) except G25,
  open. Found and fixed G23 (lddqu took the previous destination) and
  G24 (`{settings}` on AMX tile load/store left `{sib}` on); G25 (urdmsr
  register operands swapped, also in llvm-mc) is open. GNU defect: REX2
  on 3DNow! (#UD per XED). After the fixes: sweep 34,783 same, 0
  failures; corpus 3,039 same; XED decodes all 44,568 forms.
