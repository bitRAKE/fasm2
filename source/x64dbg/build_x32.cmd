@echo off
setlocal

for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
set "OUT=%ROOT%\build\x64dbg-plugin\x32"
set "FASM2_INCLUDE=%~1"
if not defined FASM2_INCLUDE set "FASM2_INCLUDE=%ROOT%\include"
set "FASM2_INCLUDE=%FASM2_INCLUDE:\=/%"

if not exist "%OUT%" mkdir "%OUT%"

call "%ROOT%\fasm2.cmd" "%~dp0plugin.asm" "%OUT%\fasm2.dp32"
if errorlevel 1 exit /b %errorlevel%

if exist "%OUT%\fasmg.dll" del /q "%OUT%\fasmg.dll"

> "%OUT%\fasm2.ini" (
    echo [fasm2]
    echo IncludeDirectory=%FASM2_INCLUDE%
    echo StartupSource=
    echo MaximumPasses=100
)

call "%ROOT%\fasm2.cmd" "%~dp0plugin_test.asm" "%OUT%\plugin_test.exe"
if errorlevel 1 exit /b %errorlevel%

pushd "%OUT%"
plugin_test.exe
set "TEST_RESULT=%ERRORLEVEL%"
popd
if not "%TEST_RESULT%"=="0" (
    echo fasm2 x64dbg plugin test failed with exit code %TEST_RESULT%.
    exit /b %TEST_RESULT%
)

echo fasm2 x64dbg assembly plugin tests passed.
echo Built %OUT%\fasm2.dp32
echo Include directory: %FASM2_INCLUDE%
