@echo off
setlocal
pushd "%~dp0"

set "RC_EXE=rc.exe"

where rc.exe >nul 2>nul
if errorlevel 1 set "RC_EXE="

if not defined RC_EXE (
	for /f "delims=" %%V in ('dir /b /ad /o-n "%ProgramFiles(x86)%\Windows Kits\10\bin\10.*" 2^>nul') do if not defined RC_EXE if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\%%V\x64\rc.exe" set "RC_EXE=%ProgramFiles(x86)%\Windows Kits\10\bin\%%V\x64\rc.exe"
)

if not defined RC_EXE (
	echo error: rc.exe not found. Install the Windows SDK or run this build from a Visual Studio developer prompt.
	set "BUILD_RC=1"
	goto done
)

if /i "%~1"=="regen" (
	python make_uwpchar_data.py --out uwpchar_data.inc
	if errorlevel 1 (
		set "BUILD_RC=1"
		goto done
	)
)

set "SDKVER="
for /f "delims=" %%I in ('dir /b /ad /o-n "%ProgramFiles(x86)%\Windows Kits\10\Include\10.*" 2^>nul') do if not defined SDKVER set "SDKVER=%%I"
set "SDKINC="
if defined SDKVER set "SDKINC=/I "%ProgramFiles(x86)%\Windows Kits\10\Include\%SDKVER%\shared" /I "%ProgramFiles(x86)%\Windows Kits\10\Include\%SDKVER%\um" /I "%ProgramFiles(x86)%\Windows Kits\10\Include\%SDKVER%\ucrt""

for %%S in (font_icon_demo.asm uwpchar.asm glyphset.asm) do (
	"%RC_EXE%" /nologo %SDKINC% /fo "%%S.res" "%%S.rc"
	if errorlevel 1 (
		set "BUILD_RC=1"
		goto done
	)
)

for %%S in (font_icon_demo.asm uwpchar.asm glyphset.asm) do (
	call "..\..\fasm2.cmd" -e 5 %%S
	if errorlevel 1 (
		set "BUILD_RC=1"
		goto done
	)
)
set "BUILD_RC=0"

:done
popd
endlocal & exit /b %BUILD_RC%
