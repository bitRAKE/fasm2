@echo off
rem hexer example build - see README.md
rem   _build.cmd [base|avx2|avx512|dispatch]   (default: dispatch)
rem   _build.cmd clean
setlocal enabledelayedexpansion
pushd "%~dp0"
set "INCLUDE=%~dp0.."
set ROOT=%~dp0..\..\..
set FASM2=call "%ROOT%\fasm2.cmd" -iNEWCOFF.DEBUG:=1
set MODE=%~1
if "%MODE%"=="" set MODE=dispatch

if /i "%MODE%"=="clean" (
  del /q *.obj *.exe *.map 2>nul
  popd & exit /b 0
)

rem --- locate a linker: prefer MSVC link, else lld-link -----------------
set "LK=link"
where link >nul 2>nul || set "LK=lld-link"
where %LK% >nul 2>nul || (
  echo [error] no linker on PATH - start from a VS dev prompt or add LLVM
  popd & exit /b 1
)

rem --- both libraries are always built and linked; their identical -----
rem --- hextab folds through EXACT_MATCH -------------------------------
%FASM2% somehex.asm || goto :err
%FASM2% u8_as_hex_avx512.asm || goto :err

set OBJS=hexer.obj somehex.obj u8_as_hex_avx512.obj

if /i "%MODE%"=="dispatch" (
  rem dispatch build: HEXER_ISA undefined, so u8_as_hex re-routes on first use
  %FASM2% dispatch.asm || goto :err
  %FASM2% hexer.asm || goto :err
  set OBJS=!OBJS! dispatch.obj
) else (
  if /i not "%MODE%"=="base" if /i not "%MODE%"=="avx2" if /i not "%MODE%"=="avx512" (
    echo [error] unknown mode "%MODE%" - use base, avx2, avx512, or dispatch
    goto :err
  )
  rem single-ISA build: the string picks u8_as_hex_<mode> as u8_as_hex, so the
  rem other variants go unreferenced and /OPT:REF drops them
  %FASM2% -i"HEXER_ISA='%MODE%'" hexer.asm || goto :err
)

echo [link] %MODE%: !OBJS!
%LK% @hexer.response !OBJS! || goto :err
echo [done] hexer.exe built for "%MODE%" - see hexer.map for kept/discarded sections
popd & exit /b 0

:err
echo [error] build failed
popd & exit /b 1
