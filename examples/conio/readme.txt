
"_build.cmd" expects a MS tools x64 build environment and fasm2 installed at "C:\fasm2"

There are many ways to get the build tools ...

winget install Microsoft.VisualStudio.2022.BuildTools

https://visualstudio.microsoft.com/visual-cpp-build-tools/

Visual Studio 2017 Build Tools: https://aka.ms/vs/15/release/vs_buildtools.exe
Visual Studio 2019 Build Tools: https://aka.ms/vs/16/release/vs_buildtools.exe
Visual Studio 2022 Build Tools: https://aka.ms/vs/17/release/vs_buildtools.exe




; Synchronized Update Mode - Terminal buffers everything between the two and presents it in one shot, so a multi-sequence redraw never shows a half-drawn frame. This is the modern answer to the save-cursor/park-and-restore dance for tear-free updates. Windows Terminal supports it; legacy conhost ignores it harmlessly (degrades to normal drawing).
	<| 27,'[?2026h' |> ;  begin
	<| 27,'[?2026l' |> ;  end



struc ECH cells ; Blank a number of cells from cursor, fixed field reset.
repeat 1,C:cells
	. equ 27,'[',`C,'X'
end repeat
end struc







