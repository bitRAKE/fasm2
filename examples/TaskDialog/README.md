# TaskDialog

This is a fasm2 Win64 port of bitRAKE's `fasmg-umbrella` TaskDialog samples.

The sample uses the branch's modular Win64 source model, the packed
`TASKDIALOGCONFIG` and `TASKDIALOG_BUTTON` equates from `include/equates`, and
an embedded common-controls v6 manifest so `TaskDialogIndirect` binds at load
time.

`Async Operation Sample` goes beyond the original set by using TaskDialog as a
modal workflow controller: command links select work, a timer callback simulates
an async worker, the progress bar switches from marquee to determinate mode,
Cancel returns `S_FALSE` and navigates after cleanup, and separate completion,
failure, and cancellation pages are driven with `TDM_NAVIGATE_PAGE`.

Build with:

```cmd
_build.cmd
```
