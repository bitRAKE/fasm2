#!/usr/bin/env python3
"""
Ask the local Windows SDK for one API function and print import, pcount, and
dependent fasm2 type snippets.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import psdk_common as psdk


def function_prototype(info: dict) -> str:
    params = info.get("params", [])
    if not params:
        params_text = "void"
    else:
        rendered = []
        for param in params:
            name = param.get("name") or "param"
            ctype = param.get("type", "")
            rendered.append(f"{ctype} {name}".strip())
        params_text = ", ".join(rendered)
    ret_type = info.get("return_type", "")
    if "(" in ret_type:
        ret_type = ret_type.split("(", 1)[0].strip()
    return f"{ret_type} {info.get('name', '')}({params_text})"


def render_report(ctx: psdk.PSDKContext, inquiry: psdk.FunctionInquiry, arch: int) -> str:
    info = inquiry.info
    params = info.get("params", [])
    deps = psdk.collect_function_dependencies(ctx, inquiry, arch=arch)
    known = psdk.known_type_map(deps)

    lines: list[str] = [
        f"; PSDK function inquiry: {inquiry.requested_name}",
        f"; resolved: {inquiry.name}",
        f"; {psdk.sdk_summary(ctx)}",
        f"; headers: {', '.join(inquiry.headers)}",
    ]
    if inquiry.dll_name:
        lines.append(f"; import library: {inquiry.dll_name}.lib")
        lines.append(f"; DLL: {inquiry.dll_name}.dll")
    if inquiry.exported is not None:
        lines.append(f"; SDK import library export: {'yes' if inquiry.exported else 'no'}")
    if info.get("file"):
        lines.append(f"; declaration: {info['file']}")
    lines.extend(["", f"; C prototype: {function_prototype(info)}", ""])

    if inquiry.dll_name:
        lines.extend([
            "; fasm2 import/pcount additions",
            f"import {inquiry.dll_name},\\",
            f"       {inquiry.name},'{inquiry.name}'",
            f"{inquiry.name}% = {len(params):2d}",
        ])
        stem = psdk.aw_stem(inquiry.name)
        if stem:
            lines.append(f"; generic API alias candidate: {stem}")
        lines.append("")

    if deps.structs or deps.enums:
        lines.append(f"; dependent types ({arch}-bit layout)")
        for enum in deps.enums.values():
            lines.append(psdk.render_enum(enum))
            lines.append("")
        for name, struct in deps.structs.items():
            layout = psdk.struct_layout(ctx, name, inquiry.headers, arch)
            lines.append(psdk.render_struct(struct, known, layout, arch))
            lines.append("")

    if deps.unknown:
        lines.append("; unresolved dependent types:")
        for unknown in sorted(deps.unknown):
            lines.append(f";   {unknown}")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Generate a fasm2 API-use snippet from the local Windows SDK")
    ap.add_argument("name", help="API function, for example TaskDialogIndirect")
    ap.add_argument("--dll", help="Search only the header set mapped to this DLL")
    ap.add_argument("--header", action="append", help="SDK header to include, repeatable")
    ap.add_argument("--arch", type=int, choices=(32, 64), default=64)
    ap.add_argument("--append-to", type=Path,
                    help="Project-local include file to append to")
    ap.add_argument("--write", action="store_true",
                    help="Actually write --append-to; otherwise preview")
    ap.add_argument("--fasm2-root", default=None)
    ap.add_argument("--llvm", default=None)
    ap.add_argument("--sdk", default=None)
    ap.add_argument("--sdk-ver", default=None)
    args = ap.parse_args(argv)

    ctx = psdk.make_context(args)
    inquiry = psdk.discover_function(
        ctx,
        args.name,
        dll_name=args.dll,
        headers=args.header,
        arch=args.arch,
    )
    if not inquiry:
        raise SystemExit(f"could not resolve API function {args.name}")

    text = render_report(ctx, inquiry, args.arch)
    if args.append_to:
        print(psdk.append_or_preview(args.append_to, text, args.write))
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
