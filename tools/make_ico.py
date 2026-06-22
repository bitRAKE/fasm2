#!/usr/bin/env python3
"""
Create a multi-resolution Windows .ico file from one source image.

Example:
    python make_ico.py note_a.png note_a.ico

Requires:
    pip install pillow
"""

from __future__ import annotations

import argparse
import io
import struct
from pathlib import Path
from PIL import Image


DEFAULT_SIZES = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]


def fit_icon_canvas(img: Image.Image, size: int) -> Image.Image:
    """
    Resize source image proportionally into a square transparent canvas.
    Keeps the whole icon visible instead of cropping.
    """
    img = img.convert("RGBA")

    # Leave a small margin so shadows and rounded edges survive at small sizes.
    margin = max(0, size // 32)
    target = size - margin * 2

    img.thumbnail((target, target), Image.Resampling.LANCZOS)

    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    x = (size - img.width) // 2
    y = (size - img.height) // 2
    canvas.alpha_composite(img, (x, y))

    return canvas


def make_png_entry(img: Image.Image) -> bytes:
    """
    Encode one icon image as PNG bytes for modern ICO storage.
    """
    buffer = io.BytesIO()
    img.save(buffer, format="PNG", optimize=True)
    return buffer.getvalue()


def write_ico(source: Path, output: Path, sizes: list[int], export_pngs: bool = False) -> None:
    src_img = Image.open(source).convert("RGBA")

    entries: list[tuple[int, bytes]] = []

    for size in sizes:
        icon_img = fit_icon_canvas(src_img, size)
        png_data = make_png_entry(icon_img)
        entries.append((size, png_data))

        if export_pngs:
            png_path = output.with_name(f"{output.stem}_{size}x{size}.png")
            icon_img.save(png_path)

    # ICO header:
    # Reserved: 0
    # Type: 1 for icons
    # Count: number of images
    ico_header = struct.pack("<HHH", 0, 1, len(entries))

    directory = bytearray()
    image_data = bytearray()

    # Directory starts after 6-byte header plus 16 bytes per entry.
    offset = 6 + 16 * len(entries)

    for size, png_data in entries:
        width_byte = 0 if size == 256 else size
        height_byte = 0 if size == 256 else size

        # ICONDIRENTRY layout:
        # BYTE  width
        # BYTE  height
        # BYTE  color_count
        # BYTE  reserved
        # WORD  color_planes
        # WORD  bits_per_pixel
        # DWORD image_size
        # DWORD image_offset
        directory.extend(
            struct.pack(
                "<BBBBHHII",
                width_byte,
                height_byte,
                0,          # color count: 0 means >= 256 colors
                0,          # reserved
                1,          # color planes
                32,         # bits per pixel
                len(png_data),
                offset,
            )
        )

        image_data.extend(png_data)
        offset += len(png_data)

    output.write_bytes(ico_header + directory + image_data)


def parse_sizes(value: str) -> list[int]:
    sizes = []
    for part in value.split(","):
        size = int(part.strip())
        if size < 1 or size > 256:
            raise argparse.ArgumentTypeError("ICO sizes must be from 1 to 256.")
        sizes.append(size)

    return sorted(set(sizes))


def main() -> None:
    parser = argparse.ArgumentParser(description="Create a multi-resolution Windows ICO file.")
    parser.add_argument("source", type=Path, help="Input image, preferably a large transparent PNG.")
    parser.add_argument("output", type=Path, help="Output .ico file.")
    parser.add_argument(
        "--sizes",
        type=parse_sizes,
        default=DEFAULT_SIZES,
        help="Comma-separated icon sizes. Default: 16,20,24,32,40,48,64,96,128,256",
    )
    parser.add_argument(
        "--export-pngs",
        action="store_true",
        help="Also save each resized PNG next to the ICO.",
    )

    args = parser.parse_args()

    write_ico(
        source=args.source,
        output=args.output,
        sizes=args.sizes,
        export_pngs=args.export_pngs,
    )

    print(f"Created {args.output}")
    print(f"Sizes: {', '.join(str(s) + 'x' + str(s) for s in args.sizes)}")


if __name__ == "__main__":
    main()
