# GDI Snake fasm2 Win64 Example

`examples/gdi_snake` is a borderless popup Snake game built with plain Win32
and GDI. It is intentionally presented as a game, but the teaching target is
the separation between simulation, board rendering, and overlay UI.

Most assembly examples in this repository are single source files. This one is
modular because the parts are useful beyond Snake:

- `board_gdi.inc` is a reusable off-screen bitmap and stretch-blit board
  renderer.
- `game_logic.inc` is a reusable grid simulation shape: cells, entity state,
  queued bonuses, ticks, and simple AI.
- `high_scores.inc` is a reusable arcade table shape: ranked entries,
  tamper-evident storage, score rendering, and a tiny entry window.
- `ui_main.inc` is a reusable borderless-window message adapter.
- `windows.inc` is the local executable policy layer.

These modules are meant to become familiar puzzle pieces. The procedures still
show the real Win32 calls, but the reader can carry the module semantics into
future examples and refine them instead of rebuilding every program as one
large file.

## Build

Run from a Visual Studio developer prompt:

```cmd
_build.cmd
```

The script calls the repository-local `..\..\fasm2.cmd`.

## Controls

- Arrow keys or `W`, `A`, `S`, `D`: turn the snake.
- `Space`: activate the next queued bonus.
- Any key: start from attract mode or game over.
- `Esc`: exit.

## Window And Difficulty

The main window is a borderless `WS_POPUP`. Before the game starts, custom
`WM_NCHITTEST` results make the edges resizeable and the body draggable. The
initial window size is an exact multiple of the preview cell size, and
`WM_SIZING` snaps resize rectangles to that same grid. The preview board is
then recalculated from the client size:

```text
board_cols = clamp(client_width  / 24, 12, 80)
board_rows = clamp(client_height / 24, 8,  60)
```

When the player presses a key to start, `board_locked` is set and the current
grid dimensions become the game board for the duration of that game. Smaller
windows create tighter boards and shorter tick intervals; larger windows give
more room and a slower start.

The resize snapping matters because the board bitmap is stretch-blitted. If the
destination width or height has leftover pixels after dividing by the logical
cell count, some cells receive extra pixels and the board looks irregular. The
unlocked window therefore grows and shrinks in cell-sized steps.

After game over, the board unlocks so the next game can use a different window
size.

## Rendering Contract

The visual model is deliberately unusual for a teaching example:

- `WM_ERASEBKGND` returns handled so Windows does not clear the client area
  between frames.
- `WM_PAINT` composes the current off-screen board bitmap plus overlay text into
  a temporary frame buffer, then blits the finished frame to the window.

Attract mode and game over draw a centered message without the score/lives
overlay. Active play draws score, lives, board size, queued-bonus count, and
transient messages. This avoids having the start message collide with state text
on small boards and makes it disappear completely once play starts.

The startup attract mode alternates between the title/message page and the
high-score table while the demo snake keeps playing. Game over starts on the
high-score table, then uses the same 20-second alternation for a more
arcade-like loop.

This keeps the board renderer and overlay renderer independent while avoiding
visible erase/paint flicker. The simulation can redraw the board bitmap whenever
state changes, and window painting only has to compose the current board plus
text.

## Runtime State

The main state lives in `app_state.inc`:

- Board dimensions, lock flag, board cells, memory DC, bitmap, and brushes.
- Snake body as a linear cell array.
- Apple cell/type and bonus queue.
- Score, lives, level, tick speed, message timer, and RNG state.
- High-score entries, score fonts, score-page timing, and entry-window handles.

Snake cells are packed as linear board positions:

```text
cell = y * board_cols + x
```

The body array stores the head at index zero. On each step the array shifts
toward the tail. This is not the fastest possible Snake representation, but it
is easy to inspect and is appropriate for a Win32/GDI teaching example.

## Game Mechanics

Attract mode runs the same simulation with a small AI:

1. Prefer a direction that moves toward the apple.
2. Reject immediate reversal.
3. Reject moves that collide with a wall or the body.
4. Fall back to the first safe direction.

The player starts a new locked game by pressing any key.

Apple colors map to effects:

- Red: score and grow by one.
- Gold: score and grow more.
- Purple: queue invincibility.
- Blue: queue teleport.
- Cyan: queue split.

Queued bonuses are activated with `Space`. Invincibility wraps through walls
for a short time and cuts through self-collisions. Teleport rebuilds the snake
at a random row. Split trims the tail half of the snake.

## High Scores

`high_scores.inc` owns the arcade table. The top eight entries are stored as
three-character names plus scores. Rendering uses several GDI fonts so the
ranking has weight: the first score is largest, the second and third step down,
and the remaining entries use a compact table font. The visual ratio is meant
to read like `5:3:2:1...` rather than a plain list.

When the final life is lost, `FinishGameWithHighScores` switches the game into
`STATE_SCORES`, unlocks the board size, shows the high-score page first, and
checks whether the current score belongs in the table. If it does, a small
owned popup asks for initials. Pressing OK inserts the typed initials; pressing
Skip records `YOU`.

The table is stored under `%LOCALAPPDATA%\Fasm2Examples\gdi_snake.hsc`. The
file is deliberately not plain text: entries are packed, XOR-obfuscated with a
rolling key, and checked with a signature over the decoded table. This is not
cryptographic security, but it prevents casual editing. If the file is missing
or fails validation, the table is reset to built-in defaults and rewritten.

## Message Map

The main window procedure is intentionally compact:

```text
WM_CREATE       create brushes, initialize preview board, start timer
WM_SIZE         update preview board only while unlocked
WM_SIZING       snap unlocked resize rectangles to cell-size increments
WM_NCHITTEST    provide borderless resize/move before play
WM_KEYDOWN      start, steer, activate bonus, or exit
WM_TIMER        advance simulation and invalidate
WM_ERASEBKGND   suppress background erase
WM_PAINT        double-buffer board bitmap plus overlay text
WM_COMMAND      handle high-score entry buttons in the score popup
WM_DESTROY      release GDI objects and quit
```

The important lesson is that the UI is complex, but mostly hidden. The
presentation surface is just a board and an overlay; the rest is ordinary
message routing into reusable runtime modules.
