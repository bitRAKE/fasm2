# fasm2 assembler provider for x64dbg

This package connects fasm2 to x64dbg's `CB_ASSEMBLE` plugin callback. The
32-bit plugin and its tests are written entirely in fasm2 assembly. The plugin
is a standalone merge of the x64dbg provider and the native fasmg DLL system
interface. A future 64-bit plugin can reuse the same provider contract through
a Windows Hypervisor Platform shell.

## Required x64dbg extension

This plugin does **not** work with an unmodified upstream x64dbg build. It
requires the proposed `CB_ASSEMBLE` plugin SDK extension, which is not part of
x64dbg at the time this package was written.

`x64dbg-cb-assemble.patch` contains the minimal required changes against
x64dbg commit `ac70d947124bf1802cf2b86eee483e5ef2d38c48`:

- Add `PLUG_CB_ASSEMBLE` and append `CB_ASSEMBLE` before `CB_LAST`.
- Add `AssemblerEngine::Plugin` and a third `Plugin (fasm2)` dialog choice.
- Dispatch callbacks only when that explicit plugin engine is selected.
- Auto-register a plugin's exported `CBASSEMBLE` function.

Apply it from an x64dbg checkout and rebuild x32dbg:

```cmd
git apply --ignore-space-change C:\git\fasm2\source\x64dbg\x64dbg-cb-assemble.patch
```

Appending the callback preserves the numeric values of all existing callback
types, but plugins must compile against the extended `_plugins.h`. Assembler
engine setting 3 selects plugin providers. The common `assemble(...)` function
in `src/dbg/assemble.cpp` then uses this ordering:

1. Handle x64dbg's built-in data directives.
2. If `AssemblerEngine::Plugin` is selected, call registered `CB_ASSEMBLE`
   providers and return the provider result.
3. Otherwise invoke the selected XEDParse or asmjit backend without calling
   plugin providers.

Plugin selection fails explicitly if no provider is loaded or claims the
instruction; it never silently falls back to another assembler. Consequently
the existing radio selection remains authoritative for the Assemble dialog,
`asm` command, and assembly-pattern searches. No hook inside `XEDParse.dll` is
required.

Build with the repository's fasm2 driver:

```cmd
build_x32.cmd
```

An alternate include root may be supplied as the first argument. The build
writes that path to the output `fasm2.ini`; it never copies the include tree.

```cmd
build_x32.cmd D:\source\fasm2\include
```

The build produces these files under `build\x64dbg-plugin\x32`:

- `fasm2.dp32`: the standalone x32dbg provider and fasmg interface.
- `plugin_test.exe`: an assembly-native callback contract test.
- `fasm2.ini`: startup configuration pointing at the original include tree.

No C/C++ compiler or runtime is involved. CPU compiler flags such as
`-march=native` do not apply to this hand-written assembly package.

The plugin accepts one source line per request. It creates a bounded binary
fasm2 source using `use32` and the supplied instruction address as `org`, so
relative branches are encoded correctly. Requests are serialized because the
embedded fasmg interface reuses its output memory region. The plugin also
exports `fasmg_GetVersion` and `fasmg_Assemble` for tools that need the merged
DLL interface directly.

## Startup configuration

Place `fasm2.ini` beside `fasm2.dp32`. See `fasm2.ini.example` for the complete
form. `IncludeDirectory` is required and should normally be an absolute path to
the authoritative fasm2 `include` directory. Relative paths are resolved from
the plugin directory. No include files are copied into the plugin package.
`StartupSource` supplies one optional source line inserted after `fasm2.inc` is
loaded, and `MaximumPasses` is clamped to the range 1 through 10000.

`StartupSource` is literal assembler source, not a filename. For example, use
`StartupSource=use32` or `StartupSource=include 'my-startup.inc'`; a bare value
such as `StartupSource=fasm2.inc` is parsed as an instruction and fails. Every
request currently has this effective source order:

```asm
include '<IncludeDirectory>/fasm2.inc'
<StartupSource, when non-empty>
format binary
use32
org <requested address>
<requested instruction>
```

Thus x32 mode does not depend on the INI setting: `use32` is always emitted by
the provider immediately before `org`.

## LLVM native x32 TitanEngine note

The local x32dbg build using `-march=native` exposed an unrelated TitanEngine
fault before the Assemble dialog could be tested. `_LocateXStateFeature` may
return packed feature blocks, but TitanEngine cast them to 16-byte-aligned
register types. LLVM consequently emitted faulting `vmovaps` instructions.
The local TitanEngine fix uses a 64-byte-aligned context allocation and
unaligned-safe bounded copies for AVX and AVX-512 feature blocks. This change
belongs in TitanEngine/x64dbg and is separate from `x64dbg-cb-assemble.patch`.

The same native optimization also exposed an x32 GUI alignment bug in
`RegistersView`: Clang folded nominally unaligned SIMD loads into `vmovdqa`
because the public register types promise 16-byte alignment while Qt's x86
object allocation supplied only 8-byte alignment. The local GUI fix removes
the invalid outer alignment promise and keeps the small SIMD detection helpers
under a Clang `optnone/noinline` boundary so their `loadu` operations remain
unaligned-safe. This is likewise a native-build fix, not part of the assembler
provider API patch.

The pass limit catches non-converging assembly, but it is not a wall-clock
timeout. The core has no cooperative cancellation hook, and forcibly killing
an in-process assembler thread can corrupt its heap and output state. A hard
timeout therefore requires an isolated worker process; this should be added
with the WHP shell instead of using unsafe thread termination.

`plugin_test.exe` loads the finished plugin and checks CET instruction bytes,
address-relative encoding, required-buffer reporting, diagnostic propagation,
and callback arbitration. `build_x32.cmd` runs this test automatically.

For the end-to-end host test, copy the build directory beneath x32dbg's
`plugins\fasm2` directory, copy `host_test.txt` there, and run x32dbg's
`headless.exe` with `-testing`, `-plugin`, and `-cf`. The script assembles
`endbr32` and verifies the emitted bytes `F3 0F 1E FB`.
