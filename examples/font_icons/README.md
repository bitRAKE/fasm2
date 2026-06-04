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

Run:

```cmd
examples\font_icons\font_icon_demo.exe
examples\font_icons\uwpchar.exe
```

`font_icon_demo.exe` shows these supported uses:

- a top-row comctl32 toolbar with owner-painted glyph buttons: Add/rest,
  Save/disabled, Search/pressed, Refresh/active, plus hover backplates during
  interaction;
- a lower-left `STATIC` control that behaves like an icon label;
- direct layered glyph drawing in the center panel on an opaque GDI paint
  surface;
- a glyph-backed cursor selected by `WM_SETCURSOR`.

`uwpchar.exe` is an assembly port of the local `C:\git\!bitRAKE\tools\uwpchar.c`
browser. It enumerates real glyphs with `GetGlyphIndicesW`, renders a
scrollable grid for `Segoe MDL2 Assets` or `Segoe Fluent Icons`, and appends
fasm constants when cells are clicked. The `Merged pairs` checkbox collapses
MDL2 fill companions into one two-color cell; clearing it shows the raw font
glyph list. The default palette is dark so layered glyph colors are visible
without changing settings first.

The `Fore`, `Back`, and `Pair` buttons open Win32 color pickers. `Fore` colors
single glyphs and pair outlines, `Back` colors the grid/editor background, and
`Pair` colors the fill layer of merged pairs. The fill layer is drawn first and
the fore/outline layer is drawn over it. The editor uses the same fore/back roles
so exported constants are previewed against the selected theme.

For Segoe MDL2 Assets, `uwpchar_data.inc` carries 46 name-derived companion
pairs such as `FavoriteStar`/`FavoriteStarFill` and
`InkingColorOutline`/`InkingColorFill`. Pair cells are previewed by drawing both
glyphs into one rect in two colors. Clicking a merged pair exports both original
source names; the code does not rename them to outline/fill aliases.

## Notes

The helpers take a font face pointer and a raw PUA codepoint. The demo uses
`Segoe Fluent Icons`, but `Segoe MDL2 Assets` works the same way for codepoints
present in that font. Recreate fonts and cursor bitmaps when DPI or theme state
changes; do not cache raster results across DPI changes.

`uwpchar_data.inc` is generated from the local `uwpchar_names.h` table. Names are
ASCII byte strings, and generated `UWPCHAR_NAME_ENTRY` initializers omit unused
or zero fields. The pair list is conservative metadata inferred from MDL2 names
with `Fill`, `Filled`, or `Solid` companions, but the exported names remain the
raw source names; verify individual compositions visually before treating them as
product icons.

The layered drawing path is intentionally opaque-surface GDI. For transparent
cached icon stacks, switch to glyph geometry (`GetGlyphOutlineW`) or
DirectWrite/Direct2D so alpha composition is correct.
