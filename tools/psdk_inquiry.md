# PSDK Inquiry Tools

These small tools answer one SDK question at a time and emit fasm2-oriented
snippets. They reuse `win32_scraper.py` for Windows SDK discovery, LLVM
discovery, clang AST queries, and C type mapping.

## Function Inquiry

```powershell
python tools\psdk_function.py TaskDialogIndirect --dll comctl32
python tools\psdk_function.py MessageBox --dll user32
python tools\psdk_function.py InitCommonControlsEx --header commctrl.h
```

The output includes the resolved declaration, import library, fasm2 `import`
entry, parameter count entry, and dependent structure/enum snippets.

To stage the output into a project-local include file:

```powershell
python tools\psdk_function.py TaskDialogIndirect --dll comctl32 --append-to C:\path\to\windows.inc
python tools\psdk_function.py TaskDialogIndirect --dll comctl32 --append-to C:\path\to\windows.inc --write
```

Without `--write`, `--append-to` is a preview.

## Structure Inquiry

```powershell
python tools\psdk_struct.py TASKDIALOGCONFIG --header commctrl.h
python tools\psdk_struct.py INITCOMMONCONTROLSEX --dll comctl32 --arch 64
```

The structure tool asks clang for the typedef/tag and record layout, then emits
32-bit and/or 64-bit fasm2 `struct` definitions with SDK byte offsets.

## COM Interface Inquiry

```powershell
python tools\psdk_interface.py IAutoComplete2
python tools\psdk_interface.py IFileOpenDialog --write
```

The interface tool searches SDK IDL files, parses the interface inheritance
chain, emits IID bytes, method signature comments, inherited vtable method
names, and nested enum constants. With `--write`, it creates
`include\com\<interface>.inc`; existing files are not overwritten unless
`--force` is supplied.

