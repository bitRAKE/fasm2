# `11_caption_fade` Implementation Brief

This is the implementation brief for the Tier 1 roadmap item. The corresponding
source now exists as `11_caption_fade.asm`; this document remains the design and
verification contract for the rung.

## Goal

Give the reserved `CSD_CAPTION_STATE.fadePhase` byte a real meaning by animating
owner-drawn caption-control state transitions:

- normal to hover;
- hover to pressed;
- pressed back to hover or normal;
- active to inactive, and inactive back to the live state;
- Windows 11 snap hover over the maximize row when `HTMAXBUTTON` is bridged
  through nonclient messages.

The animation must preserve the existing CSD mechanics:

- no render thread;
- no framework-style control tree;
- no replacement for the real system menu;
- the Search child `EDIT` row still owns its own pixels;
- state mutation still flows through the CSD state helper.

## Non-Goals

- Do not add physics, easing tables, or a compositor abstraction.
- Do not create or destroy GDI brushes every frame.
- Do not animate geometry changes from resize or DPI transitions.
- Do not make earlier rungs depend on a timer.
- Do not hide mouse, snap, or theme routing behind a generic message router.

## Starting Point

Prototype from `09_embed_edit.asm` once `10_a11y_keyboard.asm` exists, or from
`09` directly if the animation rung is developed first. The animation should
not regress these surfaces:

| Surface | Existing owner |
| --- | --- |
| Descriptor, geometry, state columns | `include/addon/csd/caption.inc` |
| Hover, pressed, inactive mutation | `include/addon/csd/state.inc` |
| Theme colors and generation counter | `include/addon/csd/theme.inc` |
| Snap hover bridge | `08_snap_layouts.asm` and `09_embed_edit.asm` |
| Child edit exclusion from owner drawing | `09_embed_edit.asm` |

## Include And Equate Gate

The repo already exposes the core APIs needed for a timer-driven fade:

| Surface | Local evidence |
| --- | --- |
| `WM_TIMER` | `include/equates/user64.inc` |
| `SetTimer`, `KillTimer` | `include/api/user32.inc` |
| `GetTickCount`, `QueryPerformanceCounter`, `QueryPerformanceFrequency` | `include/api/kernel32.inc` |
| `SetDCBrushColor`, `GetStockObject` | `include/api/gdi32.inc` |

One small stock-object equate is missing locally if the renderer uses
`DC_BRUSH` for transient blended fills. Windows SDK `10.0.26100.0` defines:

```asm
DC_BRUSH = 18
```

Add the equate beside the other GDI stock-object constants, or keep a local
definition in the first implementation if the shared include should not change
yet.

## State Shape

`CSD_CAPTION_STATE` is already four bytes:

```asm
struct CSD_CAPTION_STATE
  value       db ?
  fadePhase   db ?
  reserved    dw ?
ends
```

`11` can give the reserved word meaning without changing the row size:

```asm
CSD_CAPTION_ANIM_ACTIVE = 01h

struct CSD_CAPTION_STATE
  value       db ?       ; target state
  fadePhase   db ?       ; 0..255 blend progress toward value
  fromValue   db ?       ; state at transition start
  animFlags   db ?       ; CSD_CAPTION_ANIM_* bits
ends
```

This keeps existing descriptor, geometry, and state arrays aligned. Earlier
rungs can continue treating the last two bytes as unused.

Initialization should set:

- `value = CSD_STATE_NORMAL`;
- `fromValue = CSD_STATE_NORMAL`;
- `fadePhase = 255`;
- `animFlags = 0`.

A completed transition is represented by `fadePhase = 255` and no active flag.
This avoids a startup fade from an uninitialized source state.

## Transition Contract

`CsdCaptionStateApply` becomes the only place that starts a visual transition.
When the target state changes:

1. Read the old `value`.
2. Store the old `value` in `fromValue`.
3. Store the new target in `value`.
4. Store `0` in `fadePhase`.
5. Set `CSD_CAPTION_ANIM_ACTIVE`.
6. Return nonzero so the window can start or keep the timer running.

When the target state is unchanged, leave the phase and flags alone. Do not
restart an in-progress fade just because another mouse-move message reports the
same row.

For `CsdCaptionSetAllStates`, use the same immediate-set helper for startup and
theme rebasing. It should set both `fromValue` and `value` to the requested
state and mark the transition complete.

Leaf helper guidance applies here: the CSD state helpers that do not call
another proc should keep using `rcx`, `rdx`, and `r8` directly. Do not move a
state pointer into `rbx`, `rsi`, `rdi`, or `rax` only to address
`[reg+CSD_CAPTION_STATE.*]`.

## Timer Policy

Use a window timer as the teaching surface. It matches the existing Win32
message-loop style and cross-links naturally to `examples/gdi_snake`.

Suggested constants:

```asm
CSD_FADE_TIMER_ID       = 11
CSD_FADE_TIMER_MS       = 16
CSD_FADE_DURATION_MS    = 120
CSD_FADE_PHASE_MAX      = 255
```

Timer behavior:

- Start the timer when a state mutation returns nonzero and at least one row has
  `CSD_CAPTION_ANIM_ACTIVE`.
- On `WM_TIMER/CSD_FADE_TIMER_ID`, advance active rows and invalidate only the
  caption band or changed row rectangles.
- Kill the timer when no row remains active.
- Kill the timer in `WM_DESTROY` even if it should already be idle.

For the first implementation, a fixed phase increment is acceptable:

```asm
phase += CSD_FADE_PHASE_MAX * CSD_FADE_TIMER_MS / CSD_FADE_DURATION_MS
```

If timer stalls are visibly uneven, replace the fixed increment with a
`GetTickCount` accumulator:

- store the last tick when the timer starts;
- compute elapsed milliseconds on each `WM_TIMER`;
- advance phase by elapsed time instead of message count;
- clamp to `255`.

Use `QueryPerformanceCounter` only if the example needs sub-millisecond timing.
That adds more setup than this rung needs.

## Rendering Contract

The current renderer chooses one prebuilt brush after reading terminal state.
Animation needs two visual states and a blend phase:

```asm
fromColor = CsdCaptionStateColor(desc.flags, state.fromValue, theme)
toColor   = CsdCaptionStateColor(desc.flags, state.value, theme)
fill      = CsdBlendColorref(fromColor, toColor, state.fadePhase)
```

Keep the close button's danger palette in the resolver, not in the timer. The
timer should not know that close hover is red or that inactive text is muted.

Color helper contracts:

| Helper | Responsibility |
| --- | --- |
| `CsdCaptionStateColor` | Resolve descriptor flags plus state into a `COLORREF`. |
| `CsdBlendColorref` | Blend two `COLORREF` values by an unsigned byte phase. |
| `CsdCaptionGlyphColor` | Snap or blend glyph color, including inactive text. |

`COLORREF` is `00BBGGRR`, so blend channels by byte position without converting
through ARGB. Do not use `CsdArgbToColorref` for this; ARGB conversion is for
DWM colorization input, not per-frame GDI interpolation.

Prefer a stock DC brush for transient fill colors:

```asm
invoke  SetDCBrushColor,hdc,fillColor
invoke  GetStockObject,DC_BRUSH
invoke  FillRect,hdc,rectp,rax
```

This avoids per-frame `CreateSolidBrush` and `DeleteObject` churn. The existing
theme brushes can remain for static body, title, border, edit, and fallback
fills.

## Theme Generation Gate

`CSD_THEME.generation` increments when accent or dark-mode inputs change. The
fade implementation should use it to avoid blending stale colors from one theme
generation into another.

Recommended rule:

1. Store the theme generation observed by the animator.
2. Before a timer tick or paint pass, compare it with
   `current_theme.generation`.
3. If it changed, rebase active rows by setting `fromValue = value`,
   `fadePhase = 255`, and clearing `CSD_CAPTION_ANIM_ACTIVE`.
4. Invalidate the caption so the new theme paints immediately.

This trades a cross-theme fade for correctness and readability. Blending from a
light-theme hover color into a dark-theme hover color is visually noisy and
teaches the wrong cache policy.

## Message Integration

Add a small animation layer rather than spreading timer logic into every input
handler:

| Message/path | Animation responsibility |
| --- | --- |
| `WM_CREATE` | Initialize state rows as completed normal state. |
| Mouse and NC hover handlers | After `CsdCaption*` mutation, call `CsdCaptionAnimMaybeStart`. |
| `WM_ACTIVATE` | Start fades for active/inactive transitions or rebase immediately if chosen. |
| `WM_TIMER` | Advance phases, invalidate changed caption rectangles, stop when idle. |
| `WM_SETTINGCHANGE` and `WM_DWMCOLORIZATIONCOLORCHANGED` | Refresh theme, rebase active animations, repaint. |
| `WM_DPICHANGED` | Rebuild layout and fonts; do not reset fade state only because geometry moved. |
| `WM_DESTROY` | Kill the timer. |

Do not reduce the caption movement area. The reduced drag strip is a separate
geometry contract already established by `04`; animation only changes pixels.

## Verification

Build gate:

```cmd
cd C:\git\fasm2\examples\csd_basics
_build.cmd
```

Manual behavior gate:

- Hovering each owner-drawn row fades into hover and out again.
- Pressing and releasing a row fades through the pressed visual without losing
  capture or command routing.
- The maximize row still changes to the restore glyph while zoomed.
- On Windows 11, the `HTMAXBUTTON` snap hover bridge still updates the owner
  drawn visual and still allows dragging through the remaining caption strip.
- The Search child edit is not owner-painted or faded.
- Deactivation does not leave a stuck pressed or hover row.
- The timer stops when all rows finish animating.
- Theme changes repaint immediately without blending old-theme colors into the
  new theme.
- Moving across DPI boundaries changes geometry and font size without making
  text smaller than the system-menu baseline.

Observability gate:

- Use `examples/msgflood` beside the CSD example to watch `WM_TIMER` traffic.
- Confirm timer messages appear only while a fade is active.
- Confirm no idle timer stream remains after mouse leave, command activation,
  deactivation, or destroy.

## Source Anchors

Implemented source points for this brief:

| Source | Role |
| --- | --- |
| `include/addon/csd/caption.inc:38` | `CSD_CAPTION_STATE` names `value`, `fadePhase`, `fromValue`, and `animFlags`. |
| `include/addon/csd/state.inc:10` | `CsdCaptionSetAllStates` initializes rows to a completed transition. |
| `include/addon/csd/state.inc:27` | `CsdCaptionStateApply` starts transitions when target state changes. |
| `include/addon/csd/state.inc:85` | `CsdCaptionAnimAnyActive` reports whether a timer is still needed. |
| `include/addon/csd/state.inc:105` | `CsdCaptionAnimAdvance` advances active rows and clears completed flags. |
| `include/addon/csd/theme.inc:16` | `CSD_THEME.generation` is the redraw/cache gate. |
| `11_caption_fade.asm:348` | `CsdBlendColorref` blends `COLORREF` channels directly. |
| `11_caption_fade.asm:466` | `DrawCaptionControls` resolves old/new state colors and uses `DC_BRUSH`. |
| `11_caption_fade.asm:937` | `HandleCaptionFadeTimer` advances animation and stops the timer when idle. |
| `11_caption_fade.asm:1159` | The `HTMAXBUTTON` NC bridge still starts fades for snap-hover state. |
