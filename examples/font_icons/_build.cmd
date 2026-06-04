@echo off
setlocal
pushd "%~dp0"

call "..\..\fasm2.cmd" -e 5 font_icon_demo.asm
set "BUILD_RC=%ERRORLEVEL%"
if not "%BUILD_RC%"=="0" goto done

call "..\..\fasm2.cmd" -e 5 uwpchar.asm
set "BUILD_RC=%ERRORLEVEL%"

:done
popd
endlocal & exit /b %BUILD_RC%
