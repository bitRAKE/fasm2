
### How does WM_ERASEBKGND relate to WM_PAINT?

The thing that needs clearing is the window's *update region*, which lives at the USER/GDI layer. DWM is downstream of that: it composites whatever sits in your window's redirection surface. The update region is the OS's "this is dirty, keep nagging" bookkeeping, and `WM_PAINT` is a synthesized (not queued) message the system regenerates whenever the queue drains and that region is non-empty. So *something* has to validate the region or you get an endless `WM_PAINT` stream.

Two facts settle the question:

**`WM_ERASEBKGND` does not validate the update region.** Erasing is a sub-step of the paint cycle, not the cycle itself. You can fully draw your window inside it and return TRUE, and you'll still get `WM_PAINT` — and if that handler doesn't validate, you spin. The HDC in `wParam` is convenient, but it's the same DC `BeginPaint` hands you, clipped to the same region. It buys you nothing over `BeginPaint` except skipping two calls, at the cost of not clearing the region. So it can't replace `WM_PAINT`.

**Yes, you can just validate.** If your pixels reach the screen by another path (DComp/DX swap chain, or you draw via `GetDC` outside the cycle), the update region is just noise to silence:

```
; WM_PAINT
    invoke ValidateRect, [hwnd], 0   ; NULL = entire client
    xor   eax, eax                   ; 0 = message handled
    ret
```

`ValidateRgn(hwnd, NULL)` is the regional equivalent. Letting `DefWindowProc` take `WM_PAINT` also validates (it runs `BeginPaint`/`EndPaint` internally), but it's heavier and fires the class-brush erase — `ValidateRect` is the cheapest satisfy-and-bail. One guardrail once you go validate-only: don't also `BeginPaint` somewhere else, or that cycle's region-clipped DC will clip/overwrite the frame you presented elsewhere.

The catch with validate-only is that it stops the messages but doesn't *present* anything. If DWM is showing black or stale content, it's because nothing has put pixels in the redirection surface yet — validating won't fix that. Validate-only is correct precisely when your pixels already arrive through another channel.

Where `WM_ERASEBKGND` genuinely is the right tool: custom background fill, especially behind child controls, since the erase runs before children paint and the DC is clipped correctly. Return TRUE to skip the default class-brush fill. But that's the background step only — the update region still gets validated by your normal `WM_PAINT`/`EndPaint` path. And note the standard flicker-free idiom is the *inverse* of what you're reaching for: return TRUE from `WM_ERASEBKGND` *without drawing* (suppress the erase), then paint every pixel in `WM_PAINT`.

So, concretely: if you're GDI-drawing your content, keep it in `WM_PAINT` and use `WM_ERASEBKGND`→TRUE only to kill flicker. If your content comes from DXGI/DComp/D2D, validate in `WM_PAINT` and you're done — `WM_ERASEBKGND` then only matters insofar as you want to stop GDI erasing over your composited frame, which a NULL class brush handles just as well.
