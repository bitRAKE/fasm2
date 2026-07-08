# CSD Metrics And Constants

This companion keeps the current examples non-destructive: no source constants
are moved yet. The goal is to make the numbers readable enough that a learner
can tell a Win32 contract from a demo design choice.

Use this with `CONCEPTS.md`:

- `CONCEPTS.md` explains the CSD decisions.
- this file classifies the constants those decisions depend on.

## Classification

The CSD examples use four kinds of numeric values:

| Kind | Meaning | Examples |
| --- | --- | --- |
| Mechanical Win32 contracts | Values with external API or platform meaning. | `96` DPI base, `22000` Windows build, `1` DWM margin. |
| Reusable layout policy | Values that shape the caption algorithm and could become `csd_metrics.inc`. | title height, edge width, drag pad, button width. |
| Demo visual policy | Colors, glyphs, and initial window sizes chosen for the examples. | accent-like colors, Segoe MDL2 glyph IDs, `920x500` client size. |
| Table identity | IDs and indexes that connect descriptors to command routing. | `ID_CAPTION_*`, `CSD_CAPTION_MAX_INDEX`. |

Now that the reusable mechanics live in `include/addon/csd/`, only the first
two categories should become shared policy by default. Demo colors, glyph
choices, and app command IDs stay application-owned.

## Cross-Rung Geometry

| Value | Current source | Category | Rationale |
| --- | --- | --- | --- |
| `CSD_TITLE_HEIGHT = 52` | `01`-`03` | Reusable layout policy, fixed-pixel teaching value | The pre-DPI rungs paint a simple 96-DPI title band. Keeping it fixed avoids introducing DIP conversion before `05`. |
| `CSD_TITLE_HEIGHT_DIP = 56` | `04`-`08`, `18` | Reusable layout policy | Once caption controls exist, the caption is slightly taller and expressed in DIPs so `05` can make geometry per-monitor aware without rewriting descriptors. `18` returns to this 05-style surface for the manifest matrix. |
| `CSD_TITLE_HEIGHT_DIP = 48` | `09`-`17`, `19` | Reusable layout policy, local override | The embedded Search `EDIT` makes the title area feel more like a compact title-bar search field. This is intentional, not drift from `56`. |
| `CSD_CONTENT_PAD` / `CSD_CONTENT_PAD_DIP = 26` | `01`-`19` | Demo visual policy that also feeds layout | The body text inset and later drag pad are visual spacing choices. The value is not a Win32 metric. |
| `CSD_DRAG_PAD_DIP = CSD_CONTENT_PAD[_DIP] / 2` | `04`-`19` | Reusable layout policy | Leaves a small gap between caption controls and draggable or tabbed caption space so edge-adjacent controls do not feel fused to the drag strip. |
| `CSD_EDGE_PX` / `CSD_EDGE_DIP = 4` | `01`-`19` | Demo visual policy | This is the app-drawn visible edge. It is not the resize hit-test thickness. |
| `CSD_RESIZE_PX = 8` | `02`-`04` | Reusable layout policy for pre-DPI rungs | The fixed hit-test strip returns `HTLEFT`/`HTRIGHT`/`HTTOP`/`HTBOTTOM` before `GetSystemMetricsForDpi` is introduced. |
| fallback `8` DIPs in `CsdGetResizeBorderForDpi` | `include/addon/csd/dpi.inc` | Reusable layout policy fallback | Used only if `GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi)` fails or returns zero. The system metric is preferred. |
| `CSD_BUTTON_WIDTH_DIP = 44` | `04`-`19` | Reusable layout policy | Owner-drawn caption buttons use a stable intrinsic width independent of glyph width. |
| `CSD_ICON_PX = 18` | `04` | Demo visual policy | Static 96-DPI icon font size for the first caption-control rung. |
| `CSD_ICON_DIP = 18` | `05`-`19` | Reusable layout policy | Same visual size as `04`, expressed in DIPs once the rung has DPI support. |
| `CSD_SEARCH_WIDTH_DIP = 240` | `09`-`17`, `19` | Demo visual policy | Reserves a wide caption slot for the live child `EDIT`; not a general search-box rule. |
| `CSD_CHILD_PAD_X_DIP = 8` | `09`-`17`, `19` | Reusable child-layout policy | Horizontal inset between the Search descriptor rectangle and the child edit HWND. |
| `CSD_CHILD_PAD_Y_DIP = 12` | `09`-`17`, `19` | Reusable child-layout policy | Vertical inset that keeps the edit control inside the shorter `48` DIP caption row. |
| `CSD_TAB_MIN_WIDTH_DIP = 96`, `CSD_TAB_PREF_WIDTH_DIP = 150` | `13`-`15` | Demo tab-fit policy | Tabs shrink before clipping; `14_responsive_caption` adds descriptor overflow around the tab band, and `15_rtl_caption` mirrors the tab flow. |
| `CSD_TAB_DRAG_RECT_MAX = 8` | `13`-`15` | Teaching-layout cap | Fixed gap-rectangle storage keeps the multi-rectangle drag map visible. |
| `CSD_MIN_BODY_W_DIP = 520`, `CSD_MIN_BODY_H_DIP = 320` | `14`-`15` | Responsive minimum-size policy | Body/client minimums are combined with the irreducible caption width before `WM_GETMINMAXINFO` writes the top-level window track size. |
| `CSD_LAYOUT_RTL = 0001h` | `15` | RTL layout policy | Runtime flag toggled by F2; it mirrors semantic leading/trailing geometry while physical resize hit-tests remain unchanged. |
| `CSD_INITIAL_CLIENT_W_DIP = 920` | `05`-`19` | Demo visual policy | Desired initial client width before `AdjustWindowRectExForDpi` expands it to a top-level window. |
| `CSD_INITIAL_CLIENT_H_DIP = 500` | `05`-`19` | Demo visual policy | Desired initial client height before frame adjustment. |
| `CSD_TOOL_CLIENT_W_DIP = 560`, `CSD_TOOL_CLIENT_H_DIP = 300` | `16` | Demo topology policy | Desired client size for the owned tool frame; it is intentionally smaller than the main window. |
| `CSD_WINDOW_MAX = 4`, `CSD_WINDOW_MAIN = 0001h`, `CSD_WINDOW_TOOL = 0002h` | `16` | Multi-window ownership policy | Small live-context registry capacity and role flags for status/debug behavior. |
| `CSD_SHADOW_MARGIN_X_DIP = 28`, `CSD_SHADOW_MARGIN_TOP_DIP = 18`, `CSD_SHADOW_MARGIN_BOTTOM_DIP = 34` | `17` | Demo shadow policy | Extra layered-window surface around the owner. These margins are visual shadow extents, not resize hit-test geometry. |
| `CSD_SHADOW_RADIUS_DIP = 30`, `CSD_SHADOW_MAX_ALPHA = 96` | `17` | Demo shadow policy | Falloff distance and maximum opacity for the generated DIB alpha. |
| `CSD_VIEWER_HEIGHT_DIP = 210`, `CSD_VIEWER_GAP_DIP = 18` | `19` | Demo backdrop-viewer policy | Bounded material viewport height and spacing. The values are presentation choices, not DWM requirements. |

## Mechanical Values

| Value | Current source | Contract |
| --- | --- | --- |
| `CSD_DPI_BASE = 96` | `include/addon/csd/caption.inc` | Windows logical DPI baseline. `CsdDipToPx` converts DIPs with `MulDiv(value, dpi, 96)`. |
| `CSD_TEXT_FONT_PX96 = 13` | `include/addon/csd/dpi.inc` | Base Segoe UI text height before DPI scaling. `CsdCreateTextFontForDpi` negates the scaled height to request character height. |
| `MARGINS ... 1` | `03`-`16` | One-pixel `DwmExtendFrameIntoClientArea` margin re-enters the DWM frame path for shadow and compositor animations without restoring a visible system frame. `17` intentionally removes it; `19` uses full-client margins for the material viewer experiment. |
| `MARGINS ... -1` | `19` | Full-client `DwmExtendFrameIntoClientArea` sheet used only for the visible backdrop viewer. |
| Manifest `dpiAware` values `true`, `true/pm`; manifest `dpiAwareness` value `PerMonitorV2, PerMonitor` | `18_dpi_manifest_matrix/manifests` | DPI-awareness declarations consumed by the Windows loader. These strings are the experiment inputs for the matrix, not app layout policy. |
| `HCF_HIGHCONTRASTON = 00000001h`, `SPI_GETHIGHCONTRAST` | `10`, `include/equates/user64.inc` | High-contrast system policy. `10` queries it with `SystemParametersInfoW` and switches owner-drawn caption colors to system colors. |
| `WS_EX_NOINHERITLAYOUT = 00100000h`, `WS_EX_LAYOUTRTL = 00400000h` | `15` | SDK-defined extended styles missing from the local USER equates during this rung. `15` defines them locally; it uses `WS_EX_NOINHERITLAYOUT` for the neutral Search child and avoids relying on top-level `WS_EX_LAYOUTRTL` for owner-drawn geometry. |
| `CSD_WS_EX_NOACTIVATE = 08000000h` | `17` | SDK-defined extended style missing from the local USER equates during this rung; it keeps the shadow popup out of activation/focus behavior. |
| `AC_SRC_OVER = 0`, `AC_SRC_ALPHA = 1`, `ULW_ALPHA` | `17`, `include/equates/user64.inc` | `UpdateLayeredWindow` alpha contract. `AC_SRC_ALPHA` requires premultiplied BGRA pixels in the source DIB. |
| `CSD_SNAP_WINDOWS11_BUILD = 22000` | `include/addon/csd/snap.inc` | Windows 11 RTM build gate for the snap-layout `HTMAXBUTTON` policy. |
| `CSD_BACKDROP_WINDOWS11_BUILD = 22621` | `include/addon/csd/backdrop.inc` | Windows 11 build gate for `DWMWA_SYSTEMBACKDROP_TYPE`; separate from the snap-layout gate. |
| `CSD_BACKDROP_ALPHA_BUILD = 26100` | `include/addon/csd/backdrop.inc` | Build gate for `DWMWA_REDIRECTIONBITMAP_ALPHA`, which is separate from merely selecting a system backdrop type. |

The DWM margin looks like geometry, but its reason is platform behavior rather
than visual sizing. Do not fold it into a generic edge-width metric.

## Raw Local Offsets

The early fixed-pixel rungs also contain raw `8` pixel offsets in the body text
placement:

| Source shape | Current source | Meaning |
| --- | --- | --- |
| `CSD_TITLE_HEIGHT - 8` | `01`-`03` | Pulls the muted lower text upward inside the body text block. |
| `CSD_EDGE_PX + 8` | `01`-`03` | Keeps the lower text clear of the visible bottom edge. |
| `CSD_TITLE_HEIGHT_DIP - 8` | `04` | Same visual adjustment before the full DPI rung exists. |
| `CSD_EDGE_DIP + 8` | `04` | Same bottom spacing adjustment before the full DPI rung exists. |

These are demo typography offsets, not frame mechanics. If reusable metrics move
into source, name these as text-layout constants or leave them local with a
short comment.

## Caption Identity

Caption IDs are app-owned command identities, not reusable frame metrics:

| Value set | Current source | Role |
| --- | --- | --- |
| `ID_CAPTION_SYSTEM = 4201` through `ID_CAPTION_MIN = 4206` | `04`-`18` | Descriptor identity used by routing and status text. |
| `CSD_CAPTION_SEARCH_INDEX = 1` | `09`-`17` | Index into the descriptor/geometry rows for the child edit slot. |
| `CSD_CAPTION_MAX_INDEX = 4`, then `5` and `6` as rows are inserted | `08`-`17` | Index used by the Windows 11 snap bridge to special-case the maximize row. `18` keeps 05-level hit-testing and does not include the snap bridge. |
| `CSD_CAPTION_CONTROL_COUNT = (...) / sizeof.CSD_CAPTION_DESCRIPTOR` | `04`-`18` | Assemble-time count derived from table size; this is the preferred pattern. |
| `keyboard_order dd 0,2,5,4,3`, `CSD_KEYBOARD_ORDER_COUNT = 5` | `10` | Explicit keyboard focus order for owner-drawn rows: System menu, Settings, Minimize, Maximize/Restore, Close. This is visual order, not descriptor order. |
| `ID_CAPTION_OVERFLOW = 4208`, `ID_OVERFLOW_* = 4301`-`4303` | `14` | App-owned overflow command IDs; these deliberately do not use `SC_*` system-command values. |
| `CSD_CAPTION_OVERFLOW_INDEX = 3`, `CSD_CAPTION_MAX_INDEX = 6` | `14` | Table-position contracts after the overflow row is inserted between New Tab and Settings. |

The hard-coded indexes are correct for the current descriptor order, but they
are table contracts. If rows are inserted, these constants must move with the
descriptor table or become derived by a small lookup helper.

## Glyph And Palette Values

The icon constants are Segoe MDL2 Assets codepoints used through
`examples/font_icons/font_icons.inc`:

| Value set | Current source | Role |
| --- | --- | --- |
| `ICON_MENU`, `ICON_CLOSE`, `ICON_MINIMIZE`, `ICON_MAXIMIZE`, `ICON_RESTORE`, `ICON_SEARCH`, `ICON_SETTINGS`, `ICON_ADD`, `ICON_MORE` | `04`-`18` | Demo icon choices for the caption descriptors. |
| `CSD_TITLE_BG`, `CSD_BUTTON_BG`, `CSD_CLOSE_BG`, `CSD_EDGE_COLOR`, `CSD_TITLE_TEXT`, `CSD_BODY_TEXT`, `CSD_MUTED_TEXT`, `CSD_STATUS_TEXT` | early rungs through `05` | Fixed `COLORREF` demo palette before live theme state. |
| `CSD_THEME_*` | `include/addon/csd/theme.inc` | Default and derived palette values used once `06` introduces DWM accent/dark-mode state. |
| `CSD_STATE_BUTTON_*`, `CSD_STATE_CLOSE_*`, `CSD_STATE_TEXT_INACTIVE` | `07`-`17` | Terminal colors for hover, pressed, and inactive caption-control state. |

Palette values are `COLORREF` (`0x00BBGGRR`), even when comments show
human-facing RGB order. The ARGB-to-`COLORREF` conversion in
`include/addon/csd/theme.inc` belongs to DWM colorization input, not to these
fixed constants.

## Promotion Guidance

If source cleanup follows this documentation pass, the likely split is:

| Destination | Belongs there |
| --- | --- |
| `csd_metrics.inc` | `CSD_DPI_BASE`, caption height defaults, edge width, button width, icon size, drag pad, child padding, resize fallback. |
| app source | initial window size, colors, glyph choices, command IDs, descriptor row order, text-layout polish offsets. |
| `include/addon/csd/snap.inc` | `CSD_SNAP_WINDOWS11_BUILD`, because it is policy for the snap probe rather than a visual metric. |
| `include/addon/csd/theme.inc` | `CSD_THEME_*` fallback palette, because those constants define theme-state defaults. |

Do not centralize a value just because it repeats. Centralize it when a reader
or later example needs the same semantic contract, and keep local demo choices
near the source that owns the visual result.
