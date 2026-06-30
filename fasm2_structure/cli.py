from __future__ import annotations

import argparse
import json
from pathlib import Path

from .analysis import build_structure
from .asm_parser import parse_tree
from .graph import layering_to_dict, model_to_dict, write_dot, write_json
from .report import write_report


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Analyze fasm2/fasmg assembly source structure.")
    p.add_argument("root", nargs="?", default=".", help="Repository/source root to scan")
    p.add_argument("paths", nargs="*", help="Optional files/directories relative to root")
    p.add_argument("--out", default="analysis", help="Output directory for JSON/DOT artifacts")
    p.add_argument("--format", choices=["summary", "json"], default="summary", help="stdout format")
    p.add_argument("--no-dot", action="store_true", help="Do not write Graphviz DOT")
    p.add_argument("--report", action="store_true", help="Write interactive HTML report and Mermaid focused graphs")
    p.add_argument("--limit", type=int, default=20, help="Rows to print in summary tables")
    return p


def summarize(model, layers, limit: int) -> str:
    functions = [s for s in model.symbols.values() if s.kind == "function"]
    data = [s for s in model.symbols.values() if s.kind == "data"]
    external = [s for s in model.symbols.values() if s.external]
    recursive = layers["recursive_sccs"]
    pressured = sorted(model.metrics.values(), key=lambda m: (-m.abi_pressure, -m.parameter_uses_after_abi_call, m.name))[:limit]
    pure = sorted((m for m in model.metrics.values() if m.abi_pressure == 0), key=lambda m: m.name)[:limit]
    hottest = sorted(model.metrics.values(), key=lambda m: (-m.total_calls, m.name))[:limit]
    lines = []
    lines.append(f"functions={len(functions)} data={len(data)} externals={len(external)} edges={len(model.edges)}")
    lines.append(f"sccs={layers['scc_count']} recursive_sccs={len(recursive)} layers={len(layers['layers_leaf_first'])}")
    lines.append("")
    lines.append("highest ABI pressure (abi_calls + parameter_uses_after_abi_call):")
    for m in pressured:
        lines.append(f"  pressure={m.abi_pressure:<3} class={m.pressure_class:<18} after={m.parameter_uses_after_abi_call:<3} abi={m.abi_calls:<3} tailABI={m.tail_abi_calls:<2} calls={m.total_calls:<3} {m.name} ({m.path}:{m.line_no})")
    lines.append("")
    lines.append("sample pure leaves (abi_pressure=0):")
    for m in pure:
        lines.append(f"  pressure={m.abi_pressure:<3} class={m.pressure_class:<18} calls={m.total_calls:<3} {m.name} ({m.path}:{m.line_no})")
    lines.append("")
    lines.append("highest call fan-out:")
    for m in hottest:
        lines.append(f"  calls={m.total_calls:<3} internal={m.internal_calls:<3} abi={m.abi_calls:<2} pressure={m.abi_pressure:<3} class={m.pressure_class:<18} {m.name} ({m.path}:{m.line_no})")
    lines.append("")
    lines.append("first leaf-first implementation layers:")
    for i, layer in enumerate(layers["layers_leaf_first"][: min(limit, 10)]):
        preview = ["{" + ", ".join(comp[:3]) + (", ..." if len(comp) > 3 else "") + "}" for comp in layer[:8]]
        lines.append(f"  layer {i}: {len(layer)} SCC(s) " + "; ".join(preview))
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    args = build_arg_parser().parse_args(argv)
    root = Path(args.root).resolve()
    parsed = parse_tree(root, args.paths or None)
    model = build_structure(parsed)
    layer_info = layering_to_dict(model)
    out = root / args.out
    write_json(out / "structure.json", model_to_dict(model))
    write_json(out / "layers.json", layer_info)
    if not args.no_dot:
        write_dot(out / "structure.dot", model)
    report_paths = write_report(out, model) if args.report else {}
    payload = {"structure": model_to_dict(model), "layers": layer_info}
    if args.format == "json":
        print(json.dumps(payload, indent=2, sort_keys=True))
    else:
        print(summarize(model, layer_info, args.limit))
        print(f"\nwrote: {out / 'structure.json'}")
        print(f"wrote: {out / 'layers.json'}")
        if not args.no_dot:
            print(f"wrote: {out / 'structure.dot'}")
        for label, path in report_paths.items():
            print(f"wrote {label}: {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
