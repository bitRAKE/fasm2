@echo off
setlocal
set ROOT=%~dp0..\..
set RUN=

if /i "%~1"=="clean" (
  del /q "%~dp0*.exe" 2>nul
  exit /b 0
)

if /i "%~1"=="run" set RUN=1
if /i "%~1"=="/run" set RUN=1

for %%F in ("%~dp0*.asm") do (
  echo [build] %%~nxF
  call "%ROOT%\fasm2.cmd" "%%~fF" "%~dp0%%~nF.exe"
  if errorlevel 1 exit /b 1
  if defined RUN (
    echo [run] %%~nF.exe
    "%~dp0%%~nF.exe"
    if errorlevel 1 exit /b 1
  )
)
