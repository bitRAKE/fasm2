# Addon Includes

`include/addon` holds reusable helpers that are useful across examples or apps
but are not yet broad or coherent enough to deserve their own include
subdirectory.

Use this folder for small, opt-in helpers with a documented public surface. Keep
application-specific feature modules beside the application that owns their data
model and UI policy.

Current addons:

- `windows.inc`: small Win64 GUI executable policy for modular examples.
- `mru_api.inc`: comctl32 MRU ordinal binding and small descriptor helpers.
- `system_dark_mode.inc`: Windows app-mode dark theme detection helper.
