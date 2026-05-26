#!/usr/bin/env python3
"""
Shared helpers for small PSDK inquiry tools.

The helpers intentionally sit beside win32_scraper.py and reuse its SDK/LLVM
discovery, clang AST queries, and basic C-to-fasm2 type mapping.  The separate
front-end tools can then stay focused on one question each.
"""

from __future__ import annotations

import os
import re
import sys
import uuid
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

import win32_scraper as win32  # noqa: E402


SDK_HEADER_DIRS = ("um", "shared", "ucrt")
SDK_SEARCH_DIRS = ("um", "shared")
BUILTIN_INTERFACE_METHODS = {
    "IUnknown": ["QueryInterface", "AddRef", "Release"],
    "IDispatch": [
        "QueryInterface", "AddRef", "Release",
        "GetTypeInfoCount", "GetTypeInfo", "GetIDsOfNames", "Invoke",
    ],
}

PRIMITIVE_TYPES = {
    "void", "VOID", "char", "wchar_t", "short", "int", "long", "float",
    "double", "signed", "unsigned", "__int8", "__int16", "__int32",
    "__int64", "HRESULT", "BOOL", "BOOLEAN", "BYTE", "WORD", "DWORD",
    "QWORD", "ULONG", "LONG", "UINT", "INT", "USHORT", "SHORT",
    "UCHAR", "CHAR", "WCHAR", "FLOAT", "HANDLE", "PVOID", "LPVOID",
    "LPCVOID", "LPSTR", "LPCSTR", "LPWSTR", "LPCWSTR", "BSTR",
    "REFIID", "REFGUID", "IID", "GUID", "CLSID",
}


@dataclass(frozen=True)
class PSDKContext:
    fasm2_root: Path
    llvm_bin: Path
    sdk_root: Path
    sdk_ver: str

    @property
    def include_root(self) -> Path:
        return self.sdk_root / "Include" / self.sdk_ver


@dataclass(frozen=True)
class SourceHit:
    path: Path
    line: int
    text: str


@dataclass
class FunctionInquiry:
    requested_name: str
    name: str
    info: dict
    headers: list[str]
    dll_name: str | None = None
    exported: bool | None = None


@dataclass
class DependencySet:
    structs: dict[str, dict]
    enums: dict[str, dict]
    unknown: set[str]


def make_context(args) -> PSDKContext:
    fasm2_root = Path(getattr(args, "fasm2_root", "") or SCRIPT_DIR.parent).resolve()
    llvm_bin = Path(args.llvm).resolve() if getattr(args, "llvm", None) else win32.find_llvm_bin()
    if getattr(args, "sdk", None):
        sdk_root = Path(args.sdk).resolve()
        sdk_ver = getattr(args, "sdk_ver", None)
        if not sdk_ver:
            versions = sorted(
                d.name for d in (sdk_root / "Include").iterdir()
                if d.is_dir() and d.name.startswith("10.")
            )
            if not versions:
                raise RuntimeError(f"no Windows SDK versions under {sdk_root}")
            sdk_ver = versions[-1]
    else:
        sdk_root, sdk_ver = win32.find_sdk()
        if getattr(args, "sdk_ver", None):
            sdk_ver = args.sdk_ver
    return PSDKContext(fasm2_root, llvm_bin, sdk_root, sdk_ver)


def ensure_windows_header(headers: Sequence[str]) -> list[str]:
    seen = set()
    result: list[str] = []
    for h in ("windows.h", *headers):
        key = h.lower()
        if key not in seen:
            seen.add(key)
            result.append(h)
    return result


def sdk_file_to_include(ctx: PSDKContext, path: Path) -> str:
    path = path.resolve()
    for sub in SDK_HEADER_DIRS:
        root = (ctx.include_root / sub).resolve()
        try:
            return path.relative_to(root).as_posix()
        except ValueError:
            pass
    return path.name


def include_to_sdk_file(ctx: PSDKContext, include_name: str) -> Path | None:
    p = Path(include_name)
    if p.is_absolute() and p.exists():
        return p
    normalized = include_name.replace("/", "\\")
    for sub in SDK_HEADER_DIRS:
        direct = ctx.include_root / sub / normalized
        if direct.exists():
            return direct
    lower = normalized.lower()
    for sub in SDK_HEADER_DIRS:
        root = ctx.include_root / sub
        if not root.exists():
            continue
        for candidate in root.rglob(Path(normalized).name):
            try:
                inc = candidate.relative_to(root).as_posix().lower()
            except ValueError:
                inc = candidate.name.lower()
            if candidate.name.lower() == Path(lower).name or inc == lower.replace("\\", "/"):
                return candidate
    return None


def resolve_headers(ctx: PSDKContext, headers: Sequence[str]) -> list[str]:
    resolved: list[str] = []
    for header in headers:
        p = include_to_sdk_file(ctx, header)
        resolved.append(sdk_file_to_include(ctx, p) if p else header)
    return ensure_windows_header(resolved)


def all_sdk_sources(
    ctx: PSDKContext,
    exts: Sequence[str] = (".h", ".idl"),
    subdirs: Sequence[str] = SDK_SEARCH_DIRS,
) -> Iterable[Path]:
    for sub in subdirs:
        root = ctx.include_root / sub
        if not root.exists():
            continue
        for ext in exts:
            yield from root.rglob(f"*{ext}")


def find_sdk_sources(
    ctx: PSDKContext,
    symbol: str,
    exts: Sequence[str] = (".h", ".idl"),
    subdirs: Sequence[str] = SDK_SEARCH_DIRS,
    max_hits: int = 20,
    pattern: str | None = None,
) -> list[SourceHit]:
    regex = re.compile(pattern or rf"\b{re.escape(symbol)}\b")
    hits: list[SourceHit] = []
    for path in all_sdk_sources(ctx, exts=exts, subdirs=subdirs):
        try:
            with path.open("r", encoding="utf-8", errors="ignore") as f:
                for line_no, line in enumerate(f, 1):
                    if regex.search(line):
                        hits.append(SourceHit(path, line_no, line.strip()))
                        break
        except OSError:
            continue
        if len(hits) >= max_hits:
            break
    return hits


def query_ast(ctx: PSDKContext, name: str, headers: Sequence[str], arch: int = 64) -> dict | None:
    return win32.query_ast_filter(
        name,
        resolve_headers(ctx, headers),
        ctx.llvm_bin,
        ctx.sdk_root,
        ctx.sdk_ver,
        arch=arch,
    )


def _read_headers(ctx: PSDKContext, headers: Sequence[str]) -> str:
    chunks: list[str] = []
    for header in resolve_headers(ctx, headers):
        if header.lower() == "windows.h":
            continue
        path = include_to_sdk_file(ctx, header)
        if path:
            chunks.append(path.read_text(encoding="utf-8", errors="ignore"))
    return "\n".join(chunks)


def typedef_tag_for(ctx: PSDKContext, public_name: str, headers: Sequence[str], kind: str) -> str | None:
    text = _read_headers(ctx, headers)
    if not text:
        return None
    pat = re.compile(rf"typedef\s+{kind}\s+([A-Za-z_]\w*)\s*\{{", re.DOTALL)
    for match in pat.finditer(text):
        brace = text.find("{", match.start())
        close = matching_brace(text, brace)
        if close < 0:
            continue
        semicolon = text.find(";", close)
        if semicolon < 0:
            continue
        declarators = text[close + 1:semicolon]
        if re.search(rf"\b{re.escape(public_name)}\b", declarators):
            return match.group(1)
    return None


def matching_brace(text: str, open_index: int) -> int:
    if open_index < 0 or open_index >= len(text) or text[open_index] != "{":
        return -1
    depth = 0
    for index in range(open_index, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return index
    return -1


def record_query_names(ctx: PSDKContext, public_name: str, headers: Sequence[str]) -> list[str]:
    candidates = [public_name]
    tag = typedef_tag_for(ctx, public_name, headers, "struct")
    if tag:
        candidates.insert(0, tag)
    candidates.extend([f"_{public_name}", f"tag{public_name}"])
    return unique(candidates)


def enum_query_names(ctx: PSDKContext, public_name: str, headers: Sequence[str]) -> list[str]:
    candidates = [public_name]
    tag = typedef_tag_for(ctx, public_name, headers, "enum")
    if tag:
        candidates.insert(0, tag)
    candidates.extend([f"_{public_name}", f"_tag{public_name}", f"tag{public_name}"])
    return unique(candidates)


def resolve_record(ctx: PSDKContext, public_name: str, headers: Sequence[str], arch: int = 64) -> dict | None:
    for query_name in record_query_names(ctx, public_name, headers):
        ast = query_ast(ctx, query_name, headers, arch=arch)
        if not ast:
            continue
        info = win32.extract_record_info(ast, query_name)
        if info:
            info = dict(info)
            info["tag_name"] = query_name
            info["name"] = public_name
            return info
    return None


def resolve_enum(ctx: PSDKContext, public_name: str, headers: Sequence[str], arch: int = 64) -> dict | None:
    for query_name in enum_query_names(ctx, public_name, headers):
        ast = query_ast(ctx, query_name, headers, arch=arch)
        if not ast:
            continue
        info = win32.extract_enum_info(ast, query_name)
        if info:
            info = dict(info)
            info["tag_name"] = query_name
            info["name"] = public_name
            return info
    return None


def query_function(ctx: PSDKContext, name: str, headers: Sequence[str], arch: int = 64) -> dict | None:
    ast = query_ast(ctx, name, headers, arch=arch)
    return win32.extract_function_info(ast, name) if ast else None


def discover_function(
    ctx: PSDKContext,
    name: str,
    dll_name: str | None = None,
    headers: Sequence[str] | None = None,
    arch: int = 64,
) -> FunctionInquiry | None:
    names = [name]
    if not name.endswith(("A", "W")):
        names.extend([f"{name}W", f"{name}A"])

    if headers:
        resolved = resolve_headers(ctx, headers)
        for candidate in names:
            info = query_function(ctx, candidate, resolved, arch=arch)
            if info:
                return FunctionInquiry(name, candidate, info, resolved, dll_name)
        return None

    dll_candidates: list[tuple[str, list[str]]] = []
    if dll_name:
        dll_candidates.append((dll_name, win32.DLL_HEADERS.get(dll_name, [])))
    else:
        dll_candidates.extend((dll, hdrs) for dll, hdrs in win32.DLL_HEADERS.items())

    for dll, hdrs in dll_candidates:
        if not hdrs:
            continue
        resolved = resolve_headers(ctx, hdrs)
        for candidate in names:
            info = query_function(ctx, candidate, resolved, arch=arch)
            if info:
                exported = None
                try:
                    exported = candidate in win32.get_exports(dll, ctx.sdk_root, ctx.sdk_ver, ctx.llvm_bin)
                except Exception:
                    pass
                return FunctionInquiry(name, candidate, info, resolved, dll, exported)

    # Last chance: search headers by text and query the few likely source files.
    for hit in find_sdk_sources(ctx, name, exts=(".h",), max_hits=12):
        header = sdk_file_to_include(ctx, hit.path)
        for candidate in names:
            info = query_function(ctx, candidate, [header], arch=arch)
            if info:
                return FunctionInquiry(name, candidate, info, resolve_headers(ctx, [header]), None)
    return None


def clean_type_name(ctype: str) -> str:
    t = ctype.strip()
    t = re.sub(r"\b(const|volatile|struct|union|enum|__unaligned|__ptr64|__ptr32)\b", " ", t)
    t = re.sub(r"\b(__RPC__\w+|_In_|_Out_|_Inout_|_COM_Outptr_)\b", " ", t)
    t = re.sub(r"\[[^\]]+\]", " ", t)
    t = t.replace("*", " ")
    t = re.sub(r"\s+", " ", t).strip()
    if not t or "(" in t or ")" in t:
        return ""
    return t.split()[-1]


def type_array_count(ctype: str) -> tuple[str, int]:
    count = 1
    for value in re.findall(r"\[(\d+)\]", ctype):
        count *= int(value)
    base = re.sub(r"\[[^\]]+\]", "", ctype).strip()
    return base, count


def is_builtin_type(name: str) -> bool:
    if not name:
        return True
    return (
        name in PRIMITIVE_TYPES
        or name in win32.HANDLE_TYPES
        or name in win32.PTR_SIZED_TYPES
        or name in win32.QWORD_TYPES
        or name in win32.DWORD_TYPES
        or name in win32.WORD_TYPES
        or name in win32.BYTE_TYPES
        or name.startswith("LP")
        or name.startswith("PC")
        or is_pointer_alias(name)
    )


def dependency_type_names(types: Iterable[str]) -> list[str]:
    names = []
    for ctype in types:
        base = clean_type_name(ctype)
        if base and base[0].isupper() and not is_builtin_type(base):
            names.append(base)
    return unique(names)


def collect_dependencies(
    ctx: PSDKContext,
    root_types: Iterable[str],
    headers: Sequence[str],
    arch: int = 64,
    max_types: int = 40,
) -> DependencySet:
    structs: dict[str, dict] = {}
    enums: dict[str, dict] = {}
    unknown: set[str] = set()
    queue = dependency_type_names(root_types)
    visited: set[str] = set()

    while queue and len(visited) < max_types:
        name = queue.pop(0)
        if name in visited or is_builtin_type(name):
            continue
        visited.add(name)

        record = resolve_record(ctx, name, headers, arch=arch)
        if record:
            structs[name] = record
            queue.extend(dependency_type_names(f["type"] for f in record.get("fields", [])))
            continue

        enum = resolve_enum(ctx, name, headers, arch=arch)
        if enum:
            enums[name] = enum
            continue

        unknown.add(name)

    return DependencySet(structs, enums, unknown)


def collect_function_dependencies(ctx: PSDKContext, inquiry: FunctionInquiry, arch: int = 64) -> DependencySet:
    types = [inquiry.info.get("return_type", "")]
    types.extend(p.get("type", "") for p in inquiry.info.get("params", []))
    return collect_dependencies(ctx, types, inquiry.headers, arch=arch)


def storage_for_field(ctype: str, arch: int, known_types: dict[str, str]) -> tuple[str, int]:
    base_with_array, count = type_array_count(ctype)
    base_name = clean_type_name(base_with_array)
    if is_pointer_alias(base_name) and known_types.get(base_name) not in ("struct", "union", "enum"):
        directive = "dq" if arch == 64 else "dd"
        return directive_with_count(directive, count), (8 if arch == 64 else 4) * count

    directive, elem_size = win32.c_type_to_fasm2(base_with_array, arch=arch)
    if directive != "??":
        return directive_with_count(directive, count), elem_size * count

    if known_types.get(base_name) == "enum":
        return directive_with_count("dd", count), 4 * count
    if known_types.get(base_name) in ("struct", "union"):
        suffix = f" {count} dup (?)" if count > 1 else ""
        return f"{base_name}{suffix}", 0
    return directive_with_count("dd", count), 4 * count


def is_pointer_alias(name: str) -> bool:
    if not name:
        return False
    if name in {"POINT", "POINTS", "PAINTSTRUCT", "POLYTEXTA", "POLYTEXTW"}:
        return False
    return (
        name.startswith("LP")
        or name.startswith("LPC")
        or name.startswith("PC")
        or (name.startswith("P") and len(name) > 1 and name[1].isupper())
    )


def directive_with_count(directive: str, count: int) -> str:
    return f"{directive} {count} dup (?)" if count > 1 else f"{directive} ?"


def directive_for_size(size: int) -> str:
    if size == 8:
        return "dq ?"
    if size == 4:
        return "dd ?"
    if size == 2:
        return "dw ?"
    if size == 1:
        return "db ?"
    return f"db {size} dup (?)"


def padding_line(size: int, offset: int) -> str:
    if size == 1:
        return f"  {'':<22} db ?          ; padding to offset {offset}"
    return f"  {'':<22} db {size} dup (?) ; padding to offset {offset}"


def known_type_map(deps: DependencySet) -> dict[str, str]:
    result = {name: info.get("kind", "struct") for name, info in deps.structs.items()}
    result.update({name: "enum" for name in deps.enums})
    return result


def struct_layout(ctx: PSDKContext, name: str, headers: Sequence[str], arch: int) -> dict | None:
    try:
        return win32.get_sdk_struct_layout(
            name,
            resolve_headers(ctx, headers),
            ctx.llvm_bin,
            ctx.sdk_root,
            ctx.sdk_ver,
            arch=arch,
        )
    except Exception:
        return None


def render_struct(info: dict, known_types: dict[str, str], layout: dict | None, arch: int) -> str:
    name = info["name"]
    packed = bool(layout and layout.get("align") == 1)
    header = f"struct {name}, packed" if packed else f"struct {name}"
    lines = [header]

    layout_offsets = {}
    layout_entries = []
    if layout:
        for field_name, offset, _size in layout.get("fields", []):
            if field_name in {info.get("name"), info.get("tag_name")} and _size == 0:
                continue
            layout_entries.append((field_name, offset, _size))
            layout_offsets.setdefault(field_name, []).append(offset)

    current = 0
    for index, field in enumerate(info.get("fields", []), 1):
        raw_name = field.get("name") or f"_anonymous{index}"
        ctype = field.get("type", "")
        storage, size = storage_for_field(ctype, arch, known_types)

        union_names: list[str] = []
        if raw_name.startswith("_anonymous") and layout_entries:
            upcoming = [entry for entry in layout_entries if entry[1] >= current]
            if upcoming:
                target_offset = min(entry[1] for entry in upcoming)
                same_offset = [entry for entry in layout_entries if entry[1] == target_offset]
                union_names = unique(entry[0] for entry in same_offset)
                union_size = max((entry[2] for entry in same_offset), default=0)
                if union_size:
                    raw_name = union_names[0]
                    storage = directive_for_size(union_size)
                    size = union_size
            else:
                target_offset = current
        else:
            offsets = layout_offsets.get(raw_name)
            target_offset = offsets.pop(0) if offsets else current

        if target_offset > current:
            lines.append(padding_line(target_offset - current, target_offset))
            current = target_offset

        if union_names:
            comment = f"; union: {' / '.join(union_names)}"
        else:
            comment = f"; {ctype}"
        if layout:
            comment += f" offset {target_offset}"
        lines.append(f"  {raw_name:<22} {storage:<12} {comment}")
        current += size

    if layout and layout.get("size", 0) > current:
        lines.append(padding_line(layout["size"] - current, layout["size"]))

    lines.append("ends")
    if layout:
        lines.insert(0, f"; sizeof={layout.get('size')}, align={layout.get('align')} ({arch}-bit)")
    return "\n".join(lines)


def render_enum(info: dict) -> str:
    lines = [f"; enum {info['name']}"]
    for const in info.get("constants", []):
        value = const.get("value")
        if value is None:
            continue
        lines.append(f"{const['name']:<40} := {format_int(value)}")
    return "\n".join(lines)


def format_int(value) -> str:
    if isinstance(value, int):
        ivalue = value
    else:
        raw = str(value).strip()
        raw = re.sub(r"([0-9A-Fa-f])(?:U|L|UL|ULL|i64|ui64)$", r"\1", raw)
        try:
            ivalue = int(raw, 0)
        except ValueError:
            return raw
    if ivalue < 0:
        return str(ivalue)
    if ivalue < 10:
        return str(ivalue)
    digits = f"{ivalue:X}"
    if digits[0].isalpha():
        digits = "0" + digits
    return f"{digits}h"


def unique(values: Iterable[str]) -> list[str]:
    seen = set()
    result = []
    for value in values:
        if value and value not in seen:
            seen.add(value)
            result.append(value)
    return result


def aw_stem(name: str) -> str | None:
    return win32._aw_stem(name)


def append_or_preview(path: Path, text: str, write: bool) -> str:
    path = path.resolve()
    if not write:
        return f"; would append to {path}\n{text}"
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8", newline="\n") as f:
        f.write("\n")
        f.write(text.rstrip())
        f.write("\n")
    return f"appended to {path}"


def guid_bytes(guid_text: str) -> str:
    data = uuid.UUID(guid_text).bytes_le
    return ", ".join(f"{b:02X}h" for b in data)


def sdk_summary(ctx: PSDKContext) -> str:
    return f"SDK {ctx.sdk_ver} at {ctx.include_root}"
