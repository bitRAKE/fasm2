
## Client-Side Decoration (CSD)

Chrome implements a custom window frame to embed its tabs and profile controls directly into the space traditionally reserved for the OS title bar. This approach is known as Client-Side Decoration (CSD). To achieve this while retaining native Windows desktop features (like snapping, drop shadows, and minimize/maximize animations), Chrome must override standard Win32 window mechanics and coordinate with the Desktop Window Manager (DWM).

The architecture relies on a progression of four primary mechanisms.

### 1. Reclaiming the Frame (`WM_NCCALCSIZE`)

A standard Windows application is divided into a **Non-Client Area** (the title bar and window borders drawn by the OS) and a **Client Area** (the inner content drawn by the application).

To take over the frame, Chrome intercepts the `WM_NCCALCSIZE` (Non-Client Calculate Size) message. When Windows queries the application to determine the layout of these areas, Chrome intercepts the message and modifies the return parameters to instruct the OS that the Client Area spans the entire bounds of the window. This effectively erases the native title bar and borders, providing Chrome with a blank canvas from edge to edge.

### 2. Restoring Native Input (`WM_NCHITTEST`)

Because Chrome erased the native Non-Client Area in the previous step, Windows no longer knows how to handle window dragging, resizing, or native window control clicks. The OS assumes the mouse is strictly interacting with standard application content.

Chrome resolves this by intercepting `WM_NCHITTEST` (Non-Client Hit Test). Every time the mouse moves or clicks over the window, Windows sends this message. Chrome maps the cursor's coordinates against its own custom-drawn UI components and returns native Win32 constants back to the OS:

* **`HTCAPTION`:** Returned when the mouse is over the empty space next to the tabs. This tells Windows to handle the input as a title bar click, allowing the user to drag or snap the window natively.
* **`HTMINBUTTON` / `HTMAXBUTTON` / `HTCLOSE`:** Returned when the cursor hovers over Chrome's custom-drawn window control buttons in the top right.
* **`HTTOPLEFT` / `HTRIGHT` / etc.:** Returned when the cursor is at the outer edges of the custom frame, signaling Windows to enable native drag-to-resize cursors.

### 3. Preserving DWM Features (`DwmExtendFrameIntoClientArea`)

Stripping the native frame via `WM_NCCALCSIZE` removes the system-rendered drop shadow and disables hardware-accelerated DWM window animations.

To restore these features without losing the custom UI, Chrome utilizes the DWM API, specifically `DwmExtendFrameIntoClientArea`. By calling this function and passing a margin structure (often extending a margin of just 1 pixel), Chrome tricks the DWM. The Desktop Window Manager registers the window as possessing a native frame, successfully re-applying the system drop shadow and native window manager animations to Chrome's borderless canvas.

### 4. Chrome's Internal Routing (The Views Framework)

Chrome does not evaluate these Win32 messages directly within its rendering engine. It handles them via its cross-platform UI framework, known as **Views** (backed by the Aura windowing system).

* **`HWNDMessageHandler`:** This class acts as the direct Win32 message pump listener. It catches `WM_NCCALCSIZE`, `WM_NCHITTEST`, and DWM-related messages.
* **`DesktopWindowTreeHostWin`:** This class serves as the bridge between the native Windows `HWND` (the low-level window pointer) and Chrome's platform-agnostic UI node tree.
* **`GlassBrowserFrameView` / `OpaqueBrowserFrameView`:** When `HWNDMessageHandler` receives a hit-test request, it passes the raw pixel coordinates down through the Views tree to these frame classes. The frame view evaluates if the coordinates intersect with a UI element (like a Tab or the Omnibox) or if they hit empty frame space, allowing Chrome to accurately calculate which Win32 `HT` constant to return to the OS.

This architecture allows Chrome to run a completely custom, hardware-accelerated UI while acting functionally identical to a native Win32 window from the perspective of the Windows OS.

The local examples deliberately do not build a Views-like routing framework.
They write the `WindowProc` branches directly so each Win32 mechanism remains
inspectable in isolation. The caption descriptor/geometry/state table introduced
in `04_caption_controls.asm` is a reusable data spine, not a hidden UI toolkit;
`CONCEPTS.md` maps each survey concept to the first source anchor that implements
it.

### 5. Using the Caption Surface

After `WM_NCCALCSIZE`, `WM_NCHITTEST`, and DWM frame extension are in place, the
application can treat the reclaimed caption as ordinary client pixels while
still returning native hit-test codes for the parts that should behave like the
system frame.

The fourth local example, `04_caption_controls.asm`, adds the caption table
spine used by the later examples. Immutable `CSD_CAPTION_DESCRIPTOR` rows own
ID, icon-font glyph, command, hit-test code, flags, and intrinsic width in DIPs.
The layout pass writes physical `RECT` values to a separate geometry column,
and a state column is reserved for later hover, pressed, disabled, inactive, and
fade phases. The renderer walks descriptor and geometry rows together to draw
caption buttons through `examples/font_icons/font_icons.inc`; the hit-test path
uses the same geometry to decide whether a point is app-owned client UI,
system-menu UI, a window-command button, or drag space.

Only the remaining strip between the left-side app controls and right-side
window controls returns `HTCAPTION`. This is the reduced movement area: clicks on
Search or Settings stay in the application, the menu glyph exposes the real
system menu, and the window-management buttons dispatch normal `WM_SYSCOMMAND`
messages without letting the standard caption-button renderer paint over the
custom surface. Because the application now owns that command path, it also
updates the max/restore command and system-menu enabled states from the current
`IsZoomed`/`IsIconic` result.

### 6. Making Geometry DPI-Dependent

Once the caption table is split, DPI becomes a geometry concern instead of a
rewrite of every hit-test branch. `05_dpi_frame.asm` keeps descriptor widths in
DIPs, stores physical rectangles in the geometry column, and passes the active
DPI into `CsdCaptionLayout`. The same descriptors therefore produce wider
caption buttons, a taller title band, and a larger reduced drag area at higher
scale factors.

The example opts into Per-Monitor V2 awareness with a manifest resource. It uses
`GetDpiForWindow` for the current window DPI, `GetSystemMetricsForDpi` for the
resize border, and `AdjustWindowRectExForDpi` when turning an initial client
size into a top-level window size. On `WM_DPICHANGED`, it applies the suggested
window rectangle before rebuilding DPI-sized icon and Segoe UI text font
resources and laying out the caption again.

### 7. Synchronizing With DWM Theme State

The next local step, `06_dwm_theme.asm`, keeps the DPI-dependent caption table
and adds DWM/theme synchronization. A `CSD_THEME` row stores the current app
dark-mode preference, the DWM accent color, derived caption/body/button colors,
the window corner preference, and a generation counter for later render caches.

The example uses `DwmGetColorizationColor` for the live accent color and
`DwmSetWindowAttribute` for `DWMWA_USE_IMMERSIVE_DARK_MODE`,
`DWMWA_CAPTION_COLOR`, `DWMWA_BORDER_COLOR`, `DWMWA_TEXT_COLOR`, and
`DWMWA_WINDOW_CORNER_PREFERENCE`. It refreshes this row on `WM_SETTINGCHANGE`
and `WM_DWMCOLORIZATIONCOLORCHANGED`, recreates the dependent GDI brushes, and
repaints the client-owned caption surface. The generation only advances when
the live accent or dark-mode input changes.

### 8. Mutating Caption State

With descriptor, geometry, DPI, and theme split out, `07_caption_state.asm`
starts writing the mutable state column. Because 04 intentionally reports the
custom buttons as `HTCLIENT`, this is ordinary client-area input:
`WM_MOUSEMOVE` arms `TrackMouseEvent(TME_LEAVE)`, `WM_MOUSELEAVE` clears the hot
row, and `WM_LBUTTONDOWN`/`WM_LBUTTONUP` manage a captured pressed index.

The renderer consumes the descriptor and state rows together. Normal caption
controls receive hover, pressed, and inactive fills; the close button keeps its
separate red hover/pressed treatment; `WM_ACTIVATE/WA_INACTIVE` maps the entire
button strip into an inactive visual state. The command path stays explicit:
window commands are only dispatched when the pressed control and release control
are the same row.

### 9. Version-Gating Snap Layouts

`08_snap_layouts.asm` is the counterpoint to the 04 `HTCLIENT` decision. On
Windows 11 builds, the shell snap-layout flyout expects the maximize glyph to
hit-test as `HTMAXBUTTON`. The example probes the true OS build with
`RtlGetVersion`, keeps down-level systems on `HTCLIENT`, and switches only the
maximize row to `HTMAXBUTTON` when the snap policy is enabled.

Returning `HTMAXBUTTON` changes the input channel for that one glyph: hover and
clicks arrive as non-client messages. The example keeps owner drawing by
bridging `WM_NCMOUSEMOVE`/`WM_NCMOUSELEAVE` back into the state column and by
handling `WM_NCLBUTTONDOWN`/`WM_NCLBUTTONUP` itself after translating the screen
coordinate `lParam` back to client coordinates. The rest of the caption table
continues to behave like 07.

### 10. Hosting Child Controls in the Caption

`09_embed_edit.asm` keeps the 08 snap policy and turns the Search row into a
real child `EDIT` window. The caption descriptor still reserves the rectangle
and returns `HTCLIENT`, but the new `CSD_CAPTION_CHILD` flag tells the parent
renderer to skip that row. The top-level window also uses `WS_CLIPCHILDREN`, so
the native edit owns its own pixels, caret, selection, text input, and IME.

The child is still laid out from the same geometry column as the owner-drawn
buttons. `include/addon/csd/child.inc` converts the Search row's `RECT` into an
inset child rectangle and `MoveWindow` applies it on create, size, and DPI
changes.
`WM_COMMAND/EN_CHANGE` is ordinary child-control notification traffic, while
`WM_CTLCOLOREDIT` keeps the edit colors aligned with the active theme. The
surrounding open caption strip remains the reduced `HTCAPTION` drag area.

### 11. Current Local Extensions

The later rungs keep the same message and table spine while adding costs that
real client-side captions accumulate:

* `10_a11y_keyboard.asm` adds keyboard entry/navigation/invocation for the
  09-level owner-drawn caption buttons, switches to system colors in high
  contrast, and exposes those owner-drawn rows through a local UI Automation
  provider while leaving the Search edit on its native child-HWND provider.
* `11_caption_fade.asm` animates state transitions with `WM_TIMER` while
  resolving colors at paint time so theme changes do not leave stale blends.
* `12_backdrop.asm` adds a DWM system-backdrop policy while keeping GDI repaint
  coverage explicit, proving set/read-back behavior without exposing material.
* `19_backdrop_viewer.asm` adds the extra DWM configuration, a patterned
  software probe, a mode-colored diagnostic panel, and a bounded
  premultiplied-alpha viewport needed to make software blend versus
  Mica/Acrylic material behavior visible without making the whole GDI window
  see-through.
* `13_caption_tabs.asm` promotes the open caption band into a dynamic tab strip,
  replaces the single drag rectangle with gap rectangles around tab bodies, and
  adds F6/arrow/Enter/Delete keyboard access plus UIA reporting for visible tab
  bodies and close glyphs.
* `14_responsive_caption.asm` hides optional caption rows by priority, exposes
  hidden app commands through an overflow menu, hides the Search child HWND
  when its row collapses, and writes a DPI-aware `WM_GETMINMAXINFO` minimum.
* `15_rtl_caption.asm` toggles semantic LTR/RTL caption layout with F2,
  mirrors tab and control geometry directly, uses RTL text/menu flags, and
  keeps physical resize-edge hit-testing unchanged.
* `16_multi_window.asm` stores CSD state in per-HWND `CSD_WINDOW` records,
  attaches them with `GWLP_USERDATA`, creates an owned tool frame, and uses a
  live-context registry to refresh theme state across windows.
* `17_custom_shadow.asm` removes the DWM margin trick and uses an owned layered
  popup with a premultiplied 32bpp DIB to show what shadow pixels, geometry
  tracking, and lifetime management cost when the compositor is not doing that
  work for the app.
* `18_dpi_manifest_matrix/` assembles the same DPI-aware caption source under
  four manifests and produces a TSV report, turning DPI-awareness from an
  assumption into inspectable binary metadata plus runtime observations.

---

The technique is ubiquitous in modern desktop development. The era of the strict, OS-rendered title bar is largely over for flagship software. Because the underlying Win32 API and Desktop Window Manager (DWM) are the absolute arbiters of how a window behaves on Windows, any application that wants controls in its title bar must perform the exact same message interception (`WM_NCCALCSIZE`, `WM_NCHITTEST`) as Chrome.

The difference is that modern applications rarely write the raw message pump handlers themselves; they rely on high-level UI frameworks that abstract the interception.

Here is how some other applications implement this technique:

### 1. Modern Windows File Explorer

The Windows 11 File Explorer was heavily rewritten to utilize **WinUI 3** (part of the Windows App SDK) alongside legacy XAML islands.

To bring the tabs into the title bar, WinUI provides a high-level API called `AppWindow.ExtendsContentIntoTitleBar`. When this boolean is set to true, the WinUI framework internally intercepts `WM_NCCALCSIZE` to expand the client area to the top edge. The framework then exposes properties allowing the developers to define specific drag regions (`SetDragRectangles`). When the mouse enters these rectangles, WinUI handles the `WM_NCHITTEST` math and returns `HTCAPTION` to the OS, allowing you to drag the File Explorer window by the space next to the tabs.

### 2. Modern Windows Notepad

The modern Windows 11 Notepad is also a **WinUI 3** application and shares the exact same `ExtendsContentIntoTitleBar` framework logic as the File Explorer to achieve its tabbed interface.

This heavy, composite UI architecture is a stark contrast to older, leaner text editors like Notepad++, which utilize a more traditional Win32 architecture (combined with Scintilla). The standard Win32 frame mechanics are significantly more transparent and amenable to direct system-level hooking or custom plugin engineering, whereas the modern Windows Notepad is an opaque, UWP-descended package that tightly encapsulates its DWM interactions.

### 3. Visual Studio IDE

Visual Studio is built primarily on **WPF (Windows Presentation Foundation)**. It has been using Client-Side Decoration for over a decade to embed the global search bar, user profile, and extension menus directly into the title bar space.

WPF handles this via the `WindowChrome` class. By applying a `WindowChrome` object to a WPF window, the framework essentially performs the DWM margin trick (`DwmExtendFrameIntoClientArea`) and strips the native frame. Developers then use an attached property called `WindowChrome.IsHitTestVisibleInChrome` on specific XAML elements. If this property is false, WPF translates mouse coordinates over that element into `HTCAPTION` or `HTSYSMENU` during the underlying Win32 hit-test, allowing Visual Studio's custom-drawn top bar to behave natively.

---

### The Architectural Shift

While heavy, monolithic IDEs and modern OS utilities embrace this tightly integrated UI approach, tools within a composable, version-controlled ecosystem often stick to standard window boundaries. Using standard Win32 frames avoids the overhead of complex hit-test routing and maintains predictable, lightweight execution—though at the cost of the modern, edge-to-edge aesthetic.
