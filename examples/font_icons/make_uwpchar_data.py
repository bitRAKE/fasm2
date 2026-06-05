#!/usr/bin/env python3
"""
Generate uwpchar_data.inc from Microsoft's published Segoe icon font tables.

The installed Segoe icon fonts expose glyph coverage and metrics, but not stable
semantic glyph names. This script gathers the public Microsoft Docs markdown
tables for Segoe MDL2 Assets and Segoe Fluent Icons and emits the compact fasm2
include consumed by uwpchar.asm.
"""

from __future__ import annotations

import argparse
import dataclasses
import re
import sys
import urllib.request
from pathlib import Path
from typing import Iterable


DEFAULT_SOURCES = {
    "mdl2": "https://raw.githubusercontent.com/MicrosoftDocs/windows-dev-docs/docs/"
    "hub/apps/design/iconography/segoe-ui-symbol-font.md",
    "fluent": "https://raw.githubusercontent.com/MicrosoftDocs/windows-dev-docs/docs/"
    "hub/apps/design/iconography/segoe-fluent-icons-font.md",
}

ROW_RE = re.compile(r"^\s*\|(.+)\|\s*$")
NAME_RE = re.compile(r':::no-loc\s+text="([^"]+)":::')
HEX_RE = re.compile(r"\b[0-9a-fA-F]{4,6}\b")
ASCII_NAME_RE = re.compile(r"^[ -~]+$")

VETTED_NAME_OVERRIDES = {
    0xEB0F: "StockDown",
    0xEB11: "StockUp",
}


@dataclasses.dataclass(frozen=True)
class IconName:
    code: int
    name: str


@dataclasses.dataclass(frozen=True)
class MergeStats:
    common_codepoints: int
    exact_matches: int
    conflicts: tuple[tuple[int, str, str], ...]


def read_text(source: str) -> str:
    if re.match(r"^https?://", source, re.IGNORECASE):
        with urllib.request.urlopen(source, timeout=60) as response:
            return response.read().decode("utf-8", errors="replace")
    return Path(source).read_text(encoding="utf-8", errors="replace")


def parse_docs_markdown(text: str) -> list[IconName]:
    items: list[IconName] = []
    seen: set[int] = set()

    for line in text.splitlines():
        row = ROW_RE.match(line)
        if not row:
            continue

        cols = [col.strip() for col in row.group(1).split("|")]
        if len(cols) < 3:
            continue
        if cols[0].lower().startswith("glyph"):
            continue

        code_match = HEX_RE.search(cols[1])
        name_match = NAME_RE.search(cols[2])
        if not code_match or not name_match:
            continue

        code = int(code_match.group(0), 16)
        name = name_match.group(1).strip()
        if not name or code in seen:
            continue
        if not ASCII_NAME_RE.match(name):
            raise ValueError(f"non-ASCII or control character in name for U+{code:04X}: {name!r}")

        seen.add(code)
        items.append(IconName(code, name))

    items.sort(key=lambda item: item.code)
    return items


def merge_names(mdl2: list[IconName], fluent: list[IconName]) -> tuple[list[IconName], MergeStats]:
    mdl2_by_code = {item.code: item.name for item in mdl2}
    fluent_by_code = {item.code: item.name for item in fluent}
    common_codes = set(mdl2_by_code) & set(fluent_by_code)
    conflicts: list[tuple[int, str, str]] = []
    merged: list[IconName] = []

    for code in sorted(set(mdl2_by_code) | set(fluent_by_code)):
        mdl2_name = mdl2_by_code.get(code)
        fluent_name = fluent_by_code.get(code)
        override_name = VETTED_NAME_OVERRIDES.get(code)
        if mdl2_name is not None and fluent_name is not None and mdl2_name != fluent_name:
            conflicts.append((code, mdl2_name, fluent_name))
        if override_name is not None:
            if override_name not in (mdl2_name, fluent_name):
                raise ValueError(f"vetted name override for U+{code:04X} no longer matches input data")
            name = override_name
        elif fluent_name is None:
            name = mdl2_name
        else:
            name = fluent_name
        if name is None:
            raise ValueError(f"internal merge error for U+{code:04X}")
        merged.append(IconName(code, name))

    stats = MergeStats(
        common_codepoints=len(common_codes),
        exact_matches=sum(1 for code in common_codes if mdl2_by_code[code] == fluent_by_code[code]),
        conflicts=tuple(conflicts),
    )
    return merged, stats


def count_pairs(items: list[IconName]) -> int:
    by_name = {item.name: item for item in items}
    pairs: set[tuple[int, int]] = set()

    for item in items:
        for suffix in ("Fill", "Filled", "Solid"):
            if not item.name.endswith(suffix):
                continue

            root = item.name[: -len(suffix)]
            base = by_name.get(root) or by_name.get(root + "Outline")
            if base:
                pairs.add((base.code, item.code))
            break

    return len(pairs)


def escape_fasm_ascii(name: str) -> str:
    raw = name.encode("ascii")
    if b"'" not in raw:
        return f"'{name}'"
    if b'"' not in raw:
        return f'"{name}"'
    return ",".join(str(byte) for byte in raw)


def emit_bitfield(items: list[IconName], code_min: int, code_range: int) -> list[str]:
    words = [0] * ((code_range + 31) // 32)
    for item in items:
        bit = item.code - code_min
        words[bit >> 5] |= 1 << (bit & 31)

    lines: list[str] = []
    for index in range(0, len(words), 4):
        chunk = words[index : index + 4]
        values = ",".join(f"0{word:08X}h" for word in chunk)
        lines.append(f"    dd {values}")
    return lines


def build_name_blocks(
    items: list[IconName],
    code_min: int,
    code_range: int,
    labels_by_code: dict[int, str],
    block_size: int,
) -> tuple[int, list[tuple[int, list[str]]]]:
    by_code = {item.code for item in items}
    block_count = (code_range + block_size - 1) // block_size
    blocks: list[tuple[int, list[str]]] = []
    for block in range(block_count):
        values: list[str] = []
        used = False
        for slot in range(block_size):
            code = code_min + block * block_size + slot
            if code < code_min + code_range and code in by_code:
                values.append(f"{labels_by_code[code]} - uwpchar_name_strings")
                used = True
            else:
                values.append("uwpchar_name_missing")
        if used:
            blocks.append((block, values))
    return block_count, blocks


def emit_name_block_offsets(block_count: int, blocks: list[tuple[int, list[str]]]) -> list[str]:
    used_blocks = {block for block, _ in blocks}
    values: list[str] = []
    for block in range(block_count):
        if block in used_blocks:
            values.append(f"uwpchar_name_block_{block} - uwpchar_names")
        else:
            values.append("uwpchar_name_missing")
    lines: list[str] = []
    for index in range(0, len(values), 8):
        lines.append(f"    dw {','.join(values[index : index + 8])}")
    return lines


def emit_name_blocks(blocks: list[tuple[int, list[str]]]) -> list[str]:
    lines: list[str] = []
    for block, values in blocks:
        lines.append(f"  uwpchar_name_block_{block}:")
        lines.append(f"    dw {','.join(values)}")
    return lines


def emit_name_index(sorted_items: list[IconName], labels_by_code: dict[int, str]) -> list[str]:
    lines: list[str] = []
    values: list[str] = []
    for item in sorted_items:
        values.append(f"{labels_by_code[item.code]} - uwpchar_name_strings")
    for index in range(0, len(values), 8):
        lines.append(f"    dw {','.join(values[index : index + 8])}")
    return lines


def emit_name(label: str, item: IconName) -> str:
    return f"  {label} dw 0{item.code:04X}h\n    db {escape_fasm_ascii(item.name)},0"


def emit_data(
    names: list[IconName],
    mdl2_count: int,
    fluent_count: int,
    stats: MergeStats,
    sources: dict[str, str],
) -> str:
    code_min = min(item.code for item in names)
    code_max = max(item.code for item in names)
    code_range = code_max - code_min + 1
    name_block_shift = 3
    name_block_size = 1 << name_block_shift
    sorted_names = sorted(names, key=lambda item: item.name)
    labels_by_code = {item.code: f"uwpchar_name_{index}" for index, item in enumerate(sorted_names)}
    name_block_count, name_blocks = build_name_blocks(names, code_min, code_range, labels_by_code, name_block_size)
    string_size = sum(2 + len(item.name.encode("ascii")) + 1 for item in sorted_names)
    if string_size > 0xFFFF:
        raise ValueError(f"name string table is too large for word offsets: {string_size} bytes")

    lines: list[str] = [
        "; uwpchar_data.inc - generated by make_uwpchar_data.py",
        f"; MDL2 source: {sources['mdl2']}",
        f"; Fluent source: {sources['fluent']}",
        "; Names are ASCII byte strings keyed only by codepoint.",
        "; Fluent names are preferred for same-codepoint conflicts except vetted overrides.",
        f"; Vetted overrides={len(VETTED_NAME_OVERRIDES)}.",
        f"; Inputs: MDL2={mdl2_count}, Fluent={fluent_count}, common={stats.common_codepoints},",
        f"; exact-name matches={stats.exact_matches}, conflicts={len(stats.conflicts)}.",
        "; uwpchar_codes is a codepoint bitfield indexed from uwpchar_code_min.",
        "; uwpchar_name_block_offsets indexes 8-codepoint blocks, or 0FFFFh.",
        "; uwpchar_names stores word offsets to name records, or 0FFFFh.",
        "; Each name record stores a word codepoint followed by an ASCII string.",
        "; uwpchar_name_index stores name-record offsets sorted by ASCII name.",
        "; Pairing is derived from names at runtime.",
        "",
        "UWPCHAR_FONT_MDL2 = 0",
        "UWPCHAR_FONT_FLUENT = 1",
        "uwpchar_name_missing = 0FFFFh",
        f"uwpchar_name_count = {len(names)}",
        f"uwpchar_code_min = 0{code_min:04X}h",
        f"uwpchar_code_max = 0{code_max:04X}h",
        f"uwpchar_code_range = {code_range}",
        f"uwpchar_name_block_shift = {name_block_shift}",
        f"uwpchar_name_block_mask = {name_block_size - 1}",
        f"uwpchar_name_block_count = {name_block_count}",
        "uwpchar_name_text_offset = 2",
        "",
        "macro uwpchar_name_data",
        "  align 4",
        "  uwpchar_codes:",
    ]

    lines.extend(emit_bitfield(names, code_min, code_range))
    lines.extend(["", "  align 2", "  uwpchar_name_block_offsets:"])
    lines.extend(emit_name_block_offsets(name_block_count, name_blocks))
    lines.extend(["", "  align 2", "  uwpchar_names:"])
    lines.extend(emit_name_blocks(name_blocks))
    lines.extend(["", "  align 2", "  uwpchar_name_index:"])
    lines.extend(emit_name_index(sorted_names, labels_by_code))
    lines.extend(["", "  align 1", "  uwpchar_name_strings:"])
    lines.extend(emit_name(labels_by_code[item.code], item) for item in sorted_names)
    lines.extend(["", "purge uwpchar_name_data", "end macro", ""])
    return "\n".join(lines)


def parse_source_override(value: str) -> tuple[str, str]:
    if "=" not in value:
        raise argparse.ArgumentTypeError("source override must be KEY=PATH_OR_URL")
    key, source = value.split("=", 1)
    key = key.strip().lower()
    if key not in DEFAULT_SOURCES:
        raise argparse.ArgumentTypeError("source key must be 'mdl2' or 'fluent'")
    if not source:
        raise argparse.ArgumentTypeError("source path/url must not be empty")
    return key, source


def gather(sources: dict[str, str]) -> tuple[list[IconName], list[IconName], list[IconName], MergeStats, int]:
    mdl2_raw = read_text(sources["mdl2"])
    fluent_raw = read_text(sources["fluent"])
    mdl2 = parse_docs_markdown(mdl2_raw)
    fluent = parse_docs_markdown(fluent_raw)
    names, stats = merge_names(mdl2, fluent)
    pair_count = count_pairs(names)
    return mdl2, fluent, names, stats, pair_count


def validate(items: Iterable[IconName], label: str) -> None:
    previous = -1
    for item in items:
        if item.code <= previous:
            raise ValueError(f"{label} table is not strictly sorted at U+{item.code:04X}")
        previous = item.code


def validate_unique_names(items: Iterable[IconName], label: str) -> None:
    seen: dict[str, int] = {}
    for item in items:
        previous = seen.get(item.name)
        if previous is not None:
            raise ValueError(f"{label} duplicate name {item.name!r} at U+{previous:04X} and U+{item.code:04X}")
        seen[item.name] = item.code


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--out",
        default=str(Path(__file__).with_name("uwpchar_data.inc")),
        help="output include path; defaults to uwpchar_data.inc next to this script",
    )
    parser.add_argument(
        "--source",
        action="append",
        type=parse_source_override,
        default=[],
        metavar="KEY=PATH_OR_URL",
        help="override a source, where KEY is mdl2 or fluent",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="do not write; report counts after parsing and pair inference",
    )
    parser.add_argument(
        "--conflicts",
        action="store_true",
        help="print same-codepoint MDL2/Fluent name conflicts",
    )
    args = parser.parse_args(argv)

    sources = dict(DEFAULT_SOURCES)
    for key, source in args.source:
        sources[key] = source

    mdl2, fluent, names, stats, pair_count = gather(sources)
    validate(mdl2, "MDL2")
    validate(fluent, "Fluent")
    validate(names, "merged")
    validate_unique_names(names, "merged")

    if args.check:
        print(f"MDL2 names: {len(mdl2)}")
        print(f"Fluent names: {len(fluent)}")
        print(f"Common codepoints: {stats.common_codepoints}")
        print(f"Exact same name at same codepoint: {stats.exact_matches}")
        print(f"Name conflicts at same codepoint: {len(stats.conflicts)}")
        if args.conflicts:
            for code, mdl2_name, fluent_name in stats.conflicts:
                chosen = VETTED_NAME_OVERRIDES.get(code, fluent_name)
                print(f"U+{code:04X}: MDL2={mdl2_name} | Fluent={fluent_name} | Canonical={chosen}")
        print(f"Merged names: {len(names)}")
        print(f"Merged pairs: {pair_count}")
        return 0

    out_path = Path(args.out)
    out_path.write_text(
        emit_data(names, len(mdl2), len(fluent), stats, sources),
        encoding="utf-8",
        newline="\n",
    )
    print(f"wrote {out_path}")
    print(f"MDL2 names: {len(mdl2)}")
    print(f"Fluent names: {len(fluent)}")
    if args.conflicts:
        print(f"Name conflicts at same codepoint: {len(stats.conflicts)}")
        for code, mdl2_name, fluent_name in stats.conflicts:
            chosen = VETTED_NAME_OVERRIDES.get(code, fluent_name)
            print(f"U+{code:04X}: MDL2={mdl2_name} | Fluent={fluent_name} | Canonical={chosen}")
    print(f"Merged names: {len(names)}")
    print(f"Merged pairs: {pair_count}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
