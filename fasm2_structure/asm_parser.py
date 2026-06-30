from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
import re
from typing import Iterable

SOURCE_SUFFIXES = {".asm", ".inc", ".ash", ".alm"}

LABEL_RE = re.compile(r"^\s*([A-Za-z_.$?@][\w.$?@]*)\s*:\s*(.*)$")
PROC_RE = re.compile(r"^\s*proc\s+([A-Za-z_.$?@][\w.$?@]*)(?:\s*,\s*(.*))?$", re.I)
ENDP_RE = re.compile(r"^\s*endp\b", re.I)
MACRO_RE = re.compile(r"^\s*(?:macro|calminstruction)\s+([A-Za-z_.$?@][\w.$?@]*)", re.I)
EXTRN_RE = re.compile(r"\bextrn\s+(?:'([^']+)'|\"([^\"]+)\"|([A-Za-z_.$?@][\w.$?@]*))(?:\s+as\s+([A-Za-z_.$?@][\w.$?@]*))?", re.I)
PUBLIC_RE = re.compile(r"\bpublic\s+(.+)$", re.I)
DATA_RE = re.compile(r"^\s*([A-Za-z_.$?@][\w.$?@]*)\s+(db|dw|dd|dq|dt|du|rb|rw|rd|rq|file|struc|struct)\b", re.I)
INCLUDE_RE = re.compile(r"^\s*(?:include|Include)\s+['\"]?([^'\"\s]+)", re.I)
CALL_RE = re.compile(r"^\s*(call|jmp|invoke|stdcall|fastcall|ccall)\b\s*(.*)$", re.I)
IDENT_RE = re.compile(r"\b[A-Za-z_.$?@][\w.$?@]*\b")

ABI_MACROS = {"invoke", "stdcall", "fastcall", "ccall"}
CONTROL_WORDS = {
    "if", "else", "end", "repeat", "while", "break", "iterate", "match", "namespace", "virtual",
    "section", "format", "entry", "public", "extrn", "include", "Include", "local", "locals", "endl",
}
INSTRUCTIONS = {
    "mov", "lea", "add", "sub", "xor", "and", "or", "cmp", "test", "push", "pop", "ret", "retn",
    "enter", "leave", "imul", "mul", "div", "idiv", "shl", "shr", "sar", "sal", "inc", "dec",
    "not", "neg", "xchg", "bt", "bts", "btr", "bsf", "bsr", "setz", "setnz", "cmovz", "cmovnz",
}
DATA_DIRECTIVES = {"db", "dw", "dd", "dq", "dt", "du", "rb", "rw", "rd", "rq", "file"}

@dataclass(frozen=True)
class SourceLine:
    path: str
    line_no: int
    text: str
    code: str

@dataclass
class Symbol:
    name: str
    kind: str
    path: str
    line_no: int
    params: list[str] = field(default_factory=list)
    body: list[SourceLine] = field(default_factory=list)
    public: bool = False
    external: bool = False

@dataclass
class ParseResult:
    root: Path
    lines: list[SourceLine] = field(default_factory=list)
    symbols: dict[str, Symbol] = field(default_factory=dict)
    public_names: set[str] = field(default_factory=set)
    external_names: set[str] = field(default_factory=set)
    includes: dict[str, set[str]] = field(default_factory=dict)


def strip_comment(line: str) -> str:
    in_single = False
    in_double = False
    escaped = False
    for i, ch in enumerate(line):
        if escaped:
            escaped = False
            continue
        if ch == "\\":
            escaped = True
            continue
        if ch == "'" and not in_double:
            in_single = not in_single
        elif ch == '"' and not in_single:
            in_double = not in_double
        elif ch == ";" and not in_single and not in_double:
            return line[:i].rstrip()
    return line.rstrip()


def iter_source_files(root: Path, paths: Iterable[str] | None = None) -> list[Path]:
    if paths:
        out = []
        for p in paths:
            candidate = Path(p)
            if not candidate.is_absolute():
                candidate = root / candidate
            if candidate.is_dir():
                out.extend(x for x in candidate.rglob("*") if x.suffix.lower() in SOURCE_SUFFIXES)
            elif candidate.suffix.lower() in SOURCE_SUFFIXES:
                out.append(candidate)
        return sorted(set(out))
    return sorted(p for p in root.rglob("*") if p.suffix.lower() in SOURCE_SUFFIXES and ".git" not in p.parts)


def split_params(raw: str | None) -> list[str]:
    if not raw:
        return []
    params = []
    for item in raw.split(","):
        name = item.strip().split()[0] if item.strip() else ""
        name = name.rstrip("&=:")
        if name and IDENT_RE.fullmatch(name):
            params.append(name)
    return params


def add_symbol(result: ParseResult, sym: Symbol) -> Symbol:
    # fasm sources can reuse short local labels; keep first canonical definition and qualify duplicates.
    key = sym.name
    if key in result.symbols and result.symbols[key].path != sym.path:
        key = f"{sym.path}:{sym.name}"
        sym.name = key
    result.symbols[key] = sym
    return sym


def parse_tree(root: str | Path, paths: Iterable[str] | None = None) -> ParseResult:
    root = Path(root).resolve()
    result = ParseResult(root=root)
    current: Symbol | None = None
    current_is_proc = False

    for path in iter_source_files(root, paths):
        rel = path.relative_to(root).as_posix()
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for line_no, raw in enumerate(text.splitlines(), 1):
            code = strip_comment(raw)
            src = SourceLine(rel, line_no, raw, code)
            result.lines.append(src)

            inc = INCLUDE_RE.match(code)
            if inc:
                result.includes.setdefault(rel, set()).add(inc.group(1))

            pub = PUBLIC_RE.search(code)
            if pub:
                for name in IDENT_RE.findall(pub.group(1)):
                    if name not in CONTROL_WORDS:
                        result.public_names.add(name)

            for ex in EXTRN_RE.finditer(code):
                ext_name = ex.group(4) or ex.group(1) or ex.group(2) or ex.group(3)
                if ext_name:
                    result.external_names.add(ext_name)
                    result.symbols.setdefault(ext_name, Symbol(ext_name, "external", rel, line_no, external=True))

            proc = PROC_RE.match(code)
            if proc:
                current = add_symbol(result, Symbol(proc.group(1), "function", rel, line_no, split_params(proc.group(2))))
                current_is_proc = True
                continue

            if current is not None and current_is_proc:
                current.body.append(src)
                if ENDP_RE.match(code):
                    current = None
                    current_is_proc = False
                continue

            # A column-0 label starts a new coarse routine. Indented labels inside a routine
            # are treated as local control-flow anchors, not independent functions.
            label = LABEL_RE.match(code)
            label_name = label.group(1) if label else ""
            is_local_label = label_name.startswith(".") or label_name.startswith("@@")
            is_column0_label = bool(label and not is_local_label and raw[:1] not in {" ", "\t"})
            if is_column0_label:
                assert label is not None
                rest_first = (label.group(2).strip().split() or [""])[0].lower()
                kind = "data" if rest_first in DATA_DIRECTIVES else "function"
                current = add_symbol(result, Symbol(label.group(1), kind, rel, line_no)) if kind == "function" else None
                current_is_proc = False
                if kind == "data":
                    add_symbol(result, Symbol(label.group(1), kind, rel, line_no))
                continue

            if current is not None:
                current.body.append(src)
                continue

            data = DATA_RE.match(code)
            if data:
                add_symbol(result, Symbol(data.group(1), "data", rel, line_no))
                continue

            macro = MACRO_RE.match(code)
            if macro:
                add_symbol(result, Symbol(macro.group(1), "macro", rel, line_no))
                continue

    for name in result.public_names:
        if name in result.symbols:
            result.symbols[name].public = True
    for name in result.external_names:
        if name in result.symbols:
            result.symbols[name].external = True
    return result
