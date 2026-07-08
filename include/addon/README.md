# Addon Includes

`include/addon` holds reusable helpers that are useful across examples or apps
but are smaller and more opt-in than the main Win32 include surface.

Use this folder for small helpers and coherent helper packages with a documented
public surface. Keep application-specific feature modules beside the application
that owns their data model and UI policy.

Current addons:

- `windows.inc`: small Win64 GUI executable policy for modular examples.
- `color_HLS.inc`: Win2K ControlPaint HLS/COLORREF conversion and shade helpers.
- `csd/`: client-side-decoration mechanics split out of `examples/csd_basics`,
  including caption layout, DPI/theme/state/snap/backdrop helpers, tab-strip
  rows, child-HWND placement, policy-free UIA mechanics, and optional struct
  offset probes.
- `mru_api.inc`: comctl32 MRU ordinal binding and small descriptor helpers.
- `system_dark_mode.inc`: Windows app-mode dark theme detection helper.
- `uah.inc`: verified WM_UAH message/structure support and tolerant uxtheme
  private-ordinal wrappers.
