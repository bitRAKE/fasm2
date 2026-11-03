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

**References that already encode/decode APX** (checked):

| Tool | APX status |
|------|------------|
| LLVM 23 `llvm-mc` (`C:\git\llvm-project\build-native\bin`) | Encodes every form tried with `-triple=x86_64 --show-encoding -mattr=+egpr,+push2pop2,+ppx,+ndd,+ccmp,+nf,+cf,+zu` (plus `+avx512f`, `+bmi` for those forms). |
| XED v2025.12.14 | Decodes REX2 and EVEX map 4 (`xed -64 -d D54801C0` → ADD). `data\src\xed\datafiles\apx-f\` holds the patterns and `cpuid*.xed.txt` the gates. |
| ZydisInfo | Decodes REX2 (`D5 48 01 C0` → `add rax, r16`). |
| zydis-db (`C:\git\zyantific\zydis-db\datafiles`, 2026-06) | APX forms are in `instructions.json`, for generating test cases. |
| NASM 3.02rc7 (`C:\git\nasm\nasm.exe`) | Intel-syntax reference: assembles REX2, NDD, `{nf}` (before or after the mnemonic), `push2`, `ccmp {dfv=…}`, `{zu} setb`. Doc: `doc\syntax.src` §`apx_*`. Rejects Intel's `setzuCC` mnemonic. |
| MSVC 14.51 `cl /feature:APX` | Accepted; emits APX when it helps (a trivial function did not). A source of real compiler output (`/FA`) to assemble and compare once Phase 3 exists. |
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
byte tests only.

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

### Phase 2 — new mnemonics: `iset/apx.inc`

CALM, new names only:
- `push2` / `pop2` (and their PPX forms `push2p` / `pop2p`);
- `pushp` / `popp` (REX2.W hint on 50+r / 58+r);
- `jmpabs imm64`.

Header lists the CPUID requirement (APX_F, 64-bit). File is included by
`x86-2.inc` like every other iset file; `use APX_F` is the opt-in.

Tests: bytes vs. llvm-mc; gate off → `APX_F or higher required`; `pop2`
with the same register twice is refused (#UD).

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

---

## 4. Open questions

1. ~~`{nf}` as hook-only vs. a normal `use` setting~~ Decided: options
   (`{nf}`, `{zu}`, `{dfv=…}`) after the mnemonic only; encoding
   selection (`{rex2}`, `{evex}`) before it (§1.1).
2. ~~`{dfv=…}` and ZU syntax~~ Decided (§1.1, Phase 4).
3. Which execution emulator (§2, discovery point E).
4. Whether `x86.ext` needs a new window: APX_F is in 7.1:EDX (window 192,
   already allocated). `apx-nci-ndd-nf` (QEMU's name) is APX_NCI_NDD_NF,
   a later sub-feature; find its CPUID bit (XED/SDM) before Phase 3, since
   it may gate the NDD/NF forms of some instructions.
5. ~~Syntax in other assemblers~~ Decided: fasm's choices first, then the
   manuals (§1.1). NASM 3.02 is the only other Intel-syntax assembler here
   with APX.

## 5. Change log

- 2026-11-02: plan written from the LLVM 23 reference encodings and the
  r16 hazard test. QEMU 11.1-rc2 found to know `apxf` (execution untested).
- 2026-11-02: syntax decisions (`{nf}` before or after the mnemonic; fasm
  first, manuals next). Added Intel 817241, NASM 3.02rc7 and MSVC
  `/feature:APX` as references; NF list corrected from 817241.
- 2026-11-02: placement rule: options after the mnemonic only (`{nf}`,
  `{zu}`, `{dfv=…}`), encoding selection before it (`{rex2}`, `{evex}`).
  ZU as `{zu}`, not `setzuCC`/`imulzu`.
