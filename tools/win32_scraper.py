#!/usr/bin/env python3
"""
win32_scraper.py
================
Extract Win32 API metadata from the local Windows SDK via LLVM/Clang and
generate fasm2 include file stubs for api/, equates/ (32+64), and pcount/.

Strategy
--------
* Export list  : llvm-nm --export-symbols <dll>.lib  (exact, DLL-authoritative)
* Functions    : clang -Xclang -ast-dump-filter=NAME  (5 KB per query vs 98 MB full dump)
* Structs/enums: same filtered queries, resolved recursively from param types
* Macros       : clang -E -dM on the target header, baseline-subtracted

Usage
-----
    python win32_scraper.py --dll shcore
    python win32_scraper.py --dll comctl32 --show-exports
    python win32_scraper.py --dll user32 --gap-only   # compare vs existing fasm2 includes

The script auto-discovers LLVM and the Windows SDK.  Override with
environment variables LLVM_BIN and WINSDK_ROOT if needed.

Output (dry-run by default, use --write to write files):
    include/api/<dll>.inc
    include/equates/<dll>32.inc
    include/equates/<dll>64.inc
    include/pcount/<dll>.inc
"""

import argparse
import json
import os
import re
import subprocess
import sys
import textwrap
import winreg
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# DLL → primary SDK headers (determines which declarations belong to each DLL)
# ---------------------------------------------------------------------------
DLL_HEADERS: dict[str, list[str]] = {
    "kernel32": [
        "processthreadsapi.h", "libloaderapi.h", "fileapi.h",
        "memoryapi.h", "handleapi.h", "synchapi.h", "heapapi.h",
        "ioapiset.h", "processenv.h", "errhandlingapi.h",
        "winbase.h",           # catch-all; comes last so narrower files win
    ],
    "user32":   ["winuser.h"],
    "gdi32":    ["wingdi.h"],
    "comctl32": ["commctrl.h"],
    "comdlg32": ["commdlg.h"],
    "shell32":  ["shellapi.h", "shlobj_core.h"],
    "shcore":   ["shellscalingapi.h"],
    "advapi32": ["winreg.h", "winsvc.h"],
    "ws2_32":   ["winsock2.h", "ws2tcpip.h"],
    "dwmapi":   ["dwmapi.h"],
    "uxtheme":  ["uxtheme.h"],
    "version":  ["winver.h"],
    "winmm":    ["mmeapi.h", "timeapi.h"],
    "shlwapi":  ["shlwapi.h"],
    "ole32":    ["objbase.h", "combaseapi.h"],
    "oleaut32": ["oleauto.h"],
}

# ---------------------------------------------------------------------------
# C-type → fasm2 directive mapping
# ---------------------------------------------------------------------------

# Pointer-sized handles: dd in 32-bit, dq in 64-bit
HANDLE_TYPES = frozenset({
    "HANDLE","HWND","HINSTANCE","HMODULE","HKEY","HMENU","HICON",
    "HCURSOR","HDC","HBITMAP","HBRUSH","HPEN","HFONT","HRGN",
    "HPALETTE","HGDIOBJ","HGLRC","HMONITOR","HTREEITEM","HIMAGELIST",
    "HACCEL","HRSRC","HGLOBAL","HLOCAL","HDESK","HWINSTA","HFILE",
    "HHOOK","HRAWINPUT","HPOWERNOTIFY","HCOLORSPACE","HENHMETAFILE",
    "HMETAFILE","HTHEME","HPSS","HSTRING","HSECTION",
})

# Pointer-sized scalars: SIZE_T, WPARAM, etc.
PTR_SIZED_TYPES = frozenset({
    "DWORD_PTR","UINT_PTR","INT_PTR","LONG_PTR","ULONG_PTR",
    "SIZE_T","SSIZE_T","WPARAM","LPARAM","LRESULT",
})

# Always 64-bit
QWORD_TYPES = frozenset({
    "LONGLONG","ULONGLONG","__int64","unsigned __int64",
    "INT64","UINT64","DWORDLONG","LARGE_INTEGER","ULARGE_INTEGER",
})

# Always 32-bit
DWORD_TYPES = frozenset({
    "DWORD","UINT","INT","LONG","ULONG","BOOL","COLORREF",
    "LCID","LANGID","LCTYPE","LGRPID","ATOM","HRESULT","NTSTATUS",
    "LONG32","UINT32","INT32","ULONG32","SCODE","float","FLOAT",
    "REGSAM","ACCESS_MASK","SECURITY_INFORMATION",
})

# Always 16-bit
WORD_TYPES = frozenset({"WORD","USHORT","SHORT","WCHAR","wchar_t"})

# Always 8-bit
BYTE_TYPES = frozenset({"BYTE","UCHAR","CHAR","BOOLEAN","char","unsigned char"})


def c_type_to_fasm2(qual_type: str, arch: int) -> tuple[str, int]:
    """
    Map a C qualified-type string (from clang AST) to (fasm2_directive, byte_size).
    Returns ("??", 0) for unknown/unresolved struct/enum types (caller should
    substitute the struct name as a field type).
    """
    t = qual_type.strip()

    # Strip leading qualifiers
    for q in ("const ", "volatile ", "unsigned ", "signed ", "__cdecl ",
              "__stdcall ", "WINAPI ", "CALLBACK "):
        t = t.replace(q, "")
    t = t.strip()

    # Pointer or array-of-pointer → address-sized
    if t.endswith("*") or t.endswith("* *") or t.startswith("LP") or t.startswith("P"):
        # Check it's not a false-positive like POINT
        if t.endswith("*") or t in (
            "LPVOID","PVOID","LPCVOID","LPCSTR","LPSTR","LPWSTR","LPCWSTR",
            "LPBYTE","LPDWORD","LPWORD","LPBOOL","LPLONG","LPINT",
            "LPTSTR","LPCTSTR","LPCCH","LPCH","LPWCH","LPCWCH",
        ):
            return ("dq", 8) if arch == 64 else ("dd", 4)

    base = t.rstrip("*").strip().split("[")[0].strip()

    if base in HANDLE_TYPES:
        return ("dq", 8) if arch == 64 else ("dd", 4)
    if base in PTR_SIZED_TYPES:
        return ("dq", 8) if arch == 64 else ("dd", 4)
    if base in QWORD_TYPES:
        return "dq", 8
    if base in DWORD_TYPES:
        return "dd", 4
    if base in WORD_TYPES:
        return "dw", 2
    if base in BYTE_TYPES:
        return "db", 1
    if base in ("VOID", "void"):
        # void return — not a field, skip
        return "??", 0

    # Unknown: struct, typedef, enum — caller deals with it
    return "??", 0


# ---------------------------------------------------------------------------
# Tool / SDK discovery
# ---------------------------------------------------------------------------

def find_llvm_bin() -> Path:
    env = os.environ.get("LLVM_BIN")
    if env:
        return Path(env)
    candidates = [
        Path(r"C:\Program Files\LLVM\bin"),
        Path(r"C:\Program Files (x86)\LLVM\bin"),
    ]
    for c in candidates:
        if (c / "clang.exe").exists():
            return c
    raise RuntimeError("Cannot find LLVM.  Set LLVM_BIN env var.")


def find_sdk() -> tuple[Path, str]:
    """Return (sdk_root, version_string) for the newest installed SDK."""
    env = os.environ.get("WINSDK_ROOT")
    if env:
        root = Path(env)
        versions = sorted(
            (d.name for d in (root / "Include").iterdir() if d.is_dir() and d.name.startswith("10.")),
            reverse=True,
        )
        return root, versions[0]

    # Registry lookup
    for hive in (winreg.HKEY_LOCAL_MACHINE,):
        for sub in (
            r"SOFTWARE\Microsoft\Windows Kits\Installed Roots",
            r"SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots",
        ):
            try:
                with winreg.OpenKey(hive, sub) as k:
                    root = Path(winreg.QueryValueEx(k, "KitsRoot10")[0])
                    versions = sorted(
                        (d.name for d in (root / "Include").iterdir()
                         if d.is_dir() and d.name.startswith("10.")),
                        reverse=True,
                    )
                    return root, versions[0]
            except (FileNotFoundError, OSError):
                pass
    raise RuntimeError("Cannot find Windows SDK.  Set WINSDK_ROOT env var.")


# ---------------------------------------------------------------------------
# Export extraction
# ---------------------------------------------------------------------------

def get_exports(dll_name: str, sdk_root: Path, sdk_ver: str, llvm_bin: Path) -> set[str]:
    """
    Return the set of public symbol names exported by dll_name.lib using llvm-nm.
    """
    lib_path = sdk_root / "Lib" / sdk_ver / "um" / "x64" / f"{dll_name}.lib"
    if not lib_path.exists():
        # Try alternate lib directories
        for arch in ("x64", "x86"):
            p = sdk_root / "Lib" / sdk_ver / "um" / arch / f"{dll_name}.lib"
            if p.exists():
                lib_path = p
                break
    if not lib_path.exists():
        raise FileNotFoundError(f"Cannot find {dll_name}.lib under {sdk_root}")

    nm = llvm_bin / "llvm-nm.exe"
    result = subprocess.run(
        [str(nm), "--export-symbols", str(lib_path)],
        capture_output=True, text=True
    )
    names: set[str] = set()
    for line in result.stdout.splitlines():
        name = line.strip()
        if not name:
            continue
        # Skip C++ mangled names, linker internals, and APIset forwarder stubs.
        # Note: single-underscore prefixes like _TrackMouseEvent are legitimate
        # Win32 exports; only filter double-underscore compiler/linker symbols.
        if (name.startswith("?") or name.startswith("__") or
                "_NULL_THUNK_DATA" in name or "THUNK_DATA" in name or
                "@" in name or "#" in name):
            continue
        names.add(name)
    return names


# ---------------------------------------------------------------------------
# Clang AST queries
# ---------------------------------------------------------------------------

def _clang_args(llvm_bin: Path, sdk_root: Path, sdk_ver: str,
                headers: list[str], arch: int,
                extra: list[str] | None = None) -> list[str]:
    """Build clang invocation for a single-include wrapper."""
    inc = sdk_root / "Include" / sdk_ver
    target = "x86_64-pc-windows-msvc" if arch == 64 else "i686-pc-windows-msvc"
    args = [
        str(llvm_bin / "clang.exe"),
        "-Xclang", "-ast-dump=json",
        "-fsyntax-only",
        "-target", target,
        f"-I{inc / 'um'}",
        f"-I{inc / 'shared'}",
        f"-I{inc / 'ucrt'}",
        "-x", "c",
    ]
    if extra:
        args += extra
    return args


def _make_wrapper(headers: list[str]) -> str:
    lines = [
        "#define WIN32_LEAN_AND_MEAN",
        "#define UNICODE",
        "#define _UNICODE",
        "#define NOMINMAX",
        "#define STRICT",
    ]
    for h in headers:
        lines.append(f"#include <{h}>")
    return "\n".join(lines) + "\n"


def query_ast_filter(name: str, headers: list[str],
                     llvm_bin: Path, sdk_root: Path, sdk_ver: str,
                     arch: int = 64) -> dict | None:
    """
    Run clang with -ast-dump-filter=name and return the parsed JSON root,
    or None if nothing was found.  Keeps each round-trip tiny (~5 KB).
    """
    import tempfile
    wrapper = _make_wrapper(headers)
    with tempfile.NamedTemporaryFile(suffix=".c", mode="w",
                                    delete=False, encoding="utf-8") as f:
        f.write(wrapper)
        tmp_path = f.name

    try:
        args = _clang_args(llvm_bin, sdk_root, sdk_ver, headers, arch,
                           extra=["-Xclang", f"-ast-dump-filter={name}"])
        args.append(tmp_path)
        result = subprocess.run(args, capture_output=True, text=True)
        txt = result.stdout.strip()
        if not txt or txt == "null":
            return None
        # clang wraps filtered output in a synthetic TranslationUnitDecl
        return json.loads(txt)
    except json.JSONDecodeError:
        return None
    finally:
        os.unlink(tmp_path)


def extract_function_info(ast_root: dict, func_name: str) -> dict | None:
    """
    Walk the AST and return the first FunctionDecl matching func_name.
    Returns dict with keys: name, params, return_type, is_variadic, file.
    """
    if ast_root is None:
        return None

    def walk(node):
        if not isinstance(node, dict):
            return None
        if node.get("kind") == "FunctionDecl" and node.get("name") == func_name:
            params = []
            for child in node.get("inner", []):
                if child.get("kind") == "ParmVarDecl":
                    params.append({
                        "name": child.get("name", ""),
                        "type": child.get("type", {}).get("qualType", ""),
                    })
            return {
                "name": func_name,
                "params": params,
                "return_type": node.get("type", {}).get("qualType", ""),
                "is_variadic": node.get("variadic", False),
                "file": node.get("loc", {}).get("file", ""),
            }
        for child in node.get("inner", []):
            r = walk(child)
            if r:
                return r
        return None

    return walk(ast_root)


def extract_record_info(ast_root: dict, type_name: str) -> dict | None:
    """
    Walk the AST for a RecordDecl (struct/union) matching type_name.
    Returns dict with: name, kind, fields [{name, type, bits}].
    """
    if ast_root is None:
        return None

    def walk(node):
        if not isinstance(node, dict):
            return None
        kind = node.get("kind", "")
        if kind in ("RecordDecl", "CXXRecordDecl") and node.get("name") == type_name:
            fields = []
            for child in node.get("inner", []):
                if child.get("kind") == "FieldDecl":
                    fields.append({
                        "name": child.get("name", ""),
                        "type": child.get("type", {}).get("qualType", ""),
                        "bits": child.get("isBitfield"),
                    })
            if fields or node.get("completeDefinition"):
                return {"name": type_name,
                        "kind": node.get("tagUsed", "struct"),
                        "fields": fields}
        for child in node.get("inner", []):
            r = walk(child)
            if r:
                return r
        return None

    return walk(ast_root)


def extract_enum_info(ast_root: dict, enum_name: str) -> dict | None:
    """
    Walk the AST for an EnumDecl matching enum_name.
    Returns dict with: name, constants [{name, value}].
    """
    if ast_root is None:
        return None

    def walk(node):
        if not isinstance(node, dict):
            return None
        if node.get("kind") == "EnumDecl" and node.get("name") == enum_name:
            constants = []
            for child in node.get("inner", []):
                if child.get("kind") == "EnumConstantDecl":
                    # The integer value is in a child IntegerLiteral or
                    # available as node.get("value") in newer clang
                    val = None
                    for gc in child.get("inner", []):
                        if "value" in gc:
                            val = gc["value"]
                            break
                    if val is None:
                        val = child.get("value")
                    constants.append({"name": child.get("name", ""), "value": val})
            return {"name": enum_name, "constants": constants}
        for child in node.get("inner", []):
            r = walk(child)
            if r:
                return r
        return None

    return walk(ast_root)


# ---------------------------------------------------------------------------
# Macro / constant extraction
# ---------------------------------------------------------------------------

_INT_RE  = re.compile(r"^-?\d+$")
_HEX_RE  = re.compile(r"^0[xX][0-9A-Fa-f]+$")
_EXPR_RE = re.compile(r"^[\w\s\+\-\|\&\~\(\)\d]+$")  # simple expressions

def get_macros(headers: list[str], llvm_bin: Path, sdk_root: Path,
               sdk_ver: str, arch: int = 64,
               baseline_headers: list[str] | None = None) -> dict[str, str]:
    """
    Return {name: value} for numeric macros that `headers` add over the baseline.

    The baseline defaults to ["windows.h"] so we isolate only the constants
    new to the DLL's specific headers, not the entire Windows API.
    Passing baseline_headers=[] gives everything (useful for header-only libs).
    """
    import tempfile

    if baseline_headers is None:
        baseline_headers = ["windows.h"]

    def run_dM(include_src: str) -> dict[str, str]:
        with tempfile.NamedTemporaryFile(suffix=".c", mode="w",
                                        delete=False, encoding="utf-8") as f:
            f.write(include_src)
            tmp = f.name
        inc = sdk_root / "Include" / sdk_ver
        target = "x86_64-pc-windows-msvc" if arch == 64 else "i686-pc-windows-msvc"
        args = [
            str(llvm_bin / "clang.exe"), "-E", "-dM",
            "-target", target,
            f"-I{inc / 'um'}", f"-I{inc / 'shared'}", f"-I{inc / 'ucrt'}",
            "-x", "c", tmp,
        ]
        try:
            r = subprocess.run(args, capture_output=True, text=True)
        finally:
            Path(tmp).unlink(missing_ok=True)
        out: dict[str, str] = {}
        for line in r.stdout.splitlines():
            m = re.match(r"^#define\s+(\w+)\s+(.*)", line)
            if m:
                out[m.group(1)] = m.group(2).strip()
        return out

    baseline_src = _make_wrapper(baseline_headers) if baseline_headers else ""
    baseline     = run_dM(baseline_src)
    full         = run_dM(_make_wrapper(headers))

    new: dict[str, str] = {}
    for name, value in full.items():
        if name in baseline:
            continue
        if name.startswith("_") or name.startswith("__") or len(name) < 2:
            continue
        v      = value.strip()
        v_bare = re.sub(r"^\(+", "", re.sub(r"\)+$", "", v))
        if _INT_RE.match(v_bare) or _HEX_RE.match(v_bare):
            new[name] = v
        elif _EXPR_RE.match(v) and any(c.isdigit() for c in v):
            new[name] = v
    return new


# ---------------------------------------------------------------------------
# fasm2 code generation helpers
# ---------------------------------------------------------------------------

def _fasm2_struct_32(info: dict, known_types: dict[str, str]) -> str:
    """Generate a 32-bit fasm2 struct definition from a RecordDecl info dict."""
    lines = [f"struct {info['name']}"]
    offset = 0
    for field in info["fields"]:
        fname = field["name"] or f"_pad{offset}"
        ftype = field["type"].strip()

        # Array check: "TYPE [N]" or "TYPE [N][M]"
        arr_m = re.search(r"\[(\d+)\](?:\[(\d+)\])?$", ftype)
        count = 1
        if arr_m:
            count = int(arr_m.group(1))
            if arr_m.group(2):
                count *= int(arr_m.group(2))
            ftype = ftype[:arr_m.start()].strip()

        dir_, sz = c_type_to_fasm2(ftype, arch=32)
        if dir_ == "??":
            # Look up known struct/enum in our collected set
            base = ftype.rstrip("*").strip().split()[-1]
            known = known_types.get(base)
            if known == "enum":
                dir_, sz = "dd", 4
            elif known in ("struct", "union"):
                # Nested struct: use its name as the field type
                dup = f" {count} dup (?)" if count > 1 else ""
                lines.append(f"  {fname:<22} {base}{dup}")
                offset += sz if sz else 0  # sz unknown for nested, approximate
                continue
            else:
                dir_, sz = "dd", 4  # fallback

        dup = f" {count} dup (?)" if count > 1 else " ?"
        lines.append(f"  {fname:<22} {dir_}{dup}")
        offset += sz * count
    lines.append("ends")
    return "\n".join(lines)


def _fasm2_struct_64(info: dict, known_types: dict[str, str]) -> str:
    """
    Generate a 64-bit fasm2 struct definition.
    Inserts explicit anonymous padding fields to maintain natural alignment.
    """
    lines = [f"struct {info['name']}"]
    offset = 0

    def pad_to(target: int, current: int) -> list[str]:
        gap = target - current
        if gap <= 0:
            return []
        padlines = []
        while gap >= 8:
            padlines.append("                       dq ?")
            gap -= 8
        while gap >= 4:
            padlines.append("                       dd ?")
            gap -= 4
        while gap >= 2:
            padlines.append("                       dw ?")
            gap -= 2
        while gap >= 1:
            padlines.append("                       db ?")
            gap -= 1
        return padlines

    for field in info["fields"]:
        fname = field["name"] or ""
        ftype = field["type"].strip()

        arr_m = re.search(r"\[(\d+)\](?:\[(\d+)\])?$", ftype)
        count = 1
        if arr_m:
            count = int(arr_m.group(1))
            if arr_m.group(2):
                count *= int(arr_m.group(2))
            ftype = ftype[:arr_m.start()].strip()

        dir_, sz = c_type_to_fasm2(ftype, arch=64)
        if dir_ == "??":
            base = ftype.rstrip("*").strip().split()[-1]
            known = known_types.get(base)
            if known == "enum":
                dir_, sz = "dd", 4
            elif known in ("struct", "union"):
                # Can't easily compute nested struct size here — emit as-is
                dup = f" {count} dup (?)" if count > 1 else ""
                lines.append(f"  {fname:<22} {base}{dup}")
                continue
            else:
                dir_, sz = "dd", 4

        # Insert alignment padding before this field
        align = min(sz, 8)  # max natural alignment is 8 on x64
        if align > 1 and (offset % align) != 0:
            pad_target = ((offset + align - 1) // align) * align
            lines += pad_to(pad_target, offset)
            offset = pad_target

        label = fname if fname else ""
        dup = f" {count} dup (?)" if count > 1 else " ?"
        prefix = f"  {label:<22}" if label else "  " + " " * 22
        lines.append(f"{prefix} {dir_}{dup}")
        offset += sz * count

    lines.append("ends")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Struct layout verification
# ---------------------------------------------------------------------------

#: Byte size of each fasm2 data directive
_DIRECTIVE_SIZE: dict[str, int] = {
    "db": 1, "dw": 2, "dd": 4, "dq": 8, "dt": 10,
    "TCHAR": 2,  # assume Unicode build
}


def get_sdk_struct_layout(
    struct_name: str,
    headers: list[str],
    llvm_bin: Path,
    sdk_root: Path,
    sdk_ver: str,
    arch: int = 64,
) -> dict | None:
    """
    Return the SDK's authoritative packed layout for *struct_name* using
    clang -Xclang -fdump-record-layouts-complete.

    Returns a dict:
        {
            "size":   total size in bytes,
            "align":  alignment in bytes (1 = packed),
            "fields": [(field_name, byte_offset, size_bytes), ...],
        }
    or None if the struct is not found.
    """
    import tempfile

    inc_dir = sdk_root / "Include" / sdk_ver
    um     = inc_dir / "um"
    shared = inc_dir / "shared"
    ucrt   = inc_dir / "ucrt"

    target = "x86_64-pc-windows-msvc" if arch == 64 else "i686-pc-windows-msvc"

    # Build a small C file that forces the struct to be instantiated
    include_block = "\n".join(f"#include <{h}>" for h in headers)
    c_src = (
        "#define NTDDI_VERSION 0x06000000\n"
        "#define _WIN32_WINNT 0x0600\n"
        "#include <windows.h>\n"
        f"{include_block}\n"
        f"// Force layout emission:\n"
        f"{struct_name} __verify_struct_instance;\n"
    )

    with tempfile.NamedTemporaryFile(suffix=".c", delete=False, mode="w",
                                     encoding="utf-8") as f:
        f.write(c_src)
        tmp_path = f.name

    try:
        clang = str(llvm_bin / "clang.exe")
        cmd = [
            clang, "-target", target, "-fsyntax-only",
            "-Xclang", "-fdump-record-layouts-complete",
            f"-I{um}", f"-I{shared}", f"-I{ucrt}",
            "-D_WIN32", "-DWIN32", "-DUNICODE", "-D_UNICODE",
        ] + (["-D_WIN64"] if arch == 64 else []) + [
            "-w",  # suppress warnings
            tmp_path,
        ]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        out = result.stdout
    finally:
        Path(tmp_path).unlink(missing_ok=True)

    # Find the block for this struct.  Clang uses the tag-decl name like
    # "struct _TASKDIALOGCONFIG" for typedef'd structs.
    candidates = [
        f"struct {struct_name}\n",
        f"struct _{struct_name}\n",
        f"struct tag{struct_name}\n",
    ]
    block_text = None
    for cand in candidates:
        idx = out.find(cand)
        if idx >= 0:
            block_start = out.rfind("*** Dumping", 0, idx)
            sizeof_idx  = out.find("sizeof=", idx)
            if sizeof_idx < 0:
                continue
            block_end   = out.find("\n", sizeof_idx) + 1
            block_text  = out[block_start:block_end]
            break

    if block_text is None:
        return None

    # Parse fields: lines like "        36 |   PCWSTR pszWindowTitle"
    # Skip anonymous union/struct wrapper lines (they contain "(anonymous")
    fields: list[tuple[str, int, int]] = []
    prev_offset = -1
    prev_size   = 0

    field_re  = re.compile(r"^\s*(\d+)\s+\|\s+\S.*\s(\w+)$")
    sizeof_re = re.compile(r"\[sizeof=(\d+),\s*align=(\d+)\]")

    total_size = 0
    align_val  = 1

    for line in block_text.splitlines():
        sm = sizeof_re.search(line)
        if sm:
            total_size = int(sm.group(1))
            align_val  = int(sm.group(2))
            continue
        if "anonymous" in line or "unnamed" in line:
            continue   # skip union/struct container lines
        fm = field_re.match(line)
        if not fm:
            continue
        offset    = int(fm.group(1))
        field_name = fm.group(2)
        # Compute size of previous field = current_offset - prev_offset
        if prev_offset >= 0 and prev_offset != offset:
            # Update last field's size
            if fields and fields[-1][2] == 0:
                fields[-1] = (fields[-1][0], fields[-1][1], offset - prev_offset)
        fields.append((field_name, offset, 0))
        prev_offset = offset

    # Size of last field = total_size - last_offset
    if fields:
        last = fields[-1]
        fields[-1] = (last[0], last[1], total_size - last[1])

    return {"size": total_size, "align": align_val, "fields": fields}


def parse_fasm2_struct(
    struct_name: str,
    fasm2_root: Path,
    arch: int = 64,
) -> dict | None:
    """
    Scan equates/{name}32.inc and equates/{name}64.inc for *struct_name* and
    return its field layout as computed by walking the fasm2 data directives.

    Returns:
        {
            "packed": bool,
            "fields": [(field_name, byte_offset, size_bytes), ...],
            "size":   total size in bytes,
        }
    or None if not found.
    """
    suffix = "64" if arch == 64 else "32"
    # Search all equates files (some structs are in un-suffixed files)
    candidates = list((fasm2_root / "include" / "equates").glob("*.inc"))

    struct_re = re.compile(
        r"^struct\s+" + re.escape(struct_name) + r"(\s*,\s*packed)?\s*$",
        re.IGNORECASE,
    )
    field_re = re.compile(
        r"^\s+(?:(\w+)\s+)?"               # optional field name
        r"(db|dw|dd|dq|dt|TCHAR|du)\s+"    # directive
        r"(?:(\d+)\s+dup\s*\(\s*\?\s*\))?" # optional count  dup (?)
        r"\s*\??",                          # or bare ?
        re.IGNORECASE,
    )

    for inc_path in candidates:
        # Prefer arch-appropriate file
        if arch == 64 and "32.inc" in inc_path.name:
            continue
        if arch == 32 and "64.inc" in inc_path.name:
            continue

        text = inc_path.read_text(encoding="utf-8", errors="ignore")
        lines = text.splitlines()

        for i, line in enumerate(lines):
            if struct_re.match(line.strip()):
                packed = "packed" in line.lower()
                fields: list[tuple[str, int, int]] = []
                offset = 0

                for fline in lines[i + 1:]:
                    stripped = fline.strip()
                    if stripped.lower() == "ends":
                        break
                    fm = field_re.match(fline)
                    if not fm:
                        continue
                    fname    = fm.group(1) or ""
                    dir_     = fm.group(2).lower()
                    count    = int(fm.group(3)) if fm.group(3) else 1
                    elem_sz  = _DIRECTIVE_SIZE.get(dir_, 0)
                    fsize    = elem_sz * count

                    if not packed:
                        # Apply natural alignment (max 8 on x64, 4 on x86)
                        max_align = 8 if arch == 64 else 4
                        align = min(elem_sz, max_align)
                        if align > 1:
                            offset = ((offset + align - 1) // align) * align

                    fields.append((fname, offset, fsize))
                    offset += fsize

                return {"packed": packed, "fields": fields, "size": offset,
                        "file": str(inc_path)}

    return None


def verify_struct(
    struct_name: str,
    dll_name: str,
    fasm2_root: Path,
    llvm_bin: Path,
    sdk_root: Path,
    sdk_ver: str,
    arch: int = 64,
) -> str:
    """
    Cross-check the fasm2 struct definition against the SDK's actual layout.
    Returns a human-readable report string.
    """
    headers = DLL_HEADERS.get(dll_name, ["windows.h"])

    sdk = get_sdk_struct_layout(struct_name, headers, llvm_bin, sdk_root, sdk_ver, arch)
    fasm = parse_fasm2_struct(struct_name, fasm2_root, arch)

    lines = [
        f"Struct layout verification: {struct_name}  (arch={arch})",
        "=" * 60,
    ]

    if sdk is None:
        lines.append(f"  SDK: struct not found in {dll_name} headers")
    else:
        lines.append(f"  SDK: sizeof={sdk['size']}, align={sdk['align']}"
                     f"  ({'packed' if sdk['align'] == 1 else 'default-aligned'})")

    if fasm is None:
        lines.append(f"  fasm2: struct not found in equates files (arch={arch})")
    else:
        packed_str = "packed" if fasm["packed"] else "non-packed"
        lines.append(f"  fasm2: sizeof={fasm['size']}, {packed_str}"
                     f"  ({fasm['file'].split(chr(92))[-1]})")

    if sdk is None or fasm is None:
        return "\n".join(lines)

    # Size check
    if sdk["size"] != fasm["size"]:
        lines.append(f"\n  *** SIZE MISMATCH: SDK={sdk['size']} fasm2={fasm['size']} ***")
    else:
        lines.append(f"\n  Size match: {sdk['size']} bytes ✓")

    # Field-by-field comparison
    # Build offset→field maps
    sdk_by_offset  = {f[1]: f for f in sdk["fields"]}
    fasm_by_offset = {f[1]: f for f in fasm["fields"] if f[0]}  # skip unnamed padding

    all_offsets = sorted(set(sdk_by_offset) | set(fasm_by_offset))
    mismatches = []
    for off in all_offsets:
        s = sdk_by_offset.get(off)
        f_ = fasm_by_offset.get(off)
        if s and f_:
            sdk_end  = s[1] + s[2]
            fasm_end = f_[1] + f_[2]
            if sdk_end != fasm_end:
                mismatches.append(
                    f"  offset {off:4d}: SDK  {s[0]}  [{s[1]}..{sdk_end})\n"
                    f"               fasm2 {f_[0]} [{f_[1]}..{fasm_end})"
                )
        elif s and not f_:
            # Might be covered by a dup field
            covered = any(
                f[1] <= off < f[1] + f[2]
                for f in fasm["fields"]
            )
            if not covered:
                mismatches.append(f"  offset {off:4d}: SDK  {s[0]}  — no fasm2 field")
        elif f_ and not s:
            mismatches.append(f"  offset {off:4d}: fasm2 {f_[0]}  — no SDK field")

    if mismatches:
        lines.append("\n  Field mismatches:")
        lines.extend(mismatches)
    else:
        lines.append("  Fields match ✓")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Existing fasm2 include gap analysis
# ---------------------------------------------------------------------------

def load_existing_exports(fasm2_root: Path, dll_name: str) -> set[str]:
    """Read existing api/<dll>.inc and extract imported function names."""
    inc = fasm2_root / "include" / "api" / f"{dll_name}.inc"
    if not inc.exists():
        return set()
    names: set[str] = set()
    for line in inc.read_text(encoding="utf-8", errors="ignore").splitlines():
        m = re.match(r"\s+(\w+),'[^']+',?\\?", line)
        if m:
            names.add(m.group(1))
    return names


def load_existing_pcounts(fasm2_root: Path, dll_name: str) -> set[str]:
    """Return set of function names with pcount entries."""
    inc = fasm2_root / "include" / "pcount" / f"{dll_name}.inc"
    if not inc.exists():
        return set()
    names: set[str] = set()
    for line in inc.read_text(encoding="utf-8", errors="ignore").splitlines():
        m = re.match(r"(\w+)%\s*=", line)
        if m:
            names.add(m.group(1))
    return names


# ---------------------------------------------------------------------------
# Main pipeline
# ---------------------------------------------------------------------------

class Scraper:
    def __init__(self, dll_name: str, llvm_bin: Path,
                 sdk_root: Path, sdk_ver: str, fasm2_root: Path):
        self.dll_name  = dll_name
        self.llvm_bin  = llvm_bin
        self.sdk_root  = sdk_root
        self.sdk_ver   = sdk_ver
        self.fasm2_root = fasm2_root
        self.headers   = DLL_HEADERS.get(dll_name, [])
        # Cache of queried types: name → "struct"|"union"|"enum"|"unknown"
        self._type_cache: dict[str, str]   = {}
        self._struct_cache: dict[str, dict] = {}
        self._enum_cache:   dict[str, dict] = {}

    # -- type resolution helpers -------------------------------------------

    def _resolve_type(self, type_name: str) -> str:
        """Query the AST to determine if type_name is a struct, union, or enum."""
        if type_name in self._type_cache:
            return self._type_cache[type_name]
        ast = query_ast_filter(type_name, self.headers, self.llvm_bin,
                               self.sdk_root, self.sdk_ver)
        if ast is None:
            self._type_cache[type_name] = "unknown"
            return "unknown"

        def scan(node):
            if not isinstance(node, dict):
                return None
            k = node.get("kind", "")
            n = node.get("name", "")
            if n == type_name:
                if k in ("RecordDecl", "CXXRecordDecl"):
                    return node.get("tagUsed", "struct")
                if k == "EnumDecl":
                    return "enum"
            for child in node.get("inner", []):
                r = scan(child)
                if r:
                    return r
            return None

        result = scan(ast) or "unknown"
        self._type_cache[type_name] = result
        return result

    def _collect_struct(self, type_name: str) -> dict | None:
        if type_name in self._struct_cache:
            return self._struct_cache[type_name]
        ast = query_ast_filter(type_name, self.headers, self.llvm_bin,
                               self.sdk_root, self.sdk_ver)
        info = extract_record_info(ast, type_name) if ast else None
        self._struct_cache[type_name] = info
        return info

    def _collect_enum(self, type_name: str) -> dict | None:
        if type_name in self._enum_cache:
            return self._enum_cache[type_name]
        ast = query_ast_filter(type_name, self.headers, self.llvm_bin,
                               self.sdk_root, self.sdk_ver)
        info = extract_enum_info(ast, type_name) if ast else None
        self._enum_cache[type_name] = info
        return info

    # -- core pipeline -------------------------------------------------------

    def run(self, show_exports=False, gap_only=False, arch_filter=0) -> dict:
        """
        Execute the scraper pipeline.  Returns a results dict with:
          exports, functions, structs, enums, macros, gaps
        """
        print(f"[*] Scraping {self.dll_name} ...")

        # 1. Authoritative export list from the .lib file
        print(f"    Getting exports from {self.dll_name}.lib ...")
        exports = get_exports(self.dll_name, self.sdk_root,
                              self.sdk_ver, self.llvm_bin)
        print(f"    {len(exports)} exports found")

        if show_exports:
            for e in sorted(exports):
                print(f"      {e}")

        if not self.headers:
            print(f"    WARNING: no headers configured for {self.dll_name}")
            return {"exports": exports}

        # 2. Gap analysis against existing fasm2 includes
        existing_api    = load_existing_exports(self.fasm2_root, self.dll_name)
        existing_pcount = load_existing_pcounts(self.fasm2_root, self.dll_name)

        missing_api    = exports - existing_api
        missing_pcount = {
            e for e in exports
            if e not in existing_pcount and
               e.rstrip("AW") not in existing_pcount
        }

        gaps = {
            "missing_api":    sorted(missing_api),
            "missing_pcount": sorted(missing_pcount),
            "orphan_api":     sorted(existing_api - exports),
        }

        print(f"    api gap:    {len(missing_api)} missing, {len(gaps['orphan_api'])} orphaned")
        print(f"    pcount gap: {len(missing_pcount)} missing")

        if gap_only:
            return {"exports": exports, "gaps": gaps}

        # 3. Query function signatures for missing exports
        print(f"    Querying AST for function signatures ...")
        functions: dict[str, dict] = {}
        needed_types: set[str] = set()

        targets = missing_api if existing_api else exports  # full scrape if no existing file
        no_header: list[str] = []
        for fn_name in sorted(targets):
            ast = query_ast_filter(fn_name, self.headers, self.llvm_bin,
                                   self.sdk_root, self.sdk_ver)
            if ast is None:
                no_header.append(fn_name)
                continue
            info = extract_function_info(ast, fn_name)
            if info:
                functions[fn_name] = info
                for p in info["params"]:
                    base = p["type"].rstrip("*").strip().split()[-1]
                    if base and base[0].isupper():
                        needed_types.add(base)
            else:
                no_header.append(fn_name)

        print(f"    {len(functions)} function signatures resolved")
        if no_header:
            print(f"    {len(no_header)} not in configured headers (undocumented/WinRT):")
            for fn in no_header:
                print(f"      {fn}")

        # 4. Resolve all referenced struct/enum types
        print(f"    Resolving {len(needed_types)} referenced types ...")
        structs: dict[str, dict] = {}
        enums:   dict[str, dict] = {}

        queue = list(needed_types)
        visited: set[str] = set()

        while queue:
            tname = queue.pop(0)
            if tname in visited:
                continue
            visited.add(tname)

            kind = self._resolve_type(tname)
            if kind in ("struct", "union"):
                info = self._collect_struct(tname)
                if info:
                    structs[tname] = info
                    # Recurse into field types
                    for f in info.get("fields", []):
                        sub = f["type"].rstrip("*").strip().split()[-1]
                        if sub and sub[0].isupper() and sub not in visited:
                            queue.append(sub)
            elif kind == "enum":
                info = self._collect_enum(tname)
                if info:
                    enums[tname] = info

        print(f"    {len(structs)} structs, {len(enums)} enums resolved")

        # 5. Extract macros (numeric constants new to this DLL's headers)
        print(f"    Extracting macros ...")
        macros = get_macros(self.headers, self.llvm_bin, self.sdk_root, self.sdk_ver)
        print(f"    {len(macros)} constants extracted")

        return {
            "exports":   exports,
            "functions": functions,
            "structs":   structs,
            "enums":     enums,
            "macros":    macros,
            "gaps":      gaps,
        }


# ---------------------------------------------------------------------------
# fasm2 output generation
# ---------------------------------------------------------------------------

def _aw_stem(name: str) -> str | None:
    """Return the A/W stem if name ends in A or W and stem is plausible."""
    if len(name) > 1 and name[-1] in ("A", "W") and name[-2].isupper():
        return name[:-1]
    return None


def gen_api_inc(dll_name: str, functions: dict[str, dict],
                exports: set[str]) -> str:
    """Generate api/<dll>.inc content."""
    # Detect A/W pairs and build generic aliases
    stems: dict[str, list[str]] = {}
    for fn in sorted(exports):
        stem = _aw_stem(fn)
        if stem:
            stems.setdefault(stem, []).append(fn)

    pairs:    set[str] = set()  # stems with both A and W
    singles:  set[str] = set()  # stems with only one variant
    for stem, variants in stems.items():
        has_a = stem + "A" in exports
        has_w = stem + "W" in exports
        if has_a and has_w:
            pairs.add(stem)
        elif has_a or has_w:
            singles.add(stem)

    lines = ["", f"; {dll_name.upper()} API calls", ""]
    lines.append(f"import {dll_name},\\")

    import_lines = []
    for fn in sorted(exports):
        import_lines.append(f"       {fn},'{fn}',\\")
    # Remove trailing backslash from last entry
    if import_lines:
        import_lines[-1] = import_lines[-1].rstrip(",\\")
    lines += import_lines
    lines.append("")

    if pairs or singles:
        lines.append(f"api {', '.join(sorted(pairs | singles))}")
        lines.append("")

    return "\n".join(lines)


def gen_pcount_inc(dll_name: str, functions: dict[str, dict],
                   exports: set[str]) -> str:
    """Generate pcount/<dll>.inc content."""
    lines = ["", f"; {dll_name.upper()} API calls parameters' count", ""]
    emitted: dict[str, int] = {}

    stems: dict[str, int] = {}
    for fn in sorted(exports):
        info = functions.get(fn)
        count = len(info["params"]) if info else -1
        if count < 0:
            continue
        # Emit specific A/W name
        lines.append(f"{fn}% = {count:2d}")
        emitted[fn] = count
        # Track stem for generic alias
        stem = _aw_stem(fn)
        if stem:
            stems.setdefault(stem, count)

    # Emit generic stem aliases (use the count of the A or W variant)
    lines.append("")
    for stem, count in sorted(stems.items()):
        if stem not in emitted:
            lines.append(f"{stem}% = {count:2d}")

    return "\n".join(lines)


def gen_equates_inc(dll_name: str, structs: dict[str, dict],
                    enums: dict[str, dict], macros: dict[str, str],
                    arch: int) -> str:
    """Generate equates/<dll>{32|64}.inc content."""
    suffix = str(arch)
    known_types = {n: info["kind"] for n, info in structs.items()}
    known_types.update({n: "enum" for n in enums})

    lines = ["", f"; {dll_name.upper()}.DLL structures and constants ({arch}-bit)", ""]

    # Structs
    for name, info in sorted(structs.items()):
        if arch == 32:
            lines.append(_fasm2_struct_32(info, known_types))
        else:
            lines.append(_fasm2_struct_64(info, known_types))
        lines.append("")

    # Enums → flat constants
    for name, info in sorted(enums.items()):
        lines.append(f"; {name}")
        for const in info.get("constants", []):
            cname  = const["name"]
            cvalue = const["value"]
            if cvalue is not None:
                # Format as hex if large enough
                try:
                    iv = int(cvalue)
                    if iv >= 16:
                        cvalue = hex(iv)
                except (ValueError, TypeError):
                    pass
                lines.append(f"{cname:<40} = {cvalue}")
        lines.append("")

    # Macros
    if macros:
        lines.append("; Constants")
        lines.append("")
        for name, value in sorted(macros.items()):
            lines.append(f"{name:<40} = {value}")
        lines.append("")

    return "\n".join(lines)


def gen_gap_report(dll_name: str, gaps: dict) -> str:
    """Generate a human-readable gap report."""
    lines = [
        f"Gap analysis for {dll_name.upper()}",
        "=" * 60,
    ]

    ma = gaps.get("missing_api", [])
    if ma:
        lines.append(f"\nMissing from api/{dll_name}.inc ({len(ma)}):")
        # Group A/W pairs
        seen: set[str] = set()
        for fn in ma:
            if fn in seen:
                continue
            seen.add(fn)
            stem = _aw_stem(fn)
            if stem and stem + "A" in gaps["missing_api"] and stem + "W" in gaps["missing_api"]:
                seen.add(stem + "A")
                seen.add(stem + "W")
                lines.append(f"  {stem}A / {stem}W  (+ generic alias)")
            else:
                lines.append(f"  {fn}")

    mp = gaps.get("missing_pcount", [])
    if mp:
        lines.append(f"\nMissing from pcount/{dll_name}.inc ({len(mp)}):")
        for fn in mp[:40]:
            lines.append(f"  {fn}")
        if len(mp) > 40:
            lines.append(f"  ... and {len(mp)-40} more")

    oa = gaps.get("orphan_api", [])
    if oa:
        lines.append(f"\nIn fasm2 api/{dll_name}.inc but NOT in .lib ({len(oa)}):")
        for fn in oa:
            lines.append(f"  {fn}")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(
        description="Extract Win32 API info from PSDK and generate fasm2 include stubs")
    ap.add_argument("--dll", required=True,
                    help="DLL name without extension (e.g. shcore, comctl32)")
    ap.add_argument("--show-exports", action="store_true",
                    help="Print all exported symbol names and exit")
    ap.add_argument("--gap-only", action="store_true",
                    help="Only report gaps vs existing fasm2 includes")
    ap.add_argument("--verify-struct", metavar="STRUCTNAME", default=None,
                    help="Cross-check a struct definition against SDK layout"
                         " (use with --dll to pick headers; use --arch 32|64)")
    ap.add_argument("--arch", type=int, choices=[32, 64], default=64,
                    help="Target architecture for --verify-struct (default: 64)")
    ap.add_argument("--write", action="store_true",
                    help="Write generated files to --out-dir")
    ap.add_argument("--out-dir", default=None,
                    help="Root of fasm2 repo (default: parent of tools/)")
    ap.add_argument("--llvm", default=None, help="Override LLVM bin dir")
    ap.add_argument("--sdk",  default=None, help="Override SDK root")
    ap.add_argument("--sdk-ver", default=None, help="Override SDK version string")
    args = ap.parse_args()

    # -- Discover tools -------------------------------------------------------
    llvm_bin = Path(args.llvm) if args.llvm else find_llvm_bin()
    if args.sdk:
        sdk_root = Path(args.sdk)
        sdk_ver  = args.sdk_ver or sorted(
            (d.name for d in (sdk_root / "Include").iterdir() if d.is_dir()),
            reverse=True)[0]
    else:
        sdk_root, sdk_ver = find_sdk()

    fasm2_root = (Path(args.out_dir) if args.out_dir
                  else Path(__file__).parent.parent)

    print(f"[*] LLVM:    {llvm_bin}")
    print(f"[*] SDK:     {sdk_root / 'Include' / sdk_ver}")
    print(f"[*] fasm2:   {fasm2_root}")
    print()

    # -- Struct verification shortcut -----------------------------------------
    if args.verify_struct:
        report = verify_struct(
            args.verify_struct, args.dll, fasm2_root,
            llvm_bin, sdk_root, sdk_ver, arch=args.arch,
        )
        print(report)
        return

    # -- Run scraper ----------------------------------------------------------
    scraper = Scraper(args.dll, llvm_bin, sdk_root, sdk_ver, fasm2_root)
    results = scraper.run(
        show_exports=args.show_exports,
        gap_only=args.gap_only,
    )

    # -- Gap report -----------------------------------------------------------
    if "gaps" in results:
        print()
        print(gen_gap_report(args.dll, results["gaps"]))

    if args.gap_only or args.show_exports:
        return

    # -- Generate output ------------------------------------------------------
    functions = results.get("functions", {})
    structs   = results.get("structs", {})
    enums     = results.get("enums", {})
    macros    = results.get("macros", {})
    exports   = results.get("exports", set())

    api_txt    = gen_api_inc(args.dll, functions, exports)
    pcount_txt = gen_pcount_inc(args.dll, functions, exports)
    eq32_txt   = gen_equates_inc(args.dll, structs, enums, macros, arch=32)
    eq64_txt   = gen_equates_inc(args.dll, structs, enums, macros, arch=64)

    if args.write:
        api_path = fasm2_root / "include" / "api"    / f"{args.dll}.inc"
        pc_path  = fasm2_root / "include" / "pcount" / f"{args.dll}.inc"
        e32_path = fasm2_root / "include" / "equates" / f"{args.dll}32.inc"
        e64_path = fasm2_root / "include" / "equates" / f"{args.dll}64.inc"

        for path, content in [
            (api_path, api_txt), (pc_path, pcount_txt),
            (e32_path, eq32_txt), (e64_path, eq64_txt),
        ]:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8")
            print(f"  Written: {path}")
    else:
        print("\n" + "=" * 60)
        print("api/ stub (--write to save):")
        print(api_txt[:2000])
        print("...")
        print("=" * 60)
        print("pcount/ stub:")
        print(pcount_txt[:1000])


if __name__ == "__main__":
    main()
