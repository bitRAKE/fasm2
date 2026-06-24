# RichEdit CTF Startup Proofing Case

## Status

Unresolved. Move on unless new evidence appears.

The `uah_menu` sample can configure RichEdit 4.1 for CTF/proofing and insert a
dense misspelling fixture at startup, but the visible proofing pass does not run
until the control receives a real key press. After that first key press,
proofing appears to be armed for the document, and later programmatic edits and
test-menu actions behave as expected.

This makes the startup failure mode look like a CTF/TSF document activation
problem, not a spelling-flag or fixture-content problem.

## Repro Shape

1. Launch `uah_menu`.
2. Do not type in the RichEdit control.
3. The startup fixture text is present, but spell-check squiggles do not appear.
4. Use the `Test` menu proofing trajectories. They do not force visible
   startup proofing before a key press.
5. Press any real key in the RichEdit control.
6. Proofing wakes up. After this point, the same test actions that failed before
   the key press tend to work.

## Known-Good Trigger

A real keyboard edit in the RichEdit control is the only confirmed trigger so
far. It appears to establish whatever CTF/TSF input session or proofing context
RichEdit needs before the proofer will sweep the document.

## Attempts That Did Not Fix Startup Proofing

- Inserting the sample paragraph during RichEdit creation.
- Moving sample insertion later in startup, after the main UI was shown and
  updated.
- Loading the dense proofing fixture through direct RichEdit text-store edits,
  including the current `EM_REPLACESEL` path.
- Clearing the control and reinserting the fixture.
- Toggling `IMF_SPELLCHECKING` off and back on with `EM_SETLANGOPTIONS`.
- Reapplying the language options while preserving the existing option mask.
- Adding a manual proofing menu item that only reapplies spell-checking flags.
- Sending a synthetic `WM_CHAR` space followed by `WM_CHAR VK_BACK`.
- Sending queued `SendInput` space/backspace.
- Delaying `SendInput` until after the menu command returned.
- Loading the fixture through keyboard-free `WM_PASTE` from `CF_UNICODETEXT`.
- Rebinding CTF with `EM_SETEDITSTYLE` by clearing and restoring `SES_USECTF`.
- Forcing foreground/focus with `ShowWindow`, `SetForegroundWindow`, and
  `SetFocus`.
- Using `AttachThreadInput` around foreground/focus setup.
- Calling `EM_SETCTFOPENSTATUS` after focusing the control.
- Combining attached-thread focus with Unicode `SendInput`.

## Current Interpretation

The failed cases rule out the simple models:

- It is not just "text arrived too early."
- It is not just "wrong text insertion API."
- It is not just "spell-checking language flags are missing."
- It is not just "CTF edit style was not set."
- It is not fixed by ordinary focus, attached focus, CTF open status, or
  injected keyboard input.

The remaining practical model is that RichEdit's proofing service is gated on a
real input session that our programmatic attempts do not establish. There is no
known documented RichEdit message that means "proof this document now."

## Historical Framing

The "it'd work if I owned Word" intuition is backwards and I think that matters
here. Word doesn't ride this path at all — it has its own text engine and its
own proofing stack painting its own underlines. The apps that historically
leaned on RichEdit drove proofing *client-side*, through the old TOM
temporary-formatting route (`tomApplyTmp`) and `EM_SETAUTOCORRECTPROC`; the
RichEdit team's own notes are explicit that accessing the spell/autocorrect
components was the client's responsibility for years. RichEdit 8 bolted the TSF
proofer on afterward as a convenience for small apps, and left its activation
welded to a live input session. So this isn't a raw-install deficiency or a
trick the pros know — the serious editors bypass this seam entirely. You didn't
fail to find the magic message; for this path there very likely isn't one.

## Practical Direction

Do not spend more effort trying to coerce RichEdit's built-in TSF proofer into
startup proofing. Keep it as an opportunistic convenience once the control has a
live input session.

If deterministic startup diagnostics matter, evaluate a client-side proofing
path instead:

- Run spelling/proofing outside RichEdit's TSF activation path.
- Mark ranges with TOM temporary formatting or another explicit decoration
  layer.
- Treat `EM_SETAUTOCORRECTPROC` and TOM-based approaches as the historically
  aligned RichEdit integration surface, not the built-in TSF proofer.
- Keep the current `Test` menu only as a repro harness for the startup case.

## Notes For Future Work

Do not spend more time on this path unless a new externally verified trigger is
found. Good future evidence would be a minimal native sample from another
project that wakes RichEdit proofing at startup without physical keyboard input,
or official documentation for a RichEdit/TSF proofing refresh API.

If this sample needs deterministic squiggles at startup, treat that as a
separate feature and evaluate alternatives outside RichEdit's built-in proofing
activation path.
