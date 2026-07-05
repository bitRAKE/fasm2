@echo off
rem NEWCOFF harness tests - see readme.md
rem   _build.cmd         assemble + link + run all tests, expect exit 97
rem   _build.cmd clean
setlocal
pushd "%~dp0"
set ROOT=%~dp0..\..

if /i "%~1"=="clean" (
  del /q *.obj *.exe *.map 2>nul
  popd & exit /b 0
)

set "LK=link"
where link >nul 2>nul || set "LK=lld-link"
where %LK% >nul 2>nul || (
  echo [error] no linker on PATH - start from a VS dev prompt or add LLVM
  popd & exit /b 1
)

rem --- smoke: one object, both backends, every relocation kind ----------
call "%ROOT%\fasm2.cmd" smoke.asm smoke_new.obj || goto :err
call "%ROOT%\fasm2.cmd" -i"SMOKE_LEGACY=1" smoke.asm smoke_legacy.obj || goto :err

%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /OUT:smoke_new.exe smoke_new.obj kernel32.lib || goto :err
%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /OUT:smoke_legacy.exe smoke_legacy.obj kernel32.lib || goto :err

.\smoke_new.exe
if not "%errorlevel%"=="97" echo [FAIL] smoke_new exit %errorlevel%, expected 97 & goto :err
.\smoke_legacy.exe
if not "%errorlevel%"=="97" echo [FAIL] smoke_legacy exit %errorlevel%, expected 97 & goto :err

rem --- fold: two objects define the same EXACT_MATCH COMDAT; each ------
rem --- references its own copy and both rebind to the survivor ---------
call "%ROOT%\fasm2.cmd" fold_a.asm fold_a.obj || goto :err
call "%ROOT%\fasm2.cmd" fold_b.asm fold_b.obj || goto :err

%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /MAP:fold.map /OUT:fold.exe fold_b.obj fold_a.obj kernel32.lib || goto :err

.\fold.exe
if not "%errorlevel%"=="97" echo [FAIL] fold exit %errorlevel%, expected 97 & goto :err

echo [ok] newcoff: smoke (both backends) and exactmatch fold, exit 97
popd & exit /b 0

:err
echo [error] newcoff tests failed
popd & exit /b 1
