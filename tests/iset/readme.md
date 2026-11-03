# Instruction-set regression tests

One source per fix in `include/iset` or the extension table in
`include/x86-2.inc`. Each source explains, in its header, the environment
it runs in and the failure mode it resolves. The plan behind the fixes is in
[`include/iset/EXTENSIONS.md`](../../include/iset/EXTENSIONS.md).

## Running

```
powershell -ExecutionPolicy Bypass -File tests\iset\_run.ps1
powershell -ExecutionPolicy Bypass -File tests\iset\_run.ps1 svm lzcnt
powershell -ExecutionPolicy Bypass -File tests\iset\_run.ps1 -Root C:\git\fasm2 xtest
```

`-Root` runs the tests against another fasm2 tree; use it to confirm that a
test fails on the tree before its fix. The exit code is the number of failed
checks.

## How a test works

Everything happens at assembly time; no test code is executed.

- **Encodings.** `expect <bytes>, instruction` assembles the instruction
  inside `virtual` and compares it with the expected bytes, which come from
  `llvm-mc` or the vendor manual. A wrong byte or a wrong length stops
  assembly with `encoding mismatch` and names the line.
- **Settings.** Expectations put their settings in front of the
  instruction, `{i386, LZCNT} lzcnt eax, ecx`, so each one names exactly the
  features it needs. `i386` resets the extension bits to none; `x64` raises
  the CPU level for 64-bit code.
- **Failure cases.** A header line `; FAIL n: 'message'` declares a case.
  The runner assembles the file again with `-iFAIL=n`; that assembly must
  fail, and fasmg's output must contain the message. The source selects
  the case with `else if FAIL = n`.

The runner calls `fasmg.exe` directly, the way `fasm2.cmd` does, because
`fasm2.cmd` does not pass fasmg's exit code through.

## Tests

| Source | Fix | What it pins down |
|--------|-----|-------------------|
| `cpuid_windows.asm` | G1 | Every extension bit sits in the 32-bit window of the CPUID register that reports it (checked against XED's `cpuid*.xed.txt`); XSAVEOPT/XSAVEC/XSS moved from 7.1:EAX's window to their own (0Dh.1:EAX). |
| `lzcnt.asm` | G2 | `lzcnt` is gated on LZCNT (8000_0001h:ECX[5]), `tzcnt` on BMI1. `use BMI1` alone no longer allows `lzcnt`. |
| `xtest.asm` | G3 | One `xtest` definition, available with HLE **or** RTM (it was defined twice; the RTM one won). |
| `prefetchw.asm` | G4 | `prefetchw` is available with 3DNow **or** PRFCHW (Intel since Broadwell); `prefetch` stays 3DNow-only. |
| `broadcast.asm` | G5 | `{1to32}` broadcast for 16-bit elements, with element-scaled disp8; oversized broadcasts are still refused. |
| `svm.asm` | S1–S4 | AMD-V: implicit rAX in every mode, address-size `67h`, SKINIT and SEV_ES gates, `vmgexit`, and `svm.inc` available with no extra include. |
| `sev.asm` | new | SEV-SNP: prefixes that separate it from rdpru/invlpgb/tlbsync, rAX as address vs value, 64-bit-only forms, SEV_SNP/RMPQUERY gates. |
| `amd.asm` | new | MONITORX, CLZERO, INVLPGB/TLBSYNC, RDPRU, MCOMMIT: one bit per feature, address-size `67h`, implicit register checks. |
| `vnni_ifma.asm` | G6 | VNNI/IFMA in VEX and EVEX form: each gate alone, both together (VEX by default, `{evex}` or EVEX-only operands select EVEX), refusals when only the VEX feature is on; AVX-VNNI-INT8/INT16. |
| `bf16.asm` | new | AVX512_BF16 and AVX-NE-CONVERT: `vcvtneps2bf16` in both encodings, the half-width size rule (explicit size for an xmm destination from memory), memory-only loads. |
| `sha512.asm` | new | SHA-512: mixed ymm/xmm register operands; independent of SHA and of XSAVEOPT (its old window neighbour). |
| `sm.asm` | new | SM3 (128-bit) and SM4 (128/256-bit), separate bits, VEX only, 8-bit immediate. |
| `vp2intersect.asm` | new | Mask-register pair destination (even k only), AVX512VL for xmm/ymm, D/Q broadcast. |
| `amx.asm` | new | AMX-FP16 and AMX-COMPLEX: AMX_TILE plus own bit, distinct tiles, 64-bit only. |
| `bmi_gpr.asm` | G7 | BMI1/BMI2 assemble without AVX (VEX-encoded GPR instructions); vector VEX still needs AVX. |
| `cache.asm` | new | CLWB (memory only, beside tpause), PREFETCHWT1, PREFETCHIT0/1 (RIP-relative, 64-bit), WBNOINVD. |
| `system.asm` | new | PTWRITE (Intel PT + PTWRITE bits, no 16-bit form), PCONFIG, ENCLV, VMFUNC. |
| `rao.asm` | new | AADD/AAND/AOR/AXOR: prefix selects the operation, no 16-bit form, memory destination. |
| `fred_tdx.asm` | new | ERETS/ERETU, LKGS, TDX instructions: 64-bit only, own gates, neighbours clac/lldt unchanged, jmpe stays out. |
| `msr.asm` | new | RDMSRLIST/WRMSRLIST, WRMSRNS (all modes), URDMSR/UWRMSR register and map-7 immediate forms; enqcmd unchanged. |
| `cmpccxadd.asm` | new | All 30 condition names and aliases, operand roles, W by size, 64-bit only. |
| `sse4a.asm` | new | EXTRQ/INSERTQ (register and two-immediate forms), MOVNTSD/MOVNTSS; vmread/movntps keep their bytes. |
| `3dnowext.asm` | new | PF2IW, PI2FW (moved from 3DNow), PFNACC, PFPNACC, PSWAPD under 3DNowExt; `use AMD64` includes it. |
| `xop.asm` | new | XOP: 8Fh lead byte for maps 8/9 (C4h kept for 0F3A, e.g. vpermil2ps), is4 operand, W choosing the memory slot, vpcom aliases, 128-bit-only forms. |
| `fma4.asm` | new | FMA4: W/is4 operand placement (W1 for register-only forms, as llvm-mc), scalar memory sizes, packed-only vfmaddsub/vfmsubadd. |
| `tbm_lwp.asm` | new | TBM (ModRM.reg-selected operations; bextr imm32 vs BMI1 bextr) and LWP; no AVX needed. |
| `avx512_4x.asm` | G8 | 4VNNIW disp8 scales by 16 (Tuple1_4X), not by the vector; AVX512_4FMAPS. |
| `padlock.asm` | new | VIA/Zhaoxin PadLock: per-unit bits, mandatory F3 on xcrypt*/xsha*/montmul, montmul not in 64-bit code. |
| `avx512_fp16.asm` | new | AVX512-FP16 arithmetic, FMA, complex (distinct destination), scale/rcp/getexp/getmant/reduce/rndscale, compare and aliases, comi, fpclass, vmovsh, vmovw: EVEX maps 5/6, word broadcast and disp8. |
| `avx512_fp16_cvt.asm` | new | The 34 FP16 conversions: width ratios 1/2/4 both ways, ambiguous narrowing memory, disp8 by memory size, scalar and GPR forms. |
| `cmp_predicates.asm` | G9 | `vcmpneq_oq*` exists (avx.inc spelled it `neq_qq`, kept as an alias). |
| `avx512_sae.asm` | G10 | `{sae}` on vcvttps2qq/vcvttps2uqq (a mistyped parser call made it an illegal instruction). |
| `mpx_bndc.asm` | G11 | bndcl/bndcu/bndcn assemble at all (same mistyped parser call); bytes checked with ZydisInfo. |
| `redefinition.asm` | G12, G13 | Mnemonics shadowed by a second definition: vpmuldq VEX forms, vcvtph2ps/vcvtps2ph EVEX forms, vpmullq's DQ gate. |
| `stacking.asm` | G14 | MMX/SSE forms of movd/movq/pinsrw/pextrw/pmovmskb keep their own gates under the SSE2/SSE4.1 layers stacked over them. |
| `apx_rex2.asm` | APX Phases 0–1 | r16–r31 through REX2: register sizes, R4/X4/B4, SIB/displacement edge registers, map 0/1 (`0Fh` dropped), mandatory/67h/segment prefix order, x87/MMX/SSE memory forms, opcode-register forms (push/pop/mov/xchg/bswap), `{rex2}` and its fallback where Intel forbids REX2 (overruling llvm-mc on movbe/xsave); refusals: APX_F, 64-bit only, ah–bh, VEX/EVEX/XOP/AMX, maps 2/3, msr map 7, XSAVE*, MPX, VSIB. |
| `apx_instructions.asm` | APX Phase 2 | push2/pop2 and PPX forms (APX EVEX layout: B4 not inverted, V4 inverted, ND/U set; operand roles), pushp/popp (REX2.W even for rax–r15), jmpabs, r16–r31 in push/pop register lists; refusals: APX_F, 64-bit only, rsp and pop2 with one register twice (#UD, which llvm-mc encodes), memory and non-64-bit operands. |
| `apx_ndd.asm` | APX Phase 3 | EVEX map 4 of the legacy integer instructions: NDD, `{nf}` after the mnemonic, `{evex}`; operand roles, map 1 → map 4 opcodes, pp for 66h, X4/B4, NDD size for an unsized memory source, imul's NDD vs imm form; legacy forms (04h/05h short forms included) unchanged; refusals: `{nf}` where only NDD exists, unknown options, NDD size, ah–bh, LOCK/REP, APX_F, 64-bit only, the instruction's own feature. |
| `apx_conditional.asm` | APX Phase 4a | cmov<cc> NDD, cfcmov<cc> (load/store/register/NDD forms), set<cc> in map 4 and `{zu}`, imul `{zu}` (with `{nf}`), ccmp/ctest (DFV set or number, SCC incl. t/f), `{evex}` cmp/test; refusals for forms that do not exist. |
| `apx_promoted.asm` | APX Phase 4b | legacy map 2 instructions in EVEX map 4 with r16–r31 or `{evex}` (movbe incl. register form, crc32, adcx/adox incl. NDD, invept/invvpid/invpcid, wrss/wruss, enqcmd(s), movdir64b, movdiri, RAO-INT, urdmsr/uwrmsr incl. map 7 imm form); `use evex` without APX_F leaves legacy instructions alone. |
| `apx_vex.asm` | APX Phase 4c | promoted VEX instructions in APX EVEX form: BMI1/2 (with `{nf}` on andn/bextr/blsi/blsmsk/blsr/bzhi), CMPccXADD, KMOV*, AMX tile config/load/store (incl. `[base, index]`); B4/X4/R4/V4, unscaled disp8, VEX forms unchanged with `use evex` and no APX_F. |
| `apx_vector.asm` | APX Phase 5 | r16–r31 in vector instructions: EVEX base/index/GPR operands (B4, X4, R4), VSIB base, VEX→EVEX when an EVEX form exists (with its AVX-512/VL gates), VEX-only refused. |
| `avx_operand_state.asm` | G20 | vpinsrq/vlddqu assemble first in a source (operand parsed with `AVX_512.parse_operand`). |
| `vpbroadcastq.asm` | G21 | EVEX vpbroadcastq is W1 (was vbroadcasti32x2); masked xmm/ymm vpbroadcastd/q need AVX512VL. |
| `vcompress.asm` | G22 | vcompressps/pd in map 2 (map 1 was undecodable). |
| `sse2_rcp.asm` | G16 | no rcppd/rcpsd/rsqrtpd/rsqrtsd (#UD; the SSE2 list reused rsqrt/rcp); rcpps/rsqrtss and the rest of the SSE2 list keep their bytes. |
| `crc32.asm` | G18 | `crc32 r64, r/m8` carries REX.W. |
| `cet_wrss.asm` | G19 | WRSS/WRUSS take a memory destination only. |
| `cmpxchg16b.asm` | G17 | REX.W in the same REX as the address bits: `cmpxchg16b [r8]` was `48 41 0F C7 08`, which executes as `cmpxchg8b`. |

`_dupnames.py` is a separate check: `python tests/iset/_dupnames.py` fails on any
instruction name defined twice in `include/` that is not a reviewed, deliberate
stack. In fasmg a second definition silently replaces the first for all code
that follows.

`_apx_sweep.py` is the APX check: `python tests/iset/_apx_sweep.py [-v]`
assembles every defined mnemonic with 117 operand patterns that name an
extended register (r16–r31, as register, base or index), with fasm2 and with
llvm-mc (`_apx_diff.py`), and repeats each form fasm2 accepts with six edge
registers. It fails when fasm2 accepts a form with other bytes than llvm-mc,
or one llvm-mc refuses, unless a `REVIEWED` entry gives the reason (each was
checked to behave the same with r8). `-v` lists the forms only llvm-mc has:
the APX encodings still to do (see
[`include/iset/apx-plan.md`](../../include/iset/apx-plan.md)). Re-run it after
any change to an operand parser or an encoder. It takes about a minute.
`python tests/iset/_apx_diff.py FILE` compares any list of lines the same way.
`python tests/iset/_apx_corpus.py [-v] [name ...]` assembles LLVM's APX
Intel-syntax tests (`llvm/test/MC/X86/apx/*-intel.s`, a local checkout) and
compares each encoding; it is the progress measure for the APX phases, not
a regression test (it needs the LLVM tree).
`python tests/iset/_apx_xed.py [-v]` decodes every form fasm2 accepts in the
sweep (also with r8) and in that corpus with XED, Intel's decoder, which
refuses bytes that raise #UD. fasm2 must emit none (policy in
`include/iset/apx-plan.md` §5.3). It needs `_xedcheck.exe`, built from
`_xedcheck.c` against an XED kit (the command is in the file); it takes a
few minutes.
