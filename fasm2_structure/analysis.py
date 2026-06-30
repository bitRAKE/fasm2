from __future__ import annotations

from dataclasses import dataclass, field
import re
from typing import Iterable

from .asm_parser import ABI_MACROS, CALL_RE, IDENT_RE, INSTRUCTIONS, ParseResult, SourceLine, Symbol

REGISTER_PARAMS_X64 = {"rcx", "rdx", "r8", "r9", "ecx", "edx", "r8d", "r9d", "cl", "dl", "r8b", "r9b"}
REGISTER_PARAMS_X86 = {"ecx", "edx", "cx", "dx", "cl", "dl"}
STACK_PARAM_RE = re.compile(r"\[(?:e|r)?bp\s*\+\s*(?:[1-9][0-9]*|[A-Za-z_.$?@][\w.$?@]*)", re.I)
MEMREF_RE = re.compile(r"\[([^\]]+)\]")

@dataclass(frozen=True)
class Edge:
    source: str
    target: str
    kind: str
    path: str
    line_no: int
    text: str

@dataclass
class FunctionMetrics:
    name: str
    path: str
    line_no: int
    total_calls: int = 0
    internal_calls: int = 0
    abi_calls: int = 0
    tail_abi_calls: int = 0
    parameter_uses_after_abi_call: int = 0
    abi_pressure: int = 0
    pressure_class: str = "pure_leaf"
    notes: list[str] = field(default_factory=list)

@dataclass
class StructureModel:
    symbols: dict[str, Symbol]
    edges: list[Edge]
    metrics: dict[str, FunctionMetrics]


def normalize_target(raw: str) -> str:
    raw = raw.strip()
    if not raw:
        return ""
    raw = raw.split(",", 1)[0].strip()
    raw = raw.removeprefix("[").removesuffix("]")
    raw = raw.strip("'\"")
    return raw


def instruction_of(line: SourceLine) -> tuple[str, str] | None:
    match = CALL_RE.match(line.code)
    if not match:
        return None
    return match.group(1).lower(), match.group(2).strip()


def is_returnish(line: SourceLine) -> bool:
    code = line.code.strip().lower()
    return code.startswith(("ret", "retn", "iret")) or code in {"endp"}


def later_executable_lines(lines: list[SourceLine], idx: int) -> list[SourceLine]:
    return [ln for ln in lines[idx + 1:] if ln.code.strip()]


def is_tail_call(lines: list[SourceLine], idx: int, op: str) -> bool:
    later = later_executable_lines(lines, idx)
    if op == "jmp":
        return True
    if not later:
        return True
    # Allow only returns/end markers after a tail call heuristic.
    return all(is_returnish(ln) for ln in later[:2])


def infer_param_tokens(sym: Symbol) -> set[str]:
    names = {p.lower() for p in sym.params}
    names |= REGISTER_PARAMS_X64 | REGISTER_PARAMS_X86
    return names


def count_parameter_uses(line: SourceLine, param_tokens: set[str]) -> int:
    code = line.code
    if not code.strip():
        return 0
    count = 0
    for ident in IDENT_RE.findall(code):
        if ident.lower() in param_tokens:
            count += 1
    if STACK_PARAM_RE.search(code):
        count += 1
    return count


def likely_data_reference(line: SourceLine, known_data: set[str]) -> Iterable[str]:
    op = (line.code.strip().split(None, 1) or [""])[0].lower()
    if op in {"call", "jmp"} | ABI_MACROS | INSTRUCTIONS:
        for ident in IDENT_RE.findall(line.code):
            if ident in known_data:
                yield ident
    else:
        return


def build_structure(parsed: ParseResult) -> StructureModel:
    known_functions = {name for name, sym in parsed.symbols.items() if sym.kind == "function"}
    known_data = {name for name, sym in parsed.symbols.items() if sym.kind == "data"}
    external_names = set(parsed.external_names) | {name for name, sym in parsed.symbols.items() if sym.external}
    edges: list[Edge] = []
    metrics: dict[str, FunctionMetrics] = {}

    for name, sym in parsed.symbols.items():
        if sym.kind != "function":
            continue
        metric = FunctionMetrics(name=name, path=sym.path, line_no=sym.line_no)
        param_tokens = infer_param_tokens(sym)
        seen_abi = False
        for idx, line in enumerate(sym.body):
            ins = instruction_of(line)
            if ins:
                op, raw_target = ins
                target = normalize_target(raw_target)
                if not target:
                    continue
                kind = "abi-call" if op in ABI_MACROS and target in known_functions else "abi" if op in ABI_MACROS or target in external_names or target.startswith("[") else "call"
                if op == "jmp" and target in known_functions:
                    kind = "tail-call" if is_tail_call(sym.body, idx, op) else "jump"
                elif op == "call" and target not in known_functions and target not in external_names:
                    kind = "indirect-call" if any(ch in raw_target for ch in "[]") else "unresolved-call"
                metric.total_calls += 1
                if kind in {"call", "tail-call", "jump", "abi-call"} and target in known_functions:
                    metric.internal_calls += 1
                if kind in {"abi", "abi-call"}:
                    metric.abi_calls += 1
                    seen_abi = True
                    if is_tail_call(sym.body, idx, op):
                        metric.tail_abi_calls += 1
                edges.append(Edge(name, target, kind, line.path, line.line_no, line.text.strip()))
                continue

            if seen_abi:
                metric.parameter_uses_after_abi_call += count_parameter_uses(line, param_tokens)

            for data_name in likely_data_reference(line, known_data):
                edges.append(Edge(name, data_name, "data", line.path, line.line_no, line.text.strip()))

        metric.abi_pressure = metric.abi_calls + metric.parameter_uses_after_abi_call
        if metric.abi_pressure == 0:
            metric.pressure_class = "pure_leaf"
            metric.notes.append("pure leaf: no ABI frame needed by heuristic")
        elif metric.abi_calls == metric.tail_abi_calls and metric.parameter_uses_after_abi_call == 0:
            metric.pressure_class = "tail_abi"
            metric.notes.append("ABI interaction is tail-position by heuristic")
        elif metric.parameter_uses_after_abi_call == 0:
            metric.pressure_class = "abi_boundary"
        else:
            metric.pressure_class = "abi_state_pressure"
            metric.notes.append("parameter evidence survives across ABI calls; implementation may need frame/register choreography")
        metrics[name] = metric

    return StructureModel(parsed.symbols, edges, metrics)


def graph_adjacency(model: StructureModel, include_external: bool = False) -> dict[str, set[str]]:
    funcs = {n for n, s in model.symbols.items() if s.kind == "function"}
    adj = {n: set() for n in funcs}
    for edge in model.edges:
        if edge.kind in {"call", "tail-call", "jump", "abi-call"} and edge.target in funcs:
            adj.setdefault(edge.source, set()).add(edge.target)
        elif include_external and edge.kind in {"abi", "unresolved-call", "indirect-call"}:
            adj.setdefault(edge.source, set()).add(edge.target)
    return adj


def tarjan_scc(adj: dict[str, set[str]]) -> list[list[str]]:
    index = 0
    stack: list[str] = []
    on_stack: set[str] = set()
    indices: dict[str, int] = {}
    low: dict[str, int] = {}
    comps: list[list[str]] = []

    def strongconnect(v: str) -> None:
        nonlocal index
        indices[v] = index
        low[v] = index
        index += 1
        stack.append(v)
        on_stack.add(v)
        for w in adj.get(v, ()):  # nodes outside adj are external and ignored
            if w not in adj:
                continue
            if w not in indices:
                strongconnect(w)
                low[v] = min(low[v], low[w])
            elif w in on_stack:
                low[v] = min(low[v], indices[w])
        if low[v] == indices[v]:
            comp = []
            while True:
                w = stack.pop()
                on_stack.remove(w)
                comp.append(w)
                if w == v:
                    break
            comps.append(sorted(comp))

    for v in sorted(adj):
        if v not in indices:
            strongconnect(v)
    return comps


def condensation_layers(adj: dict[str, set[str]], comps: list[list[str]]) -> list[list[list[str]]]:
    comp_id = {node: i for i, comp in enumerate(comps) for node in comp}
    out: dict[int, set[int]] = {i: set() for i in range(len(comps))}
    indeg: dict[int, int] = {i: 0 for i in range(len(comps))}
    for src, targets in adj.items():
        a = comp_id[src]
        for dst in targets:
            if dst not in comp_id:
                continue
            b = comp_id[dst]
            if a != b and b not in out[a]:
                out[a].add(b)
                indeg[b] += 1
    # Layer 0 = leaves/no outgoing deps, useful for implementation ordering.
    remaining = set(range(len(comps)))
    layers: list[list[list[str]]] = []
    reverse_out: dict[int, set[int]] = {i: set() for i in range(len(comps))}
    for a, bs in out.items():
        for b in bs:
            reverse_out[b].add(a)
    no_deps = {i for i in remaining if not out[i]}
    while remaining:
        if not no_deps:
            no_deps = set(remaining)
        layer_ids = sorted(no_deps)
        layers.append([comps[i] for i in layer_ids])
        remaining -= set(layer_ids)
        next_no_deps: set[int] = set()
        for i in layer_ids:
            for parent in reverse_out[i]:
                if parent in remaining:
                    out[parent].discard(i)
                    if not out[parent]:
                        next_no_deps.add(parent)
        no_deps = next_no_deps
    return layers
