#!/usr/bin/env python3
"""
Generate a fasm2 COM interface include from a Windows SDK IDL file.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

import psdk_common as psdk
from idl_forensics import IDLForensicParser


def find_idl(ctx: psdk.PSDKContext, name: str, explicit: str | None) -> Path:
    if explicit:
        path = Path(explicit)
        if not path.exists():
            raise SystemExit(f"IDL file not found: {path}")
        return path

    hits = psdk.find_sdk_sources(
        ctx,
        name,
        exts=(".idl",),
        max_hits=20,
        pattern=rf"\binterface\s+{re.escape(name)}\b",
    )
    for hit in hits:
        parser = IDLForensicParser(verbose=False)
        parser.parse_file(str(hit.path))
        if any(iface.name == name for iface in parser.interfaces):
            return hit.path
    raise SystemExit(f"could not find SDK IDL interface {name}")


def load_interfaces(path: Path) -> dict[str, object]:
    parser = IDLForensicParser(verbose=False)
    parser.parse_file(str(path))
    return {iface.name: iface for iface in parser.interfaces}


def method_groups(name: str, iface_map: dict[str, object]) -> list[tuple[str, list[str]]]:
    groups: list[tuple[str, list[str]]] = []

    def add(interface_name: str) -> None:
        iface = iface_map.get(interface_name)
        if not iface:
            builtin = psdk.BUILTIN_INTERFACE_METHODS.get(interface_name)
            if builtin:
                groups.append((interface_name, builtin))
            elif interface_name:
                groups.append((interface_name, []))
            return
        add(iface.base)
        groups.append((iface.name, [m.name for m in iface.methods]))

    add(name)
    return groups


def interface_chain(name: str, iface_map: dict[str, object]) -> list[object]:
    chain: list[object] = []

    def add(interface_name: str) -> None:
        iface = iface_map.get(interface_name)
        if not iface:
            return
        add(iface.base)
        chain.append(iface)

    add(name)
    return chain


def read_interface_body(path: Path, name: str) -> str:
    text = path.read_text(encoding="utf-8", errors="ignore")
    match = re.search(rf"\binterface\s+{re.escape(name)}\s*:\s*\w+\s*\{{", text)
    if not match:
        return ""
    brace = text.find("{", match.start())
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1:index]
    return ""


def strip_comments(text: str) -> str:
    text = re.sub(r"//.*", "", text)
    return re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)


def remove_idl_attributes(text: str) -> str:
    out: list[str] = []
    depth = 0
    for char in text:
        if char == "[":
            depth += 1
            continue
        if char == "]" and depth:
            depth -= 1
            continue
        if depth == 0:
            out.append(char)
    return "".join(out)


def split_idl_statements(text: str) -> list[str]:
    statements: list[str] = []
    start = 0
    paren_depth = 0
    brace_depth = 0
    bracket_depth = 0
    for index, char in enumerate(text):
        if char == "(":
            paren_depth += 1
        elif char == ")":
            paren_depth = max(0, paren_depth - 1)
        elif char == "{":
            brace_depth += 1
        elif char == "}":
            brace_depth = max(0, brace_depth - 1)
        elif char == "[":
            bracket_depth += 1
        elif char == "]":
            bracket_depth = max(0, bracket_depth - 1)
        elif char == ";" and paren_depth == 0 and brace_depth == 0 and bracket_depth == 0:
            statements.append(text[start:index])
            start = index + 1
    tail = text[start:].strip()
    if tail:
        statements.append(tail)
    return statements


def raw_method_signatures(path: Path, iface_name: str) -> dict[str, str]:
    body = strip_comments(read_interface_body(path, iface_name))
    body = re.sub(r"typedef\s+(?:\[[^\]]+\]\s*)?enum\s+(?:\w+\s*)?\{.*?\}\s*\w+\s*;", "", body, flags=re.DOTALL)
    result: dict[str, str] = {}
    for statement in split_idl_statements(body):
        clean = remove_idl_attributes(statement)
        clean = re.sub(r"\s+", " ", clean).strip()
        if clean.startswith("typedef "):
            continue
        match = re.match(r"(HRESULT|void|SCODE|DWORD|ULONG|BOOL)\s+(\w+)\s*\((.*)\)\s*$", clean, re.DOTALL)
        if not match:
            continue
        ret_type, method, params = match.groups()
        params = params.strip() or "void"
        result[method] = f"{ret_type} {method}({params})"
    return result


def parse_enum_value(expr: str, known: dict[str, int]) -> int | str:
    cleaned = strip_comments(expr).strip()
    cleaned = re.sub(r"\b(UL|ULL|U|L|i64|ui64)\b", "", cleaned)
    cleaned = re.sub(r"([0-9A-Fa-f])(?:UL|ULL|U|L|i64|ui64)\b", r"\1", cleaned)
    if not cleaned:
        return 0
    try:
        return int(cleaned, 0)
    except ValueError:
        pass

    eval_text = cleaned
    for name, value in known.items():
        eval_text = re.sub(rf"\b{re.escape(name)}\b", str(value), eval_text)
    if re.fullmatch(r"[0-9xXa-fA-F\s\|\&\~\+\-\<\>\(\)]+", eval_text):
        try:
            return int(eval(eval_text, {"__builtins__": {}}, {}))
        except Exception:
            pass
    return cleaned


def nested_enums(path: Path, chain: list[object]) -> list[dict]:
    enums: list[dict] = []
    for iface in chain:
        body = read_interface_body(path, iface.name)
        if not body:
            continue
        body = strip_comments(body)
        pattern = re.compile(
            r"typedef\s+(?:\[[^\]]+\]\s*)?enum\s+(?:\w+\s*)?"
            r"\{(.*?)\}\s*(\w+)\s*;",
            re.DOTALL,
        )
        for match in pattern.finditer(body):
            enum_name = match.group(2)
            values: dict[str, int | str] = {}
            current = 0
            numeric_values: dict[str, int] = {}
            for raw_item in match.group(1).split(","):
                item = raw_item.strip()
                if not item:
                    continue
                if "=" in item:
                    const_name, expr = item.split("=", 1)
                    const_name = const_name.strip()
                    value = parse_enum_value(expr, numeric_values)
                    if isinstance(value, int):
                        current = value
                        numeric_values[const_name] = value
                else:
                    const_name = item.strip()
                    value = current
                    numeric_values[const_name] = value
                values[const_name] = value
                if isinstance(value, int):
                    current = value + 1
            enums.append({
                "name": enum_name,
                "constants": [{"name": k, "value": v} for k, v in values.items()],
            })
    return enums


def signature_comment(method) -> str:
    params = []
    for param in method.params:
        name = param.name or "param"
        params.append(f"{param.type} {name}".strip())
    params_text = ", ".join(params) if params else "void"
    return f";   {method.ret_type} {method.name}({params_text})"


def render_interface_macro(iface, iface_map: dict[str, object]) -> str:
    groups = method_groups(iface.name, iface_map)
    methods = [method for _base, group in groups for method in group]
    lines = [f"; interface {iface.name} : {iface.base}"]
    for base, group in groups:
        if group:
            lines.append(f";   {base}: {', '.join(group)}")
        else:
            lines.append(f";   {base}: methods not available in this IDL")

    if iface.iid and iface.iid != "00000000-0000-0000-0000-000000000000":
        lines.append(f"IID_{iface.name} db {psdk.guid_bytes(iface.iid)}")

    lines.append(f"interface {iface.name},\\")
    for index, method in enumerate(methods):
        suffix = ",\\" if index + 1 < len(methods) else ""
        lines.append(f"\t{method}{suffix}")
    return "\n".join(lines)


def render_include(ctx: psdk.PSDKContext, name: str, path: Path, iface_map: dict[str, object]) -> str:
    if name not in iface_map:
        raise SystemExit(f"{name} was not parsed from {path}")
    chain = interface_chain(name, iface_map)

    lines: list[str] = [
        f"; PSDK COM interface inquiry: {name}",
        f"; {psdk.sdk_summary(ctx)}",
        f"; source: {path}",
        "",
    ]

    enums = nested_enums(path, chain)
    for enum in enums:
        lines.append(psdk.render_enum(enum))
        lines.append("")

    lines.append("; Method signatures from IDL")
    for iface in chain:
        sigs = raw_method_signatures(path, iface.name)
        lines.append(f"; {iface.name} : {iface.base}")
        for method in iface.methods:
            lines.append(f";   {sigs.get(method.name, signature_comment(method).lstrip('; '))}")
    lines.append("")

    for iface in chain:
        lines.append(render_interface_macro(iface, iface_map))
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Generate include/com/<interface>.inc from a Windows SDK IDL interface")
    ap.add_argument("name", help="COM interface name, for example IAutoComplete2")
    ap.add_argument("--idl", help="Specific IDL file to parse")
    ap.add_argument("--out-dir", type=Path,
                    help="Directory for --write (default: include/com)")
    ap.add_argument("--write", action="store_true",
                    help="Write include/com/<interface>.inc instead of printing")
    ap.add_argument("--force", action="store_true",
                    help="Allow --write to overwrite an existing include")
    ap.add_argument("--fasm2-root", default=None)
    ap.add_argument("--llvm", default=None)
    ap.add_argument("--sdk", default=None)
    ap.add_argument("--sdk-ver", default=None)
    args = ap.parse_args(argv)

    ctx = psdk.make_context(args)
    idl_path = find_idl(ctx, args.name, args.idl)
    iface_map = load_interfaces(idl_path)
    text = render_include(ctx, args.name, idl_path, iface_map)

    if not args.write:
        print(text, end="")
        return 0

    out_dir = args.out_dir or (ctx.fasm2_root / "include" / "com")
    out_path = out_dir / f"{args.name}.inc"
    if out_path.exists() and not args.force:
        raise SystemExit(f"{out_path} already exists; use --force to overwrite")
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path.write_text(text, encoding="utf-8", newline="\n")
    print(f"wrote {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
