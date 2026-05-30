@echo off
setlocal
pushd "%~dp0"

call "..\..\fasm2.cmd" -e 5 conio.asm
link @conio.response
pause
