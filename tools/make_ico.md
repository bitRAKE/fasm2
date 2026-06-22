Below is a practical compendium for creating ICOs in 2026-era workflows. Treat `.ico` as a **container**, not a single image.

## 1. ICO fundamentals

An ICO file starts with an `ICONDIR` header, followed by one `ICONDIRENTRY` per image. Each entry stores width, height, bit depth, payload size, and file offset; width or height byte `0` means `256`. The payload is usually a DIB/BMP-style bitmap or a PNG stream. ([Microsoft for Developers][1])

When compiled into a Windows executable, the `.ico` is not stored as one blob. The resource compiler splits it into one `RT_GROUP_ICON` directory resource plus one `RT_ICON` resource for each image. Since Windows Vista, icon resources may contain PNG-compressed image data. ([Microsoft for Developers][2])

For modern Windows shell use, Microsoft’s current minimum recommended Win32 set is:

```text
16×16, 24×24, 32×32, 48×48, 256×256
```

Windows prefers an exact-size match; otherwise it chooses the next larger image and scales down. That makes `24×24`, `40×40`, `60×60`, `72×72`, and `96×96` more relevant on high-DPI Windows than older `16/32/48/256`-only recipes suggest. ([Microsoft Learn][3])

A conservative high-quality Windows application set is:

```text
16, 20, 24, 30, 32, 40, 48, 60, 64, 72, 96, 128, 256
```

A compact but still modern set is:

```text
16, 24, 32, 48, 256
```

Older Microsoft Win32 icon guidance still centers on `16×16`, `32×32`, `48×48`, and `256×256`, with `24×24`, `64×64`, `96×96`, and `128×128` useful for additional contexts. ([Microsoft Learn][4])

## 2. Compression profiles

Use these profiles depending on target:

| Profile                     |                                 Payloads | Best for                                  |
| --------------------------- | ---------------------------------------: | ----------------------------------------- |
| **Classic-compatible**      | Small sizes as DIB/BMP, `256×256` as PNG | Windows desktop apps, broad compatibility |
| **All-PNG ICO**             |               Every entry PNG-compressed | Small files, modern Windows/web use       |
| **All-BMP ICO**             |                      Every entry DIB/BMP | Old tools/readers, but large files        |
| **Hand-tuned multi-source** |      Each size drawn/exported separately | Best visual quality                       |

FFmpeg’s ICO muxer is strict: dimensions must be at most `256×256`; images must be BMP/DIB or PNG; BMP entries support formats such as `bgra`, and PNG entries must be `rgba`. ([FFmpeg][5])

PNG-compressed ICO entries are normal in modern Windows, but not every old or non-Windows program reads them. GIMP’s ICO export dialog still flags PNG compression as potentially unsupported by some programs. ([GIMP Documentation][6])

## 3. FFmpeg recipes

### 3.1 Minimal single-image ICO

This creates a one-entry `256×256` ICO:

```bat
ffmpeg -y -i input.png ^
  -vf "scale=256:256:flags=lanczos,format=rgba" ^
  -c:v png ^
  output.ico
```

This is acceptable for quick internal tools, but it is not ideal for a polished app because Windows must downscale it for every smaller context.

### 3.2 Multi-size all-PNG ICO

```bat
ffmpeg -y -i input.png -filter_complex ^
"[0:v]scale=16:16:flags=lanczos,format=rgba[ico16];^
 [0:v]scale=24:24:flags=lanczos,format=rgba[ico24];^
 [0:v]scale=32:32:flags=lanczos,format=rgba[ico32];^
 [0:v]scale=48:48:flags=lanczos,format=rgba[ico48];^
 [0:v]scale=64:64:flags=lanczos,format=rgba[ico64];^
 [0:v]scale=96:96:flags=lanczos,format=rgba[ico96];^
 [0:v]scale=128:128:flags=lanczos,format=rgba[ico128];^
 [0:v]scale=256:256:flags=lanczos,format=rgba[ico256]" ^
  -map "[ico16]" ^
  -map "[ico24]" ^
  -map "[ico32]" ^
  -map "[ico48]" ^
  -map "[ico64]" ^
  -map "[ico96]" ^
  -map "[ico128]" ^
  -map "[ico256]" ^
  -c:v png ^
  -pix_fmt rgba ^
  output.ico
```

Use this for web favicons, modern-only apps, or where file size matters more than legacy compatibility.

### 3.3 Mixed classic/modern ICO

Small entries as BMP/DIB, large entry as PNG:

```bat
ffmpeg -y -i input.png -filter_complex ^
"[0:v]scale=16:16:flags=lanczos,format=bgra[ico16];^
 [0:v]scale=24:24:flags=lanczos,format=bgra[ico24];^
 [0:v]scale=32:32:flags=lanczos,format=bgra[ico32];^
 [0:v]scale=48:48:flags=lanczos,format=bgra[ico48];^
 [0:v]scale=256:256:flags=lanczos,format=rgba[ico256]" ^
  -map "[ico16]" ^
  -map "[ico24]" ^
  -map "[ico32]" ^
  -map "[ico48]" ^
  -map "[ico256]" ^
  -c:v:0 bmp ^
  -c:v:1 bmp ^
  -c:v:2 bmp ^
  -c:v:3 bmp ^
  -c:v:4 png ^
  output.ico
```

This is a good default for Windows executables.

### 3.4 Hand-tuned size layers

Best result: draw or export each size separately, then pack them.

```bat
ffmpeg -y ^
  -i icon_16.png ^
  -i icon_24.png ^
  -i icon_32.png ^
  -i icon_48.png ^
  -i icon_256.png ^
  -map 0:v ^
  -map 1:v ^
  -map 2:v ^
  -map 3:v ^
  -map 4:v ^
  -c:v png ^
  -pix_fmt rgba ^
  output.ico
```

For mixed compatibility:

```bat
ffmpeg -y ^
  -i icon_16.png ^
  -i icon_24.png ^
  -i icon_32.png ^
  -i icon_48.png ^
  -i icon_256.png ^
  -map 0:v -map 1:v -map 2:v -map 3:v -map 4:v ^
  -c:v:0 bmp ^
  -c:v:1 bmp ^
  -c:v:2 bmp ^
  -c:v:3 bmp ^
  -c:v:4 png ^
  output.ico
```

### 3.5 Preserve aspect ratio on a transparent square canvas

For a non-square logo:

```bat
ffmpeg -y -i logo.png ^
  -vf "scale=256:256:force_original_aspect_ratio=decrease:flags=lanczos,pad=256:256:(ow-iw)/2:(oh-ih)/2:color=black@0,format=rgba" ^
  -c:v png ^
  logo_256.ico
```

Use this as preprocessing, then generate the full size set.

### 3.6 Pixel art / hard-edge icons

For pixel art, use nearest-neighbor scaling:

```bat
ffmpeg -y -i pixel_icon.png ^
  -vf "scale=256:256:flags=neighbor,format=rgba" ^
  -c:v png ^
  pixel_icon.ico
```

For normal vector or raster art, `lanczos` is usually a better first choice.

## 4. ImageMagick recipes

ImageMagick is often the simplest free CLI tool for ICO creation. It supports ICO read/write, and its ICO writer has `icon:auto-resize` for generating multiple entries from a `256×256` source. ([ImageMagick][7])

### 4.1 One-command multi-size ICO

```bat
magick input.png ^
  -background none ^
  -resize 256x256 ^
  -gravity center ^
  -extent 256x256 ^
  -define icon:auto-resize=256,128,96,64,48,32,24,16 ^
  output.ico
```

This is the best “quick production” command when the source image is high quality.

### 4.2 Compact Windows set

```bat
magick input.png ^
  -background none ^
  -resize 256x256 ^
  -gravity center ^
  -extent 256x256 ^
  -define icon:auto-resize=256,48,32,24,16 ^
  app.ico
```

### 4.3 Control PNG compression threshold

ImageMagick exposes `icon:png-compression-size` to control when ICO entries are stored as PNG rather than BMP. ([ImageMagick][8])

```bat
magick input.png ^
  -background none ^
  -resize 256x256 ^
  -gravity center ^
  -extent 256x256 ^
  -define icon:auto-resize=256,128,96,64,48,32,24,16 ^
  -define icon:png-compression-size=65536 ^
  output.ico
```

Use this when you want smaller entries left as BMP/DIB and larger entries PNG-compressed.

## 5. SVG-first workflow

Design the master icon as SVG when possible. Export raster sizes, inspect the small sizes, then pack the ICO.

Inkscape’s current CLI export model uses options such as `--export-type`, `--export-filename`, `--export-width`, and `--export-height`. ([Inkscape Wiki][9])

### Windows CMD

```bat
for %s in (16 20 24 30 32 40 48 60 64 72 96 128 256) do ^
  inkscape icon.svg --export-type=png --export-filename=icon_%s.png --export-width=%s --export-height=%s
```

Inside a `.bat` file, double the `%`:

```bat
for %%s in (16 20 24 30 32 40 48 60 64 72 96 128 256) do ^
  inkscape icon.svg --export-type=png --export-filename=icon_%%s.png --export-width=%%s --export-height=%%s
```

Then pack:

```bat
ffmpeg -y ^
  -i icon_16.png -i icon_20.png -i icon_24.png -i icon_30.png ^
  -i icon_32.png -i icon_40.png -i icon_48.png -i icon_60.png ^
  -i icon_64.png -i icon_72.png -i icon_96.png -i icon_128.png -i icon_256.png ^
  -map 0:v -map 1:v -map 2:v -map 3:v ^
  -map 4:v -map 5:v -map 6:v -map 7:v ^
  -map 8:v -map 9:v -map 10:v -map 11:v -map 12:v ^
  -c:v png -pix_fmt rgba ^
  app.ico
```

For production-quality small icons, do not blindly trust the exported `16×16`, `20×20`, and `24×24` versions. Hand-tune them.

## 6. Pillow automation

Pillow’s ICO writer supports `sizes`, defaults to common sizes up to `256×256`, and can save ICO entries as PNG by default or BMP with `bitmap_format="bmp"`. ([Pillow (PIL Fork)][10])

### 6.1 Generate sizes from one image

```python
from PIL import Image

sizes = [
    (16, 16), (24, 24), (32, 32), (48, 48),
    (64, 64), (96, 96), (128, 128), (256, 256),
]

img = Image.open("input.png").convert("RGBA")
img.save("app.ico", sizes=sizes)
```

### 6.2 Use hand-tuned layers

```python
from PIL import Image

sizes = [16, 24, 32, 48, 256]
images = [Image.open(f"icon_{s}.png").convert("RGBA") for s in sizes]

largest = images[-1]
smaller = images[:-1]

largest.save(
    "app.ico",
    sizes=[im.size for im in images],
    append_images=smaller,
)
```

### 6.3 Force BMP-style entries

```python
from PIL import Image

img = Image.open("input.png").convert("RGBA")
img.save(
    "app_bmp.ico",
    sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (256, 256)],
    bitmap_format="bmp",
)
```

Pillow is useful when ICO generation is part of a build script or test pipeline.

## 7. icoutils

`icoutils` is a POSIX-oriented toolkit for Windows icon/cursor files. `icotool` can create ICO/CUR files from PNGs and extract ICO images to PNG; `wrestool` can extract resources from PE/NE executables and DLLs. ([Nongnu][11])

Create:

```sh
icotool -c -o app.ico icon_16.png icon_24.png icon_32.png icon_48.png icon_256.png
```

List:

```sh
icotool -l app.ico
```

Extract:

```sh
icotool -x -o extracted app.ico
```

Extract icons from a Windows executable or DLL:

```sh
wrestool -l app.exe
wrestool -x -t 14 -o extracted app.exe
```

This is useful in CI, reverse inspection, packaging checks, and cross-platform build systems.

## 8. GUI tools

| Tool                          | Use case                  | Notes                                                                                                                                                      |
| ----------------------------- | ------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **GIMP**                      | Free GUI editing/export   | ICO export supports multiple icon sizes and lets you choose 1/4/8-bit indexed, 24 bpp, 32 bpp, or PNG-compressed entries. ([GIMP Documentation][6])        |
| **Greenfish Icon Editor Pro** | Icon-specific manual work | Freeware editor for icons, cursors, animations, and icon libraries; current releases include Windows portable/installer builds. ([Greenfish Software][12]) |
| **Inkscape**                  | SVG master artwork        | Best for vector-first workflows, then raster export per size. ([Inkscape Wiki][9])                                                                         |
| **ImageMagick**               | Fast CLI conversion       | Strong default for auto-resized multi-size ICOs. ([ImageMagick][13])                                                                                       |
| **FFmpeg**                    | Scripted pipelines        | Useful when your build already uses FFmpeg or you want explicit stream mapping/codecs. ([FFmpeg][5])                                                       |

Greenfish’s own documentation makes the same practical point as Microsoft: an icon is a set of alternate images, and small sizes often need their own simplified artwork rather than naive downscaling. ([Greenfish Software][14])

## 9. Visual design techniques

### Size-specific drawing

At `16×16`, an icon is not a miniature illustration. It is a symbol. Typical adjustments:

```text
256×256: full detail, gradients, soft shadow acceptable
128×128: still detailed, reduce fine texture
64×64: simplify internal lines
48×48: remove small text and weak details
32×32: emphasize silhouette
24×24: thicken strokes, simplify shapes
16×16: pixel-grid design, high contrast, no decorative noise
```

### Alpha handling

Use straight alpha RGBA source images. Avoid dark or light fringes by extending edge colors into transparent pixels before resizing. This is sometimes called alpha bleeding or color bleeding.

For example, when a red object fades to transparent, the fully transparent pixels around it should still carry red-ish RGB values, not black. Some renderers and resamplers reveal that hidden RGB during filtering.

### Scaling filters

General icon downscaling:

```text
lanczos        good default for photographic/vector-rendered art
bicubic        softer, sometimes better for simple shapes
neighbor       pixel art only
area           useful for strong downscaling in some pipelines
```

For tiny sizes, filter choice matters less than hand correction.

### Pixel alignment

For small sizes:

```text
1 px strokes should land on full pixels
avoid half-pixel vertical/horizontal edges
prefer filled silhouettes over outlines
avoid small internal holes
remove text below 32×32 unless it is a single bold glyph
```

### Backgrounds

Prefer transparent icons for Windows app resources. For favicons, transparency is fine, but test against light/dark browser UI.

## 10. Validation and inspection

### 10.1 Minimal Python ICO directory dump

Useful for build tests:

```python
import struct
import sys

PNG_MAGIC = b"\x89PNG\r\n\x1a\n"

def dump_ico(path: str) -> None:
    data = open(path, "rb").read()

    if len(data) < 6:
        raise ValueError("Too small for ICO header")

    reserved, icon_type, count = struct.unpack_from("<HHH", data, 0)

    if reserved != 0 or icon_type != 1:
        raise ValueError(f"Not an ICO file: reserved={reserved}, type={icon_type}")

    print(f"{path}: {count} image(s)")

    for i in range(count):
        off = 6 + i * 16
        (
            b_width,
            b_height,
            color_count,
            b_reserved,
            planes,
            bit_count,
            size,
            image_offset,
        ) = struct.unpack_from("<BBBBHHII", data, off)

        width = 256 if b_width == 0 else b_width
        height = 256 if b_height == 0 else b_height

        payload = data[image_offset:image_offset + min(size, 8)]
        kind = "PNG" if payload.startswith(PNG_MAGIC) else "DIB/BMP"

        print(
            f"{i:02d}: {width}x{height}, "
            f"{bit_count} bpp, {kind}, "
            f"{size} bytes @ {image_offset}"
        )

if __name__ == "__main__":
    dump_ico(sys.argv[1])
```

Example:

```bat
python dump_ico.py app.ico
```

### 10.2 Other inspection commands

```bat
magick identify app.ico
```

```bat
ffprobe -hide_banner -show_streams app.ico
```

```sh
icotool -l app.ico
```

Also test visually in:

```text
Explorer details/list/large icon views
taskbar
Alt-Tab
Start menu
window title bar
file Properties dialog
high-DPI monitor
dark and light themes
```

Windows icon caching can show stale results. Rename the EXE or clear the icon cache when testing repeated builds.

## 11. Embedding ICOs into Windows executables

### 11.1 Resource script

A basic `.rc` file:

```rc
#define IDI_APP 1

IDI_APP ICON "app.ico"
```

Microsoft’s resource syntax for icons is `nameID ICON filename`; the icon file may contain multiple images. ([Microsoft Learn][15])

### 11.2 MinGW / GNU windres

`windres` converts `.rc` to `.res` or COFF object form for linking. ([Sourceware][16])

```bat
windres app.rc -O coff -o app.res.o
gcc main.o app.res.o -o app.exe
```

Cross-compile example:

```sh
x86_64-w64-mingw32-windres app.rc -O coff -o app.res.o
x86_64-w64-mingw32-gcc main.o app.res.o -o app.exe
```

### 11.3 MSVC tools

```bat
rc /nologo app.rc
link main.obj app.res user32.lib kernel32.lib /OUT:app.exe
```

### 11.4 Patching an existing EXE

Resource Hacker is a free Windows resource editor/compiler/decompiler with GUI and command-line modes. It can add, overwrite, extract, delete, and modify resources. ([AngusJ][17])

Typical command shape:

```bat
ResourceHacker.exe ^
  -open app.exe ^
  -save app_patched.exe ^
  -action addoverwrite ^
  -resource app.ico ^
  -mask ICONGROUP,1,
```

`rcedit` is a command-line tool for editing Windows executable resources and supports `--set-icon`. ([GitHub][18])

```bat
rcedit app.exe --set-icon app.ico
```

Modifying resources invalidates an Authenticode signature. Sign after patching, not before.

### 11.5 Assembly-oriented note

For fasm/fasmg-style PE generation, the important distinction is:

```text
.ico file:
  ICONDIR
  ICONDIRENTRY[]
  raw image payloads

.exe .rsrc:
  RT_GROUP_ICON
  RT_ICON #1
  RT_ICON #2
  RT_ICON #3
  ...
```

The group resource resembles the ICO directory, but its entries point to resource IDs, not file offsets. Do not embed the entire `.ico` as `RCDATA` and expect Explorer to use it as the application icon. Use a proper `ICON` resource or emit the corresponding `RT_GROUP_ICON` / `RT_ICON` tree yourself. ([Microsoft for Developers][2])

## 12. Favicons and web use

For web pages, `favicon.ico` is now mainly a compatibility layer. Browsers commonly use favicon images for tabs, bookmarks, and similar UI; traditional favicon sizes are often `16×16`, with PNG, GIF, or ICO support depending on context. ([MDN Web Docs][19])

A practical modern web setup:

```html
<link rel="icon" href="/favicon.ico" sizes="any">
<link rel="icon" href="/favicon.svg" type="image/svg+xml">
<link rel="apple-touch-icon" href="/apple-touch-icon.png">
<link rel="manifest" href="/site.webmanifest">
```

`favicon.ico`:

```text
16×16
32×32
48×48
```

Optional:

```text
64×64
128×128
256×256
```

For Google Search surfaces, Google recommends a square favicon at least `8×8`, preferably larger than `48×48`, with a stable URL. ([Google for Developers][20])

For web app manifests, ICO can list multiple raster sizes, while SVG can use `sizes: "any"`; declaring the image `type` can help the browser choose resources more efficiently. ([MDN Web Docs][21])

Example `site.webmanifest`:

```json
{
  "icons": [
    {
      "src": "/favicon.svg",
      "sizes": "any",
      "type": "image/svg+xml"
    },
    {
      "src": "/icon-192.png",
      "sizes": "192x192",
      "type": "image/png"
    },
    {
      "src": "/icon-512.png",
      "sizes": "512x512",
      "type": "image/png"
    }
  ]
}
```

Do not try to force `512×512` into a conventional ICO pipeline. Use PNG/SVG for those web/PWA sizes.

## 13. Recommended default pipelines

### Fast Windows app icon

```bat
magick input.png ^
  -background none ^
  -resize 256x256 ^
  -gravity center ^
  -extent 256x256 ^
  -define icon:auto-resize=256,48,32,24,16 ^
  app.ico
```

Good for utilities, prototypes, internal tools.

### Polished Windows desktop app

```text
1. Create SVG or large 1024×1024 master.
2. Export 256,128,96,64,48,32,24,16 PNGs.
3. Manually tune 16,24,32.
4. Pack small entries as BMP/DIB and 256 as PNG.
5. Embed as ICON resource, not RCDATA.
6. Test in Explorer, taskbar, Start, Alt-Tab, title bar, high DPI.
```

Packing command:

```bat
ffmpeg -y ^
  -i icon_16.png ^
  -i icon_24.png ^
  -i icon_32.png ^
  -i icon_48.png ^
  -i icon_256.png ^
  -map 0:v -map 1:v -map 2:v -map 3:v -map 4:v ^
  -c:v:0 bmp ^
  -c:v:1 bmp ^
  -c:v:2 bmp ^
  -c:v:3 bmp ^
  -c:v:4 png ^
  app.ico
```

### High-DPI exact-match Windows icon

```text
16, 20, 24, 30, 32, 40, 48, 60, 64, 72, 96, 128, 256
```

Use when pixel precision matters and a larger ICO file is acceptable.

### Web favicon set

```text
/favicon.ico       16,32,48 ICO
/favicon.svg       scalable modern favicon
/apple-touch-icon.png
/icon-192.png
/icon-512.png
/site.webmanifest
```

## 14. Common mistakes

| Mistake                           | Result                 | Fix                                               |
| --------------------------------- | ---------------------- | ------------------------------------------------- |
| Only shipping `256×256`           | Blurry small icons     | Include at least `16,24,32,48,256`                |
| Blindly downscaling to `16×16`    | Muddy symbol           | Hand-tune tiny sizes                              |
| Using non-square input            | Cropping or distortion | Pad to square transparent canvas                  |
| Using all-PNG ICO for old targets | Some readers fail      | Use BMP/DIB small entries, PNG only for large     |
| Embedding ICO as raw data         | Explorer ignores it    | Use `ICON` resource / `RT_GROUP_ICON` + `RT_ICON` |
| Patching signed EXE               | Broken signature       | Patch first, sign last                            |
| Testing with stale Explorer cache | False failure          | Rename file or clear icon cache                   |
| Transparent pixels have black RGB | Dark halos             | Alpha-bleed edge colors before resizing           |

Default recommendation: for Windows desktop, ship `16,24,32,48,256`; make small entries DIB/BMP and `256×256` PNG; hand-tune `16` and `24`. For web, keep `favicon.ico` as a fallback and use SVG/PNG manifest icons for modern surfaces.

[1]: https://devblogs.microsoft.com/oldnewthing/20101018-00/?p=12513 "The evolution of the ICO file format, part 1: Monochrome beginnings - The Old New Thing"
[2]: https://devblogs.microsoft.com/oldnewthing/20120720-00/?p=7083 "The format of icon resources - The Old New Thing"
[3]: https://learn.microsoft.com/en-us/windows/apps/design/iconography/app-icon-construction "Construct your Windows app's icon - Windows apps | Microsoft Learn"
[4]: https://learn.microsoft.com/en-us/windows/win32/uxguide/vis-icons "Icons (Design basics) - Win32 apps | Microsoft Learn"
[5]: https://ffmpeg.org/ffmpeg-formats.html "      FFmpeg Formats Documentation
"
[6]: https://docs.gimp.org/3.2/en/file-ico-export.html "5.29. Export Image as Microsoft Windows Icon"
[7]: https://imagemagick.org/formats/ "ImageMagick | Image Formats"
[8]: https://imagemagick.org/defines/ "ImageMagick | Defines"
[9]: https://wiki.inkscape.org/wiki/Using_the_Command_Line "Using the Command Line - Inkscape Wiki"
[10]: https://pillow.readthedocs.io/en/stable/handbook/image-file-formats.html "Image file formats - Pillow (PIL Fork) 12.2.0 documentation"
[11]: https://www.nongnu.org/icoutils/ "icoutils home"
[12]: https://greenfishsoftware.org/gfie.php "Greenfish Icon Editor Pro 4.5 - Official Website"
[13]: https://imagemagick.org/ "ImageMagick | Mastering Digital Image Alchemy"
[14]: https://greenfishsoftware.org/help/gfie/English/win_icon.html "Windows icons - Greenfish Icon Editor Pro Help"
[15]: https://learn.microsoft.com/en-us/windows/win32/menurc/icon-resource "ICON resource - Win32 apps | Microsoft Learn"
[16]: https://sourceware.org/binutils/docs/binutils/windres.html "windres (GNU Binary Utilities)"
[17]: https://angusj.com/resourcehacker/ "Resource Hacker"
[18]: https://github.com/electron/rcedit "GitHub - electron/rcedit: Command line tool to edit resources of exe · GitHub"
[19]: https://developer.mozilla.org/en-US/docs/Glossary/Favicon "Favicon - Glossary | MDN"
[20]: https://developers.google.com/search/docs/appearance/favicon-in-search "Define Website Favicon for Search Results | Google Search Central  |  Documentation  |  Google for Developers"
[21]: https://developer.mozilla.org/en-US/docs/Web/Progressive_web_apps/Manifest/Reference/icons "icons - Web app manifest | MDN"
