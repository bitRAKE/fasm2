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

set "SDKVER="
for /f "delims=" %%I in ('dir /b /ad /o-n "%ProgramFiles(x86)%\Windows Kits\10\Include\10.*" 2^>nul') do if not defined SDKVER set "SDKVER=%%I"
set "SDKINC="
if defined SDKVER set "SDKINC=/I "%ProgramFiles(x86)%\Windows Kits\10\Include\%SDKVER%\shared" /I "%ProgramFiles(x86)%\Windows Kits\10\Include\%SDKVER%\um" /I "%ProgramFiles(x86)%\Windows Kits\10\Include\%SDKVER%\ucrt""

for %%R in (05_dpi_frame.rc 06_dwm_theme.rc 07_caption_state.rc 08_snap_layouts.rc 09_embed_edit.rc 10_a11y_keyboard.rc 11_caption_fade.rc 12_backdrop.rc 13_caption_tabs.rc 14_responsive_caption.rc 15_rtl_caption.rc 16_multi_window.rc 17_custom_shadow.rc 19_backdrop_viewer.rc) do (
	"%RC_EXE%" /nologo %SDKINC% /fo "%%~nR.res" "%%R"
	if errorlevel 1 (
		set "BUILD_RC=1"
		goto done
	)
)

for %%S in (01_nccalc_frame.asm 02_nchittest_frame.asm 03_dwm_frame.asm 04_caption_controls.asm 05_dpi_frame.asm 06_dwm_theme.asm 07_caption_state.asm 08_snap_layouts.asm 09_embed_edit.asm 10_a11y_keyboard.asm 11_caption_fade.asm 12_backdrop.asm 13_caption_tabs.asm 14_responsive_caption.asm 15_rtl_caption.asm 16_multi_window.asm 17_custom_shadow.asm 19_backdrop_viewer.asm) do (
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
