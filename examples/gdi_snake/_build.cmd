@echo off
setlocal
pushd "%~dp0"

call "..\..\fasm2.cmd" -e 5 gdi_snake.asm
set "BUILD_RC=%ERRORLEVEL%"

popd
endlocal & exit /b %BUILD_RC%
