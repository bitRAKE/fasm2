@echo off
setlocal
pushd "%~dp0"

call "..\..\fasm2.cmd" -e 5 modern.asm
set "BUILD_RC=%ERRORLEVEL%"

popd
endlocal & exit /b %BUILD_RC%
