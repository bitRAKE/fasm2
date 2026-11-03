
# Instruction encoding verification (IEV):

When implementing new instruction sets into an assembler, it is good to have some kind of automated or semi-automated testing. One option is to use a third-party disassembler (if there is one) and compare its output with the original source. Another option is to have reference output for a given source and then compare the binary files. But I have been playing with yet another variant - to have the reference output in the source code itself, like:
```asm
use64
vaddps ymm6{k1}{z},ymm12,ymm24		; 62 91 1C A9 58 F0
vsubps zmm1,zmm2,dword [rsi] {1to16}	; 62 F1 6C 58 5C 0E
```
I have made a small module for fasmg that allows to automatically verify with references defined as in the above sample. The test might look like:
```asm
; CPU headers to test:

	include 'cpu/x64.inc'
	include 'cpu/ext/avx512.inc'

; Verification module:

	include 'iev.alm'

; Reference files to test:

	include 'instructions.ref'
```
[IEV.ALM](IEV.ALM):
```
; Instruction Encoding Verifier for fasm g

	retaincomments

	calminstruction calminstruction?.initsym? var*, val&
		publish var, val
	end calminstruction

	calminstruction calminstruction?.asm? command&
		local i, const, tmp
		initsym i, 0
		initsym const, const
		compute i, i+1
		arrange tmp, const#i
		publish tmp, command
		arrange tmp, =assemble tmp
		assemble tmp
	end calminstruction

	calminstruction hex_nibble digit*, command: display
		compute digit, 0FFh and '0123456789ABCDEF' shr (digit*8)
		arrange command, command digit
		assemble command
	end calminstruction

	calminstruction hex_dump data*, command: display
		local	digit, i
		compute i, 0
	    loop:
		local	digit
		compute digit, (data shr (i*8+4)) and 0Fh
		call	hex_nibble, digit, command
		compute digit, (data shr (i*8)) and 0Fh
		call	hex_nibble, digit, command
		compute i, i + 1
		check	lengthof data > i
		jno	done
		local	separator
		arrange separator, command ' '
		assemble separator
		jump	loop
	    done:
	end calminstruction

	calminstruction ? line&
		local comment, i, chunk
		local data, reference

		match ;comment, line
		jyes done

		compute i, $
		match line;comment, line
		assemble line
		jno done

		asm load data:$-i from :i

		arrange reference, =0x
		compute i, 0
	    extract:
		match chunk comment?, comment
		jno completed
		compute i, i + 1
		arrange reference, reference#chunk
		jump extract
	    completed:
		compute reference, reference bswap i

		check data eq reference
		jyes done

		stringify line
		asm display 'Discrepancy:',13,10,9, line, 13,10, 'Assembled: '
		call hex_dump, data
		asm display 13,10, 'Reference: '
		call hex_dump, reference
		asm display 13,10

	    done:
	end calminstruction
```
This of course could be used with other instruction set implementations for fasmg, so if you're working on one, perhaps you may find it helpful.

With fasm 1 processing the same test is a bit harder and the simplest route is to convert the reference into something more fasm-friendly, like the following fasmg-based script does.

[REF2FASM.CMD](REF2FASM.CMD):
```
@goto equ

	MAX_INSTRUCTION_LENGTH := 40

	retaincomments

	macro processor
		PROCESSED = __FILE__
		macro ?! line&
			if __FILE__ = PROCESSED
				match instruction;comment, line
					db '@ ',`instruction,MAX_INSTRUCTION_LENGTH-lengthof `instruction dup ' ',' @ ',`comment
				else
					db `line
				end match
				db 13,10
			end if
		end macro
	end macro

	db "include 'iev.inc'",13,10

	include SRCFILE, processor

:equ

@echo off
if not exist "%~1" goto info

fasmg "%~f0" "%~n1.fasm" -i"SRCFILE='%1'"

goto end

:info

echo Please provide a name of file containing x86 test reference.
echo This tool converts the reference into IEV test for fasm.

:end
```
This is a source file for fasmg that also doubles as a Windows batch file that launches the process automatically (it expects fasmg to be in PATH environment variable, but can easily modified to use full path instead). It converts the reference into a source that looks like:
```asm
include 'iev.inc'
use64
@ vaddps ymm6{k1}{z},ymm12,ymm24           @ 62 91 1C A9 58 F0
@ vsubps zmm1,zmm2,dword [rsi] {1to16}     @ 62 F1 6C 58 5C 0E
```
And this is then much easier for fasm 1 to process, just a single macro suffices.

[IEV.INC](IEV.INC):
```
; Instruction Encoding Verifier for fasm 1

virtual at 0
	__hex:: db '0123456789ABCDEF'
end virtual

macro @ line&
{
	local proxy, offset, length, reference, reference_length, a, b, c
	define proxy line
	match instruction =@ comment, proxy
	\{
		offset = $ - $$
		instruction
		length = $ - $$ - offset
		virtual at 0
			reference::
			irps chunk, comment
			\\{
				db 0x\\#chunk
			\\}
			reference_length = $
		end virtual
		c = 0
		if length = reference_length
			repeat length
				load a byte from $$+offset+%-1
				load b byte from reference:%-1
				if a <> b
					c = 1
					break
				end if
			end repeat
		else
			c = 1
		end if
		if c
			display 'Assembled:'
			repeat length
				load a byte from $$+offset+%-1
				load b byte from __hex:a shr 4
				load c byte from __hex:a and 0Fh
				display ' ',b,c
			end repeat
			display 13,10,'Reference:'
			repeat reference_length
				load a byte from reference:%-1
				load b byte from __hex:a shr 4
				load c byte from __hex:a and 0Fh
				display ' ',b,c
			end repeat
			display 13,10
			err
		end if
	\}
}
```
This one is probably not very useful, though, unless you plan to tweak the source of fasm 1 or implement instructions of some other CPU, etc. Instead of being tested, fasm's output could itself become a reference. And I have made another script that does exactly that.

[ASM2REF.CMD](ASM2REF.CMD):
```
@goto equ

	MAX_INSTRUCTION_LENGTH := 40

	virtual at 0
		opcodes:: file BINFILE
		TOP_OFFSET = $
	end virtual

	virtual at 0
		file FASFILE

		load preprocessed_offset:dword from 32
		load dump_offset:dword from 40
		load dump_length:dword from 44
		PREVIOUS_OFFSET = 0
		LINE_NUMBER = 0
		repeat dump_length/28, i:0
			load CURRENT_OFFSET:dword from dump_offset+i*28
			if (CURRENT_OFFSET>PREVIOUS_OFFSET)
				repeat 1, n:LINE_NUMBER
					load DATA#n:CURRENT_OFFSET-PREVIOUS_OFFSET from opcodes:PREVIOUS_OFFSET
				end repeat
				PREVIOUS_OFFSET = CURRENT_OFFSET
			end if
			load line_offset:dword from dump_offset+i*28+4
			load LINE_NUMBER:dword from preprocessed_offset+line_offset+4
		end repeat
		if (TOP_OFFSET>PREVIOUS_OFFSET)
			repeat 1, n:LINE_NUMBER
				load DATA#n:TOP_OFFSET-PREVIOUS_OFFSET from opcodes:PREVIOUS_OFFSET
			end repeat
		end if
	end virtual

	calminstruction hex_nibble digit*, command: db
		compute digit, 0FFh and '0123456789ABCDEF' shr (digit*8)
		arrange command, command digit
		assemble command
	end calminstruction

	calminstruction hex_dump data*, command: db
		local	digit, i
		compute i, 0
	    loop:
		local	digit
		compute digit, (data shr (i*8+4)) and 0Fh
		call	hex_nibble, digit, command
		compute digit, (data shr (i*8)) and 0Fh
		call	hex_nibble, digit, command
		compute i, i + 1
		check	lengthof data > i
		jno	done
		local	separator
		arrange separator, command ' '
		assemble separator
		jump	loop
	    done:
	end calminstruction

	macro processor
		PROCESSED = __FILE__
		macro ?! line&
			if __FILE__ = PROCESSED
				db `line
				repeat 1, n:__LINE__
					if defined DATA#n
						db MAX_INSTRUCTION_LENGTH-lengthof `line dup ' ', '; '
						hex_dump DATA#n
					end if
				end repeat
				db 13,10
			end if
		end macro
	end macro

	include SRCFILE, processor

:equ

@echo off
if not exist "%~1" goto info

fasm "%~1" "%~n1.bin" -s "%~n1.fas"
fasmg "%~f0" "%~n1.ref" -i"SRCFILE='%1'" -i"BINFILE='%~n1.bin'" -i"FASFILE='%~n1.fas'"
del "%~n1.bin"
del "%~n1.fas"

goto end

:info

echo Please provide a name of file containing x86 instructions.
echo This tool assembles them with fasm and generates a test reference.

:end
```
This one was really fun to craft - again it combines fasmg source with a batch file, but this time it also uses the .FAS file generated with fasm to combine the source lines with bytes extracted from fasm-generated output. The reference it makes looks exactly as the ones shown here in the beginning.

On a final one, I feel it's important to add that this kind of testing is usually just a "sanity check", to make sure that nothing is obviously broken in your instruction encoders. In my experience bugs may be very context-dependent and can be easily missed by such basic testing. Therefore in fasm development my release process usually included verifying several real projects in addition to a collection of snippets (starting with reassembling itself, which has always been the most obvious sanity check for fasm).

---

## IEV in fasm2

The post above is Tomasz Grysztar's, with his files (`iev.alm`,
`IEV.INC`, `REF2FASM.CMD`, `ASM2REF.CMD`, `instructions.ref`). In fasm2 the
form runs against this tree's `include/`, in two directions:

| Folder | Line form | Checked by | Meaning |
|---|---|---|---|
| `pass/` | `instruction ; HEX BYTES` | `iev.alm` | assembles to exactly these bytes |
| `fail/` | `instruction ; 'message'` | `fail.alm` | stops with an error containing the message |

A `fail/` line without a comment is setup and must assemble without error
(for example the valid line that leaves an immediate behind before the line
that must not reuse it). fasmg cannot catch an error inside the source, so
`fail.alm` only drops the comment and assembles the line; the runner maps
fasmg's error reports to the lines.

| File | What |
|---|---|
| `sample.asm` | his `fasmg/packages/x86-2/test.asm` (`{settings}` per line: `{i286}`, `{rmsrc}`, `{imm32}`, `{AVX512}` ...) |
| `test.asm` | the post's `test.asm` with `pass/instructions.ref` |
| `pass/instructions.ref` | the post's reference: moffs, `ebp+ebp`, EVEX masking and broadcast |
| `pass/boundaries.ref` | encoding boundaries, for correctness and for the shortest form: disp8 and disp8\*N edges, rbp/r13 and rsp/r12 bases, index-only addresses, default segments, moffs, immediate widths, accumulator and short forms, jump ranges, VEX2/VEX3/EVEX choice, in 16-, 32- and 64-bit code |
| `fail/policy.ref` | what must be refused: #UD forms, unmet `{vex2}`/`{vex3}`/`{evex}`/`{rex2}`, `{nf}`/`{zu}`/`{dfv=}` without that form, stale operand state, operand classes and sizes, values out of range |
| `fail/absent.ref` | every mnemonic XED has and fasm2 does not (145 on 2026-11-03: AVX10.2, AMX FP8/TF32/MOVRS, ACE, MOVRS, PBNDKB, IBHF, UDB ...), each with a real form from LLVM's Intel-syntax MC tests where LLVM has one; generated by `_absent.py` (needs the XED and LLVM trees) |
| `fail/absent_forms.ref` | missing forms of instructions fasm2 has (MSR-IMM, AVX-512 VL gaps, AVX10.2 EVEX forms of VEX instructions), and what is refused by design (implied shift count, ccmp/ctest without `{dfv=}`) |
| `pass/iset/*.ref`, `fail/iset/*.ref` | the byte checks and the `FAIL n` cases of `tests/iset/*.asm`, generated by `_iset2ref.py` |
| `fail.alm` | the fail-form module (fasm2's companion to `iev.alm`) |
| `_run.ps1` / `_run.py` | runs all of them |
| `_ref_check.py` | checks a `pass/` reference against llvm-mc and XED: fasm2 shorter, longer or equal to llvm-mc, XED decoding every line in its mode, jump targets through XED; `--write` fills in fasm2's bytes after review |

```
powershell -File tests\IEV\_run.ps1
python tests\IEV\_ref_check.py pass\boundaries.ref   fasm2 vs llvm-mc and XED, line by line
python tests\IEV\_iset2ref.py              regenerate pass\iset, fail\iset after changing tests\iset
python tests\IEV\_absent.py               regenerate failbsent.ref (XED and LLVM trees needed)
```

fasm2's `x86-2.inc` is included through `fasm2.inc`: its encoders call the
data directives `word`, `dword`, `qword` from `dd.inc`, which `fasm2.inc`
includes first. Included alone (as fasmg's self-contained package is),
`{imm32} add ecx,1` stops with "symbol '@dest.dword' is undefined".

`pass/iset` and `fail/iset` are a second form of the same checks, not a
second source of truth: `tests/iset` stays where they are written.
`_iset2ref.py` takes each test's top-level `use` and `expect` lines, and
each `FAIL n` case whose lines are plain (with the test's header settings
and a reset of the encoding preferences before it); lines in loops or using
the test's own helper macros stay iset-only. `_run.ps1` fails if a
generated file is out of date. Each `.ref` is assembled on its own, with
`fasm2.inc` and the module given on the command line, so settings such as
`use evex` cannot leak from one file into the next.

`iev.alm`'s pattern needs comment text, so it assembles a line holding only
`;` as an instruction; leave such lines empty in `pass/` (`fail.alm`
matches `comment?` and takes them).

### What `boundaries.ref` found (2026-11-03)

Against llvm-mc 23, with XED decoding every line in its mode: 117 lines
identical, 20 where fasm2 is **shorter** (index-only addresses as
base+index, a restated default segment dropped, the `ebp+ebp` swap, 32-bit
moffs with 67h), 10 jumps whose targets XED confirms and which use rel8
whenever it fits, 1 same-length alternative. Four where llvm-mc is shorter:

| Line | fasm2 | llvm-mc | Verdict |
|---|---|---|---|
| `vaddps xmm0,xmm1,xmm8` | `C4 C1 70 58 C0` | `C5 B8 58 C1` | llvm-mc swaps the sources to reach VEX2. Not exact: with two NaN inputs x86 returns the first source's NaN. fasm2 is right. |
| `vmovaps xmm0,xmm8` | `C4 C1 78 28 C0` | `C5 78 29 C0` | the store-direction opcode puts xmm8 in ModRM.reg, where VEX2 has R. Same result, 1 byte: open. |
| `xchg eax,eax` (32-bit) | `87 C0` | `90` | `90` is `xchg eax,eax` in 32-bit code (not in 64-bit, where it would not zero rax's upper half; there both write `87 C0`). 1 byte: open. |
| `xchg rax,rax` | `48 90` | `90` | same operation, 1 byte: open. |

It also found fasm2's own rbp swap missing for r13 (and r21/r29):
`[r13+rcx]` took 5 bytes where `[rcx+r13]` takes 4. Fixed: in 64-bit code
a base with low bits 101 trades places with a scale-1 index that has other
low bits.

### Open: values below the signed minimum (2026-11-03)

fasmg's `emit` takes a value from -2^n to 2^n-1 for an n-bit field, so a
value below the signed minimum wraps instead of failing: `db -129` is `7F`
(`db -257` is refused), and through it `add al, -81h` assembles as
`add al, 7Fh`, `add ax, -8001h` as `add ax, 7FFFh`. fasm 1 refuses
`db -129`. The engine's range may be deliberate (it lets `db -0FFh` mean the
two's complement of an unsigned value); for instruction immediates, where
the value has one meaning, fasm2 could check -2^(n-1) .. 2^n-1 itself.
Undecided; `fail/policy.ref` leaves these lines out until it is.
