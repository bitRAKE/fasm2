# fasm2 structure tools

Python static-analysis helpers for understanding the `bitRAKE/fasm2` `win32` branch at a higher level.

The first pass is intentionally heuristic and source-preserving. It does not try to fully expand fasm2/fasmg macros. Instead it extracts enough structure to guide multi-file implementation work:

- function/data graph: calls, ABI calls, unresolved/indirect calls, and data references
- ABI pressure score: integer `abi_calls + parameter_uses_after_abi_call`
- hierarchy/layering: Tarjan SCCs and leaf-first condensation layers

## Baseline

This repository was cloned from:

- upstream: `https://github.com/bitRAKE/fasm2`
- branch: `win32`
- baseline commit at project creation: `82abfef add a compact wParam/lParam lookup table with footnotes for the messy cases`

## Run

From the repo root:

```sh
python -m fasm2_structure . source/windows examples tests/ntdll --report
```

Outputs are written to `analysis/` by default:

- `analysis/structure.json` — symbols, edges, and function ABI pressure metrics
- `analysis/layers.json` — Tarjan SCCs and leaf-first implementation layers
- `analysis/structure.dot` — Graphviz DOT for the function/data graph
- `analysis/report.html` — self-contained interactive structure report
- `analysis/report-data.json` — browser/report data model
- `analysis/mermaid/scc-condensation.mmd` — focused SCC condensation graph
- `analysis/mermaid/module-graph.mmd` — module/directory dependency graph
- `analysis/mermaid/top-pressure.mmd` — top ABI-pressure neighborhood snapshot

Internal calls made through ABI-style macros are emitted as `abi-call`: they contribute to both ABI pressure and the function dependency graph.

Use a narrower path while iterating:

```sh
python -m fasm2_structure source/windows/fasmg.asm source/windows/system.inc --out analysis/windows-core
```

## ABI pressure interpretation

The primary implementation-dynamics metric is now an integer:

```text
abi_pressure = abi_calls + parameter_uses_after_abi_call
```

`abi_pressure = 0` means a pure leaf routine: no ABI calls were detected, so no ABI frame is needed by the heuristic. This gives pure leaf functions a distinct control surface for code-generation dynamics.

`abi_pressure = 1` commonly means one ABI boundary with no detected parameter survival afterward. Check `pressure_class` and `tail_abi_calls` to distinguish tail-call wrappers from normal ABI boundaries.

Higher values mean more ABI boundaries and/or more evidence that incoming parameter/register state survives across them. This acts as a simple register/frame pressure signal for implementation planning.

The analyzer also emits `pressure_class`:

- `pure_leaf` — `abi_pressure == 0`
- `tail_abi` — all ABI interactions are tail-position and no parameters survive afterward
- `abi_boundary` — ABI calls exist, but no parameter survival is observed
- `abi_state_pressure` — parameter evidence survives after ABI calls; implementation may need frame/register choreography

The component counts remain in JSON so ranking can be adjusted later without losing evidence.

Current parameter evidence includes:

- declared `proc` parameters when visible
- common ABI parameter registers (`rcx`, `rdx`, `r8`, `r9`, `ecx`, `edx` variants)
- stack parameter patterns such as `[rbp+...]` / `[ebp+...]`

## Layering interpretation

`layers_leaf_first` in `layers.json` is ordered for implementation work:

- layer 0: routines with no internal function dependencies
- higher layers: routines that depend on previous layers
- SCC entries with multiple functions are recursive/mutually recursive islands and should be planned together

## Visual report

Use `--report` to generate visual tooling aimed at making poor structure decisions obvious. The HTML report is static and can be opened directly in a browser.

The report includes:

- SCC/layer condensation view, colored by ABI pressure
- module graph, aggregated by top source/example/test subdirectory
- function neighborhood view with selectable depth
- filters for search text, minimum ABI pressure, and hiding pure leaves
- pressure table synchronized with graph focus
- structure smell list for high ABI pressure, high pressure in leaf layers, recursive SCCs, and cross-file SCCs

The Mermaid files are intentionally focused snapshots. They are useful for documentation and quick sharing, while the HTML report is better for choosing the right depth interactively.

## Known limitations

- Macro expansion is not performed; edges are lexical evidence.
- Local labels are not modeled as separate functions unless they start at column 0.
- ABI recognition is conservative: `invoke`/`stdcall`/`fastcall`/`ccall`, external symbol calls, and bracketed indirect calls are treated as ABI-ish or unresolved as appropriate.
- Data references are best-effort identifier matches against known data labels/directives.
