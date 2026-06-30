from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .analysis import StructureModel, condensation_layers, graph_adjacency, tarjan_scc


def model_to_dict(model: StructureModel) -> dict[str, Any]:
    return {
        "symbols": {
            name: {
                "kind": sym.kind,
                "path": sym.path,
                "line": sym.line_no,
                "params": sym.params,
                "public": sym.public,
                "external": sym.external,
            }
            for name, sym in sorted(model.symbols.items())
        },
        "edges": [edge.__dict__ for edge in model.edges],
        "abi_pressure": {name: metric.__dict__ for name, metric in sorted(model.metrics.items())},
    }


def layering_to_dict(model: StructureModel) -> dict[str, Any]:
    adj = graph_adjacency(model)
    sccs = tarjan_scc(adj)
    layers = condensation_layers(adj, sccs)
    recursive = [comp for comp in sccs if len(comp) > 1 or any(n in adj.get(n, set()) for n in comp)]
    return {
        "scc_count": len(sccs),
        "recursive_sccs": recursive,
        "layers_leaf_first": layers,
    }


def write_json(path: str | Path, obj: Any) -> None:
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_text(json.dumps(obj, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def write_dot(path: str | Path, model: StructureModel, include_data: bool = True) -> None:
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    lines = ["digraph fasm2_structure {", "  rankdir=LR;"]
    for name, sym in sorted(model.symbols.items()):
        if sym.kind == "external":
            shape = "box"
            style = "dashed"
        elif sym.kind == "data":
            if not include_data:
                continue
            shape = "cylinder"
            style = "solid"
        elif sym.kind == "macro":
            shape = "component"
            style = "solid"
        else:
            shape = "ellipse"
            style = "solid"
        safe = name.replace('"', '\\"')
        lines.append(f'  "{safe}" [shape={shape}, style={style}];')
    colors = {
        "call": "black",
        "tail-call": "darkgreen",
        "jump": "gray30",
        "abi-call": "darkblue",
        "abi": "blue",
        "data": "purple",
        "unresolved-call": "orange",
        "indirect-call": "red",
    }
    for edge in model.edges:
        if edge.kind == "data" and not include_data:
            continue
        src = edge.source.replace('"', '\\"')
        dst = edge.target.replace('"', '\\"')
        color = colors.get(edge.kind, "black")
        lines.append(f'  "{src}" -> "{dst}" [label="{edge.kind}", color={color}];')
    lines.append("}")
    Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")
