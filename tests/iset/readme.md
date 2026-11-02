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
