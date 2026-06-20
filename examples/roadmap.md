# Examples roadmap

This roadmap collects examples that would add practical value to the fasm2 Win32/Win64 teaching set. The goal is not to make every example large. Small single-purpose examples are still the right shape for isolated mechanisms. The larger examples should earn their size by producing reusable modules with clear semantics.

## Selection Criteria

Prefer new or expanded examples when they demonstrate at least one of these:

- a reusable Win32 runtime module that can move into another program;
- a message-loop shape that is easy to get wrong;
- resource, manifest, or PE metadata behavior that is hard to infer from docs;
- PSDK translation pressure: functions, structures, constants, COM interfaces, callbacks, or notification payloads that should become local includes;
- a useful debugging, profiling, or inspection workflow;
- a compact single-file technique that is better studied without abstraction.

Avoid examples that only wrap an API once without teaching ownership, lifetime, state, message routing, or build integration.

## Near-Term Expansions

### `msgflood`: Profile-Guided Message Routing

`msgflood` is already more than a message log. It is a natural base for a profile-guided optimization example because it can observe real message traffic, classify it, and then use those observations to tune routing.

Useful expansion:

- add a profiling mode that counts each `msg` value and optionally each `msg`/`wParam` class;
- export or display a sorted hot-message table;
- document how common idle traffic differs from menu, drag/drop, resize, raw input, tray, and RichEdit scenarios;
- refactor the window procedure from a naive compare chain into a tuned route: hot messages first, rare messages table-driven, and default traffic falling through quickly;
- keep before/after code side by side or under build switches so readers can inspect what changed;
- measure code size and branch layout in addition to speed, because the main teaching value is routing clarity under real traffic.

This would demonstrate a useful assembly habit: observe the workload first, then shape the dispatch code around the observed message distribution.

### `modern_rc`: Resource and Dialog Growth Path

`modern_rc` is the right place to keep expanding resource-backed application structure. Good next steps:

- add a string table and move user-visible menu/dialog/message text out of source;
- add a second modeless dialog to show routing for multiple live dialogs;
- add a status bar through common controls v6;
- add recent search strings as a second MRU-backed module;
- add drag/drop feedback and explicit rejection messaging for multi-file drops;
- document how command IDs are shared between RC, accelerators, dialogs, and fasm2 routing.

### `uah_menu`: Popup Menu Subclassing Matrix

`uah_menu` can grow into the reference for native menu theming boundaries:

- compare main-menu, context-menu, tray-menu, and RichEdit proofing-menu popup lifetimes;
- document which popup paths require the CBT hook and which are under direct application control;
- add a small diagnostics mode that shows which `WM_UAH*` messages were handled without turning the example back into a logger;
- split the painter policy from the hook policy even more sharply so a new app can reuse only the hook support;
- add high-DPI checks for menu metrics and icon font sizing.

### `gdi_snake`: Timing, Rendering, And Persistence Variants

`gdi_snake` has several reusable modules already. Useful expansions:

- add a fixed-step simulation accumulator to compare with timer-per-tick updates;
- add a paint timing overlay that can be toggled during attract mode;
- add a replay recorder using compact input/tick records;
- add a second high-score storage backend, such as registry or DPAPI-protected file, to compare persistence contracts;
- add keyboard layout notes for WASD and arrow-key input.

### `font_icons`: Promote Reusable Icon Surfaces

The font-icon work can become a stronger bridge between tools and app code:

- move stable helper interfaces toward a reusable include once the API settles;
- add a small "icon button strip" example separate from the full browser tools;
- add DPI/theme recreation rules with one short sample that recreates fonts, cursors, and cached bitmaps correctly;
- add a resource export path for generated icon catalogs.

## New Example Candidates

### PSDK Inquiry Tool Demonstrations

The repository direction points toward local PSDK inspection and translation.
That deserves examples that show how generated knowledge lands in an application:

- function inquiry: request a Win32 API by name, update a local `windows.inc` with constants, prototype, dependent structures, enums, and import binding;
- structure inquiry: translate one SDK structure plus nested dependencies and verify size/offsets against a small C probe;
- COM inquiry: generate `include\com\<interface>.inc` for one interface, including GUIDs, vtable layout, method prototypes, and a tiny consumer.

Good starter targets:

- `IFileOpenDialog` for COM interface generation;
- `SHFILEOPSTRUCT` or `NOTIFYICONDATA` for large structure translation;
- `GetDpiForWindow`, `AdjustWindowRectExForDpi`, and `WM_DPICHANGED` for a DPI-focused function set.

### DPI-Aware Window Topology

Create a compact example dedicated to per-monitor DPI:

- manifest declares DPI awareness;
- main window reacts to `WM_DPICHANGED`;
- child controls and fonts are rebuilt from DPI-scaled metrics;
- saved window placement is restored safely across monitor changes;
- a small overlay shows current DPI, monitor bounds, and suggested rectangle.

This would complement the resource examples and prevent DPI handling from being scattered as incidental code.

### Clipboard And Data Exchange

A clipboard example would expose several tricky ownership rules:

- Unicode text and custom registered formats;
- delayed rendering with `WM_RENDERFORMAT`;
- clipboard viewer/listener registration;
- drag/drop data object comparison if COM support is available;
- safe allocation ownership through `GlobalAlloc`/`SetClipboardData`.

The example can start as text-only and later grow into a COM `IDataObject` variant.

### Shell Integration

A small shell-facing example would cover APIs that often appear together:

- `ShellExecuteEx` with error reporting;
- `IFileOperation` or a simpler SHFileOperation-era comparison;
- known folders and PIDLs;
- file association lookup;
- jump-list or recent-document registration.

This pairs well with the PSDK inquiry direction because Shell APIs bring many structures and COM interfaces.

### Common Controls Gallery

Create a resource-backed common-controls gallery with one tab per control:

- list-view with image lists, custom draw, sorting, and virtual mode;
- tree-view with lazy expansion;
- toolbar/rebar/status bar with DPI-aware metrics;
- trackbar/up-down/progress controls;
- notification routing through `WM_NOTIFY` structures.

Keep it practical: the point is not a visual catalog, but a set of reusable notification and state-management patterns.

### RichEdit Deep Dive

The `uah_menu` RichEdit surface proves proofing and popup menus. A separate RichEdit example could focus on the control itself:

- text mode ordering and empty-control requirements;
- URL detection, `EN_LINK`, and friendly-name links;
- streaming plain text and RTF in/out;
- selection formatting with `CHARFORMAT2` and paragraph formatting;
- undo grouping and modified-state handling;
- optional OLE callback boundaries if the supporting interfaces are generated.

### Async I/O And IPC

A Win32 assembly example for asynchronous boundaries would fill a real gap:

- overlapped file reads with an event or IO completion port;
- named pipe client/server pair;
- cancellation and shutdown ordering;
- message-only window or posted completion notifications to a GUI thread.

This should stay small, but it would clarify handle ownership and lifetime rules better than prose.

### Thread Pool And Timers

The examples currently show message timers and waitable timers. A thread-pool example would round out the options:

- `CreateThreadpoolWork`, `CreateThreadpoolTimer`, and cleanup groups;
- GUI-thread marshaling through `PostMessage`;
- cancellation and "wait for callbacks" shutdown;
- comparison with a hand-created worker thread.

### PE Metadata And Loader Behavior

The loader metadata experiments should be distilled into a public, repeatable example set:

- subsystem version variants;
- `.pdata`, `.reloc`, load config, security cookie, and high-entropy VA;
- manifest variants for `asInvoker`, `highestAvailable`, and `requireAdministrator`;
- scripts that run `dumpbin`, `mt.exe`, and expected-result checks.

The value is not the executables themselves, but the reproducible matrix and the habit of inspecting PE headers instead of guessing.

### Localization And Resource Selection

Add a small GUI with English and one secondary language:

- string tables, dialogs, menu text, and version resources;
- `FindResourceEx` or thread preferred UI language selection;
- fallback behavior when a resource is missing;
- layout pressure from translated strings.

This would make the resource pipeline more complete without depending on a large application.

### Service Or Scheduled Task Control

A service example would cover a different Windows program shape:

- service entry, control handler, stop event, and status updates;
- install/uninstall helper mode;
- event log reporting or ETW stub;
- privilege and session-zero notes.

This should be carefully scoped because services are easy to overbuild.

### Winsock Event Loop

A networking example can stay practical without becoming a framework:

- nonblocking TCP echo client/server;
- `WSAEventSelect` or IOCP comparison;
- graceful shutdown and half-close handling;
- IPv4/IPv6 address parsing with `GetAddrInfoW`.

### SIMD Dispatch

`win64avx512` proves AVX-512 instruction encoding and CPU probing. A more application-shaped SIMD example could show:

- CPUID/XGETBV dispatch table;
- scalar, SSE2, AVX2, and AVX-512 implementations of one tiny workload;
- alignment and tail handling;
- measured result display.

This keeps instruction-set examples connected to runtime feature selection.

## Documentation And Tooling Ideas

- Add a root examples index check that warns when a directory has an entry source but no README or table entry.
- Add a build smoke script that runs all examples with `_build.cmd` and reports which require SDK tools.
- Add an "example contract" template: purpose, build, files, runtime state, message map, reusable pieces, extension points.
- Add generated include verification probes for PSDK-translated structures: assemble-time offsets plus optional C `static_assert` comparisons.

## Priority Cut

Highest leverage:

1. `msgflood` profile-guided message routing.
2. PSDK inquiry demonstrations for functions, structures, and COM interfaces.
3. DPI-aware window topology.
4. Common-controls notification gallery.
5. RichEdit deep dive.

These fill gaps that normal Win32 examples often leave vague: real message traffic, generated API knowledge, DPI correctness, `WM_NOTIFY` payloads, and stateful controls with strict setup ordering.
