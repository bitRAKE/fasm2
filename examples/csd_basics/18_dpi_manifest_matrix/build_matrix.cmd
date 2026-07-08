@echo off
setlocal
pushd "%~dp0"

set "BUILD_RC=0"
set "RC_EXE=rc.exe"
set "MT_EXE=mt.exe"
set "DUMPBIN_EXE=dumpbin.exe"

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

if not exist generated mkdir generated
if not exist reports mkdir reports
if not exist extracted mkdir extracted

pushd generated
for %%R in (18_unaware.rc 18_system.rc 18_pmv1.rc 18_pmv2.rc) do (
	"%RC_EXE%" /nologo %SDKINC% /fo "%%~nR.res" "%%R"
	if errorlevel 1 (
		popd
		set "BUILD_RC=1"
		goto done
	)
)
popd

for %%S in (18_unaware.asm 18_system.asm 18_pmv1.asm 18_pmv2.asm) do (
	call "..\..\..\fasm2.cmd" -e 5 "generated\%%S"
	if errorlevel 1 (
		set "BUILD_RC=1"
		goto done
	)
)

where mt.exe >nul 2>nul
if errorlevel 1 set "MT_EXE="

if not defined MT_EXE (
	for /f "delims=" %%V in ('dir /b /ad /o-n "%ProgramFiles(x86)%\Windows Kits\10\bin\10.*" 2^>nul') do if not defined MT_EXE if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\%%V\x64\mt.exe" set "MT_EXE=%ProgramFiles(x86)%\Windows Kits\10\bin\%%V\x64\mt.exe"
)

if defined MT_EXE (
	for %%V in (unaware system pmv1 pmv2) do (
		"%MT_EXE%" -nologo -inputresource:"generated\18_%%V.exe";#1 -out:"extracted\18_%%V.manifest"
		if errorlevel 1 echo warning: mt.exe could not extract manifest for %%V
	)
) else (
	echo warning: mt.exe not found; report will use source manifests.
)

where dumpbin.exe >nul 2>nul
if errorlevel 1 set "DUMPBIN_EXE="

if defined DUMPBIN_EXE (
	for %%V in (unaware system pmv1 pmv2) do (
		"%DUMPBIN_EXE%" /headers "generated\18_%%V.exe" > "extracted\18_%%V.dumpbin.txt"
		if errorlevel 1 echo warning: dumpbin.exe could not inspect %%V
	)
) else (
	echo warning: dumpbin.exe not found; PE-header inspection skipped.
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "write_report.ps1"
if errorlevel 1 (
	set "BUILD_RC=1"
	goto done
)

set "BUILD_RC=0"

:done
popd
endlocal & exit /b %BUILD_RC%
