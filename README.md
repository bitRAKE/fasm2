> [!WARNING]
> This software is work in progress. Keep your expectations low.

Building on [flat assembler 2](https://github.com/tgrysztar/fasm2) with a focus on 64-bit Windows.
 - expanded API support
 - extensive examples
 - PSDK scrapping tools
 - coding guides


Typical Updating:
```cmd
git fetch origin
git rebase origin/master
git push --force-with-lease
```

## Repository-local assembler wrappers

Use the wrapper for your host so the repository `include` directory is placed on
fasmg's include search path before assembling fasm2 sources:

```cmd
fasm2.cmd -e 5 examples\globstr\demo_windows.asm
```

```sh
./fasm2.sh -e 5 examples/globstr/demo_linux.asm /tmp/demo_linux
```

The Windows wrapper calls `fasmg.exe`; the POSIX wrapper calls the checked-in
Linux `fasmg.x64` binary. Both inject `Include('fasm2.inc')` so examples and
local tests can use the fasm2 macro layer without repeating the bootstrap line.

---

# flat assembler 2

This project combines flat assembler g with a set of headers that implement an x86 assembler. It is largely compatible with fasm 1, except for the macroinstruction syntax.
