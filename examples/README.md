# fasm2 examples

This directory is a mixed collection of small demonstrations, larger Win32
study programs, and a few local workbench directories. The older examples tend
to be single-file programs that make one technique easy to inspect. The newer
examples are intentionally modular: each include file owns a reusable piece of
application behavior while still keeping the Win32 API calls visible.

## Building

Most Windows examples expect the repository-local wrapper:

```cmd
cd C:\git\fasm2\examples\<example>
_build.cmd
```

Examples that use resources need the Windows SDK resource compiler, so run the
build from a Visual Studio developer prompt where `rc.exe`, `link.exe`, and the
SDK include paths are available. Single-file examples without `_build.cmd`
can usually be assembled directly with:

```cmd
..\..\fasm2.cmd -e 5 example.asm
```

Some classic samples use plain `fasm2` and an external linker response file.
Check the source header or local README before assuming a uniform build path.

## Suggested Reading Order

1. `globstr` - smallest macro/string demonstration.
2. `conio` - MS64 COFF build, generated linker response, custom ABI syntax,
   and console event probing.
3. `modern` - first modular Win64 GUI editor.
4. `modern_rc` - resources, accelerators, modeless dialogs, and shell drops.
5. `msgflood` - message-observation harness and edit-control logging.
6. `gdi_snake` - reusable GDI board/game/high-score modules.
7. `uah_menu` - owner-painted native popup menu theming and RichEdit proofing.
8. `font_icons` - local icon-font helper package and richer GUI tooling.

The first item is a compact macro/string example. `conio` is a deeper language
experiment that happens to use the console as its runtime surface. The later
items show how larger assembly applications can grow by refining reusable
modules instead of becoming one anonymous source file.

## Curated Examples

| Directory | Main file | Focus |
| --- | --- | --- |
| `conio` | `conio.asm` | MS64 COFF object build with generated linker response settings, custom `::Function` Win32 ABI invocation, late data-block gatherers, CALM output helpers, and a console input-event probe. |
| `font_icons` | `font_icon_demo.asm`, `uwpchar.asm`, `glyphset.asm` | Segoe icon-font helpers, toolbar/static/glyph cursor rendering, layered glyph composition, and resource-backed GUI tools for exporting icon constants. |
| `gdi_snake` | `gdi_snake.asm` | Borderless GDI Snake game with stretch-blit board rendering, attract-mode AI, queued bonuses, arcade high scores, and tamper-evident score storage. |
| `globstr` | `demo_windows.asm`, `demo_linux.asm` | `macro/globstr.inc` usage for inline/global string data on Windows and Linux. |
| `modern` | `modern.asm` | Modular Unicode Win64 text editor with local `windows.inc`, `common.inc`, UTF-8 file I/O, and comctl32 MRU ordinal binding. |
| `modern_rc` | `modern.asm`, `modern.rc` | Resource-backed companion to `modern`: shared `resource.h`, manifest, version info, accelerator table, modeless Find dialog, and single-file drag/drop open. |
| `msgflood` | `msgflood.asm` | Win32 message-discovery harness that inserts newest log rows at the top of an edit control, filters noisy sources, and coalesces adjacent identical messages. |
| `opengl` | `opengl.asm` | Minimal Win64 OpenGL 3.3 window, WGL extension loading, shader compilation, VBO/VAO setup, and frame painting. |
| `TaskDialog` | `TaskDialog.asm` | Modular Win64 port of TaskDialog samples, including command links, progress, navigation, callbacks, and an async-operation workflow. |
| `tetros` | `tetros.asm` | 512-byte-style boot-sector Tetris for VGA/i386 environments, suitable for DOSBox boot testing. |
| `uah_menu` | `uah_menu.asm`, `uah_menu.rc` | UAH-themed native popup menus through a CBT hook, tray menu, themed RichEdit context/proofing menus, resource menus, accelerators, and dark/light menu policy. |
| `win64avx512` | `win64avx512.asm` | AVX-512 feature probing and ZMM/k-mask instruction playground with a GUI result display. |

## Modern Modular Examples

`modern`, `modern_rc`, `gdi_snake`, `msgflood`, and `uah_menu` are the best
examples to study when writing new Win32 assembly code in this tree. Their
shape is deliberate:

- entry sources are thin and mostly include modules;
- module data/BSS ownership stays near the procedures that use it;
- local `windows.inc` or `include/addon/windows.inc` files define executable
  policy and finalization;
- resources use a shared ID file when both RC and fasm2 need the same values;
- README files explain the runtime contracts, not just how to build.

This style is meant to make modules learnable as large semantic pieces:
MRU-backed recent files, modeless dialog routing, board rendering, high-score
storage, message logging, RichEdit setup, tray handling, and popup-menu
subclassing can each be carried into another program and refined.

## Example Notes

### `conio`

`conio` is unique in the curated set because it is both a console event probe
and a fasm2/fasmg language-extension study. It emits a Microsoft x64 COFF
object, generates `conio.response` from assembly source, and lets `link.exe`
produce the final executable. Its local syntax records imported functions and
libraries through `::Function` and `:library:Function`, gathers data through
`≡`, `∵`, and `□` blocks, and uses CALM line interceptors for immediate
`WriteFile` output and buffered text buildup. The runtime console work remains
useful, but the main lesson is how source-level notation can make object-file
linking, import binding, data ownership, and Win64 call-frame sizing explicit.

### `modern`

`modern` is a study-sized Unicode text editor. It demonstrates UTF-8 file
load/save, dirty-state prompts, code-built menus, and a persistent recent-file
menu backed by the undocumented comctl32 MRU ordinals. The local modules are
small enough to read independently: `file_io.inc`, `dialogs.inc`,
`mru_recent.inc`, and `ui_main.inc` each own a clear feature boundary.

### `modern_rc`

`modern_rc` keeps the editor/MRU behavior from `modern` but moves UI identity
into resources. It is the reference example for a shared `resource.h`, RC menu
resources, accelerator tables, version information, an XML manifest, a modeless
Find dialog, and `WM_DROPFILES` handling. Its message loop shows the required
ordering for `TranslateAcceleratorW` and `IsDialogMessage`.

### `gdi_snake`

`gdi_snake` is a game, but the teaching value is the runtime structure. The
board renderer owns the off-screen bitmap and stretch blit; game logic owns
cells, snake state, apple effects, queued bonuses, and attract-mode AI; the
high-score module owns ranked rendering and validation. The window can be
resized before play to change difficulty, then the board size is locked for the
active game.

### `msgflood`

`msgflood` is useful when exploring which messages Windows sends for a feature.
It registers for raw input, shell hooks, clipboard updates, power/session
events, tray callbacks, hotkeys, drops, timers, and UIPI filter changes. The log
is a normal edit control, but new rows are inserted at character zero so the
latest message stays visible. The filter menu and adjacent-message coalescing
keep high-volume sources readable.

### `uah_menu`

`uah_menu` demonstrates themed native popup menus. The reusable support is split
between `include/addon/uah.inc` for UAH constants/structures/wrappers and
`include/subclass/uah_menu.inc` for the CBT hook that discovers transient
`#32768` menu windows. The executable owns the visual policy and hosts a
RichEdit control so the main menu, tray menu, and RichEdit context/proofing menu
all exercise the same popup painter.

### `font_icons`

`font_icons` is a local package for Segoe Fluent Icons, Segoe MDL2 Assets, and
Segoe UI Symbol experimentation. It includes helper procedures for creating
icon fonts, drawing single and layered glyphs, toolbar custom draw, static
controls, and glyph-backed cursors. `uwpchar.exe` and `glyphset.exe` are richer
tools for browsing glyphs, composing layers, and exporting fasm constants.

### Classic Technique Samples

`globstr`, `opengl`, `tetros`, and `win64avx512` are closer to traditional
single-purpose assembly examples. They are valuable because the whole technique
is visible in one file: inline global strings, OpenGL setup, boot-sector game
layout, and CPU feature probing respectively.

## When Adding A New Example

Prefer the smallest structure that teaches the idea:

- use a single source file for isolated API, instruction, or format behavior;
- use modules when the example teaches reusable program pieces;
- include `_build.cmd` when the build needs more than direct assembly;
- include a local README when behavior, state, message-loop shape, resources,
  or runtime contracts are not obvious from one source file;
- keep resource IDs in one shared file when both RC and fasm2 need them;
- leave generated `.exe`, `.res`, and other build outputs out of source
  history unless the example intentionally studies binary outputs.
