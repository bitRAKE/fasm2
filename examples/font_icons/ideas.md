
Okay, maybe we should look at some tools: https://github.com/bitRAKE/tools

We have it locally at `C:\git\!bitRAKE\tools\`

We want to use the `uwpchar` font-icons in our applications/tools. From the character values: how to create a toolbar?, Render icon-like in dialogs? Can we create a pseudo-pointer from a glyph?

---

`uwpchar` hands you two things per icon: a PUA codepoint (`0xE74E`) and a face name (`Segoe Fluent Icons` / `Segoe MDL2 Assets`). All three of your questions are then the *same* operation wearing three costumes — render that codepoint through a GDI `HDC` with the icon font selected. The only variable is the target surface: a toolbar button's DC, a static control's client area, or an offscreen DIB that becomes an `HICON`. So the vetted piece is one primitive, and the three features are linkages of rising complexity off it:

```c
// The whole "piece". color via SetTextColor, size via the font's height.
static void DrawGlyph(HDC hdc, WCHAR cp, const RECT *rc, HFONT hIcon, COLORREF clr) {
    int   bk   = SetBkMode(hdc, TRANSPARENT);
    COLORREF c = SetTextColor(hdc, clr);
    HFONT old  = SelectObject(hdc, hIcon);
    RECT   r   = *rc;
    DrawTextW(hdc, &cp, 1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    SelectObject(hdc, old);
    SetTextColor(hdc, c);
    SetBkMode(hdc, bk);
}
```

The font itself: `CreateFontW(-pixels, 0, 0, 0, FW_NORMAL, 0,0,0, DEFAULT_CHARSET, ... CLEARTYPE_QUALITY, ..., L"Segoe Fluent Icons")`. `-pixels` because these are vector — the entire reason to prefer them over an imagelist is DPI-independence and recolor-by-`SetTextColor`. Recreate the `HFONT` on `WM_DPICHANGED`; never cache a raster.

**Toolbar.** Use the real comctl32 toolbar (`InitCommonControlsEx` with `ICC_BAR_CLASSES`, `TBSTYLE_FLAT`), and let it do all the state work — don't owner-draw the whole button. The trick: set each button's *text* to the single glyph wchar, then intercept `NM_CUSTOMDRAW` (the toolbar sends `TBCUSTOMDRAW`). At `CDDS_ITEMPREPAINT`, `SelectObject` your icon font into `nmcd->hdc`, set `clrText`, and return `CDRF_NEWFONT | TBCDRF_USECDCOLORS`. The control then renders the glyph as text for you — centered, with hot-track, pressed, and disabled states free, and you still get tooltips and `TBN_` notifications. You get vector glyphs *inside* the stock machinery. The raster fallback — render each glyph to a 32bpp DIB and `ImageList_Add` it into the toolbar's imagelist — works with zero custom draw but throws away the DPI/recolor win and forces regeneration per DPI, so it's the escape hatch, not the default.

**Icon-like in a dialog.** Don't owner-draw unless you must. A plain `STATIC` whose font is the icon font and whose window text is the glyph *is* the icon: `SetWindowTextW(hStatic, L"\uE74E")` + `SendMessageW(hStatic, WM_SETFONT, (WPARAM)hIconFont, TRUE)`. Color it by handling `WM_CTLCOLORSTATIC` (`SetTextColor` on the returned DC). Size follows the font height; alignment via `SS_CENTER`/`SS_CENTERIMAGE`. That's it — no `WM_DRAWITEM`. The escape hatches: `SS_OWNERDRAW` + `DrawGlyph` when you need precise compositing, or rasterize to an `HICON` and `STM_SETICON` when something downstream demands a real icon handle. Watch the dialog's own font — the template sets Segoe UI 9pt via `DS_SETFONT`; you're overriding only the icon control.

**Pseudo-pointer from a glyph.** Yes, and this is where the gotcha lives — which is fitting, since it's the most volatile of the three. A cursor is just an `HICON` built with `fIcon = FALSE`, so the path is: render the glyph into a **32bpp top-down DIB section**, then `CreateIconIndirect`. The trap: GDI text rendering does *not* produce a real alpha channel — ClearType/AA blends against the background and leaves the alpha byte untouched, so a naive `DrawText` onto a transparent DIB gives you garbage edges on a cursor. The dependency-light fix is to synthesize coverage: draw the glyph black-on-white into the DIB, then walk the pixels and set `A = 255 - luma`, `RGB = your color × (A/255)` — straight (non-premultiplied) alpha, since `CreateIconIndirect` wants straight alpha (unlike `UpdateLayeredWindow`, which wants premultiplied — easy to conflate).

```c
ICONINFO ii = {0};
ii.fIcon    = FALSE;            // FALSE => cursor; hotspot fields live
ii.xHotspot = tipX;            // put this at the glyph's visual point
ii.yHotspot = tipY;
ii.hbmColor = hDib32;          // 32bpp, alpha synthesized as above
ii.hbmMask  = hMaskMono;       // 1bpp, all zero -> per-pixel alpha governs
HCURSOR hc  = (HCURSOR)CreateIconIndirect(&ii);
```

The hotspot is what makes it a *pointer* rather than a decoration — pick a glyph with an actual tip and aim the hotspot there. Drive it via `WM_SETCURSOR` (per-window) or `SetClassLongPtr(GCLP_HCURSOR)` (per-class). Size it to `SPI_GETCURSORSIZE` × DPI so it tracks the accessibility cursor-size setting. If you want crisp anti-aliasing with genuine alpha instead of the luma trick, the high-fidelity path is DirectWrite drawing into a Direct2D/WIC BGRA target — correct, but it drags in `dwrite`/`d2d1`/`windowscodecs`, so reserve it for when the synthesized-coverage edges aren't good enough.

So the construction-set read: `DrawGlyph` is the transparent piece you own outright; the toolbar and dialog cases are thin linkages over it; the cursor is the one that earns real vetting, because that's where the alpha semantics bite.

---

`uwpchar` doesn't even belong with those console tools - I should spin it off with some examples.  Layering colored glyphs to create a theme identity, active/disabled/hover states, etc.

---

Agreeing it belongs elsewhere is the easy part — your README already files it under "transient, migrate when similar tools accumulate." The sharper consequence is that the instant you say "layering for theme identity," uwpchar stops being a *browser* and becomes an *authoring* tool, and its current export breaks. `#define NAME 0xXXXX` is the right output for one monochrome glyph; a layered, themed, stateful icon isn't a codepoint, it's a small composition. So the spin-off's reason to exist beyond moved folders is that it closes a loop — author the composition in uwpchar, export the *composition*, consume it at runtime. That loop is the example worth building, and it's what justifies the separate repo.

The idea that makes "theme identity" real rather than per-icon fiddling: a themed icon is a **layer stack that references roles, not colors**, resolved at draw time by (theme × state). A layer is {source, role, transform}, where source is a glyph codepoint or a primitive (a rounded-rect backplate), role is semantic (`surface`, `accent`, `muted`, `outline`), and the theme is the agreement mapping role → `COLORREF`. Identity emerges because *every* icon obeys the same role map — not from any single icon's colors. Reskinning edits one artifact; the icons never change. That's the externalized-config pattern from before: palette in the registry, primed by the install script, never baked into the binary.

```c
typedef enum { ROLE_SURFACE, ROLE_ACCENT, ROLE_MUTED, ROLE_OUTLINE } Role;
typedef enum { ST_REST, ST_HOVER, ST_PRESSED, ST_ACTIVE, ST_DISABLED } State;

typedef struct { WCHAR cp; Role role; int dx, dy; } Layer;  // cp==0 => backplate
typedef struct { Layer layer[4]; int count; } Icon;

COLORREF Resolve(const Theme *t, Role r, State s);  // theme × state -> color
```

States then aren't separate assets — they're a transform over the resolved palette, which is exactly why `Resolve` takes the state and the renderer doesn't. Disabled is desaturate plus roughly a third opacity (the Fluent convention); hover makes a backplate layer appear at low-alpha accent; pressed darkens and insets; active is a persistent accent fill; focus is a *separate* ring drawn around the stack, never part of it. You add a state by adding a transform, not by re-rendering forty icons.

Here is where the cheap path strains, and it's worth knowing before you write the examples: `DrawText`-twice composites cleanly only onto an *opaque* surface. Stack two glyphs on a *transparent* DIB and GDI's text AA blends the second layer against garbage and never writes correct alpha — the composite has dirty edges the moment it sits over anything. Three ways out, rising: (1) always composite on a backplate, which hover/pressed want anyway, so for toolbar buttons it's free; (2) drop to `GetGlyphOutlineW` with `GGO_NATIVE`/`GGO_BEZIER`, take the real contours, and fill and stroke them yourself through a GDI path — geometry composites cleanly, gives you true outlines and strokes, stays dependency-light, and is precisely your kind of seam between "font glyph" and "your own compositor"; (3) Direct2D/DirectWrite when you want gradients and correct alpha everywhere, at three more imports. Route (2) is the one I'd reach for — it's where layering stops being a hack and starts being a renderer you own.

Two things to fold in. The font already does part of this: Segoe MDL2 Assets ships companion-codepoint pairs designed to layer — a background/outline glyph and a fill glyph registered to the same em box, meant to be drawn at one rect in two colors. uwpchar shows them as unrelated cells today; an authoring tool should surface the pairing, because "the two-layer status badge" is one decision, not two. And to keep the elegant version fast: hold the *authoring* form as vector, but cache the *composited* result into a DIB keyed by (stack, size, state, theme-generation), and bump a generation counter on theme edit or `WM_DPICHANGED` so the whole cache invalidates at once — no per-icon bookkeeping, instant reskin, cheap repaint.

---

UI direction: `uwpchar` now uses a resource-backed growable dialog as its main
window. The left-side output area is tabbed. `Output` owns the export edit;
`Complex` owns a square-ish preview above a layer list and an append path for
`FONTICON_LAYER {glyph,color}` arrays. The first mock-up is the heart stack:
`HeartFill` in red, then `Heart` in white, rendered by
`FontIcon_DrawLayeredGlyphArray`.

The `Complex` tab is intentionally buttonless. The preview is the command
surface: left-click appends, right-click opens a context menu. The list handles
editing from the keyboard: `Ctrl+Up`/`Ctrl+Down` and `+`/`-` reorder,
`Enter`/`Space` changes color, and `Delete`/`Backspace` removes.

Next growth for `Complex`: decide if list order should display bottom-to-top or
top-to-bottom stack semantics more explicitly, add named array labels so
repeated appends do not reuse `icon_layers`, and consider a drag/drop ordering
path after the keyboard model feels right. Longer term, the tab should move from
raw `COLORREF` records to role-based layers so themes and interaction states can
be resolved at draw time.

