#!/usr/bin/env python3
"""
Ask the local Windows SDK for one C structure and print fasm2-ready output.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

import psdk_common as psdk
import win32_scraper as win32


def discover_headers(ctx: psdk.PSDKContext, name: str, args) -> list[str]:
    if args.header:
        return psdk.resolve_headers(ctx, args.header)
    if args.dll:
        headers = win32.DLL_HEADERS.get(args.dll)
        if not headers:
            raise SystemExit(f"unknown DLL header map: {args.dll}")
        return psdk.resolve_headers(ctx, headers)

    hits = psdk.find_sdk_sources(
        ctx,
        name,
        exts=(".h",),
        max_hits=12,
        pattern=rf"\b{re.escape(name)}\b",
    )
    for hit in hits:
        header = psdk.sdk_file_to_include(ctx, hit.path)
        headers = psdk.resolve_headers(ctx, [header])
        if psdk.resolve_record(ctx, name, headers, arch=64):
            return headers
    raise SystemExit(f"could not find a SDK header defining {name}")


def render_for_arches(ctx: psdk.PSDKContext, name: str, headers: list[str], arches: list[int]) -> str:
    out: list[str] = [
        f"; PSDK struct inquiry: {name}",
        f"; {psdk.sdk_summary(ctx)}",
        f"; headers: {', '.join(headers)}",
        "",
    ]

    root_info = psdk.resolve_record(ctx, name, headers, arch=64)
    if not root_info:
        raise SystemExit(f"clang could not resolve struct {name} from {', '.join(headers)}")

    deps = psdk.collect_dependencies(ctx, [name], headers, arch=64)
    deps.structs.setdefault(name, root_info)
    known = psdk.known_type_map(deps)

    if deps.enums:
        for enum in deps.enums.values():
            out.append(psdk.render_enum(enum))
            out.append("")

    for arch in arches:
        out.append(f"; ---- {arch}-bit ----")
        for struct_name, info in deps.structs.items():
            layout = psdk.struct_layout(ctx, struct_name, headers, arch)
            out.append(psdk.render_struct(info, known, layout, arch))
            out.append("")

    if deps.unknown:
        out.append("; unresolved dependent types:")
        for unknown in sorted(deps.unknown):
            out.append(f";   {unknown}")
        out.append("")

    return "\n".join(out).rstrip() + "\n"


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Generate a fasm2 structure snippet from the local Windows SDK")
    ap.add_argument("name", help="SDK typedef name, for example TASKDIALOGCONFIG")
    ap.add_argument("--dll", help="Use the header set mapped to this DLL")
    ap.add_argument("--header", action="append", help="SDK header to include, repeatable")
    ap.add_argument("--arch", choices=("32", "64", "both"), default="both")
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
    headers = discover_headers(ctx, args.name, args)
    arches = [32, 64] if args.arch == "both" else [int(args.arch)]
    text = render_for_arches(ctx, args.name, headers, arches)

    if args.append_to:
        print(psdk.append_or_preview(args.append_to, text, args.write))
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
