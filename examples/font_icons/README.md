# Font Icon Helpers

`examples/font_icons` is a small Win64 package for using Segoe icon-font
codepoints from tools such as `uwpchar`.

The reusable include is `font_icons.inc`. It deliberately stays local to this
example while the surface is being tested.

## Package Surface

- `FontIcon_CreateFont`: creates an `HFONT` for an icon face at a pixel height.
- `FontIcon_CreateFontForWindow`: DPI-scales a pixel height for one window and
  creates the icon font.
- `FontIcon_DrawGlyph`: draws one `WCHAR` glyph into an `HDC`/`RECT`.
- `FontIcon_DrawLayeredGlyph`: draws two codepoints into the same rect with
  separate colors for companion glyph pairs.
- `FontIcon_DrawLayeredGlyphArray`: draws a caller-provided array of
  `FONTICON_LAYER {glyph,color}` records into one rect, in array order.
- `FontIcon_SetStaticGlyph`: turns a static control into an icon-like label.
- `FontIcon_PrepareControlColor`: helper for `WM_CTLCOLORSTATIC`.
- `FontIcon_ToolbarCustomDraw`: selects the icon font during toolbar
  `NM_CUSTOMDRAW` while leaving default toolbar painting in charge.
- `FontIcon_ToolbarGlyphCustomDraw`: owner-paints toolbar glyph items from each
  button's `dwData` and applies rest, hover, pressed, active, and disabled
  colors.
- `FontIcon_CreateGlyphCursor`: renders a glyph into a 32bpp DIB and creates a
  color cursor from the synthesized coverage.

## Demo

Build from a Visual Studio developer prompt:

```cmd
examples\font_icons\_build.cmd
```

The build script compiles `font_icons.rc` to `font_icons.res` with `rc.exe`,
then the assembly samples include that resource section through `windows.inc`.
`resource.h` is the shared ID file used by the resource compiler and the fasm2
sources.

Refresh `uwpchar_data.inc` from Microsoft Docs and then build all samples:

```cmd
examples\font_icons\_build.cmd regen
```

Run just the data generator:

```cmd
python examples\font_icons\make_uwpchar_data.py --out examples\font_icons\uwpchar_data.inc
```

Print the same-codepoint MDL2/Fluent name conflicts:

```cmd
python examples\font_icons\make_uwpchar_data.py --check --conflicts
```

Run:

```cmd
examples\font_icons\font_icon_demo.exe
examples\font_icons\uwpchar.exe
examples\font_icons\glyphset.exe
```

`font_icon_demo.exe` shows these supported uses:

- a top-row comctl32 toolbar with owner-painted glyph buttons: Add/rest,
  Save/disabled, Search/pressed, Refresh/active, plus hover backplates during
  interaction;
- a lower-left `STATIC` control that behaves like an icon label;
- direct layered glyph drawing in the center panel on an opaque GDI paint
  surface;
- a glyph-backed cursor selected by `WM_SETCURSOR`.

`uwpchar.exe` has a primary growable dialog window from `font_icons.rc`; the custom
glyph grid remains a registered child window class. It enumerates real glyphs
with `GetGlyphIndicesW`, renders a scrollable grid for `Segoe MDL2 Assets` or
`Segoe Fluent Icons`, and appends fasm constants when cells are clicked. The
`Merged pairs` checkbox collapses
MDL2 fill companions into one two-color cell; clearing it shows the raw font
glyph list. `Show legacy` adds deprecated `E000`-`E5FF` glyphs back into the
grid; they are hidden by default. The filter edit narrows the grid by generated
ASCII name, and merged pairs match either layer name. The default palette is dark
so layered glyph colors are visible without changing settings first.

The `Fore`, `Back`, and `Pair` buttons open Win32 color pickers. `Fore` colors
single glyphs and pair outlines, `Back` colors the grid/editor background, and
`Pair` colors the fill layer of merged pairs. The fill layer is drawn first and
the fore/outline layer is drawn over it. The editor uses the same fore/back roles
so exported constants are previewed against the selected theme.

`uwpchar.exe` derives companion pairs at runtime from the canonical generated
names, such as `FavoriteStar`/`FavoriteStarFill` and
`InkingColorOutline`/`InkingColorFill`. Pair cells are previewed by drawing both
glyphs into one rect in two colors. Clicking a merged pair exports both original
source names; the code does not rename them to outline/fill aliases.

The left output area is tabbed. `Output` is the existing multiline export edit.
`Layering` is the first composition surface: a square dark preview above a list
box of `value, name` layer rows. The current mock-up seeds `HeartFill` in red
under `Heart` in white so the layer order and color handling are easy to inspect.
While `Layering` is active, clicking the glyph grid adds a layer to the list;
clicking a visible merged MDL2 pair adds fill first and outline second.
The output edit is non-wrapping and uses a horizontal scrollbar so exported layer
blocks remain shaped like assembler source.

The layering page is buttonless. Left-click or right-click the large preview to
open its context menu; use `Append to output` there to write the current
`FONTICON_LAYER` array back to `Output`. In the layer list,
`Ctrl+Up`/`Ctrl+Down` or `+`/`-` reorder the selected row, `Enter`/`Space`
changes its color, and `Delete`/`Backspace` removes it. Double-clicking a list
item also removes it.

Exports are wrapped in font namespaces so short icon names do not collide with
application symbols:

```asm
namespace SegoeMDL2
	Camera := 0E722h
end namespace
```

Layer stacks export as:

```asm
label icon_layers:icon_layers.bytes/sizeof.FONTICON_LAYER
	FONTICON_LAYER glyph: 0EB52h, color: 000000DCh ; HeartFill
	FONTICON_LAYER glyph: 0EB51h, color: 00FFFFFFh ; Heart
.bytes = $ - icon_layers
```

`glyphset.exe` is the first layered glyph-set viewer. It loads a growing
collection from `glyphset_layers.inc`, shows the named configurations in a list,
and previews the selected `FONTICON_LAYER` array at large size. Arrow keys in
the list navigate normally; the preview also accepts arrow keys, space, home/end,
and mouse wheel navigation.

To add work from `uwpchar`, paste the emitted `label icon_layers:...` block into
a namespace in `glyphset_layers.inc`, add a title string, then add one raw item
row pointing at `namespace_name.icon_layers`. The sample rows use
`namespace_name.icon_layers.bytes / sizeof.FONTICON_LAYER` for their layer count,
which keeps the table independent of any `sizeof.<qualified-label>` parser
limitations.

## Notes

The helpers take a font face pointer and a raw PUA codepoint. The demo uses
`Segoe Fluent Icons`, but `Segoe MDL2 Assets` works the same way for codepoints
present in that font. Recreate fonts and cursor bitmaps when DPI or theme state
changes; do not cache raster results across DPI changes.

`uwpchar_data.inc` is generated by `make_uwpchar_data.py` from Microsoft's
published Segoe MDL2 Assets and Segoe Fluent Icons markdown tables in the
`MicrosoftDocs/windows-dev-docs` repository. The MDL2 and Fluent name tables are
mostly the same by codepoint, so the generated data is intentionally small:
`uwpchar_codes` is a bounded codepoint bitfield, `uwpchar_names` stores word
offsets to compact name records through 8-codepoint blocks, and each record keeps
the word codepoint next to its ASCII string. `uwpchar_name_index` is an
offset-only reverse index sorted by ASCII name. Fluent names are preferred for
same-codepoint conflicts except the vetted MDL2 `StockDown` and `StockUp` choices
at `EB0F` and `EB11`. The include does not store font-specific
name tables, flags, pair records, wide strings, or 64-bit name pointers. Pair
discovery is a runtime interpretation of exact `Fill`, `Filled`, or `Solid` name
companions, with an `Outline` fallback for base names. Verify individual
compositions visually before treating them as product icons.

The `microsoft/fluentui-system-icons` repository has richer per-icon metadata,
but its generated font JSON uses codepoints for that project's packaged fonts,
not Windows' installed `Segoe Fluent Icons` mapping. Treat that repository as a
separate future importer, not as a direct replacement source for
`uwpchar_data.inc`.

The layered drawing path is intentionally opaque-surface GDI. For transparent
cached icon stacks, switch to glyph geometry (`GetGlyphOutlineW`) or
DirectWrite/Direct2D so alpha composition is correct.
