@echo off
setlocal enabledelayedexpansion
set ROOT=%~dp0..\..
pushd "%~dp0"

if /i "%~1"=="clean" (
  del /q *.obj *.exe *.map 2>nul
  popd & exit /b 0
)

rem ---- locate linkers -------------------------------------------------
set "LLD="
where lld-link >nul 2>nul && set "LLD=lld-link"
if not defined LLD if exist "C:\Program Files\LLVM\bin\lld-link.exe" set "LLD=C:\Program Files\LLVM\bin\lld-link.exe"
set "MSLINK="
where link >nul 2>nul && set "MSLINK=link"
if not defined LLD if not defined MSLINK (
  echo [skip] no linker found: start from a VS developer prompt and/or install LLVM
  popd & exit /b 1
)

set FAILED=0

rem ---- objects that must be REJECTED at assembly time -----------------
for %%F in (fail_*.asm) do (
  call "%ROOT%\fasm2.cmd" "%%~fF" "%%~nF.obj" >nul 2>nul
  if not errorlevel 1 (
    echo [FAIL] %%~nxF assembled, expected rejection
    set FAILED=1
  ) else (
    echo [ok]   %%~nxF rejected by assembler, as expected
  )
)

rem ---- assemble everything else ---------------------------------------
for %%F in (optref*.asm linkfail_*.asm crc_vectors.asm) do (
  call "%ROOT%\fasm2.cmd" "%%~fF" "%%~nF.obj" >nul || (
    echo [FAIL] %%~nxF did not assemble
    set FAILED=1
  )
)

rem ---- link + run under each available linker --------------------------
for %%L in (lld ms) do (
  set "LINKER="
  if "%%L"=="lld" if defined LLD set "LINKER=!LLD!"
  if "%%L"=="ms" if defined MSLINK set "LINKER=!MSLINK!"
  if defined LINKER (
    for %%T in (optref64 optref_assoc optref_static optref_nosym optref_bss optref_empty optref_pinned optref_exactmatch crc_vectors) do (
      call :run_42 %%L "!LINKER!" "%%T_%%L.exe" %%T.obj
    )
    call :run_42 %%L "!LINKER!" "optref_any_%%L.exe" optref_any_a.obj optref_any_b.obj
    call :run_42 %%L "!LINKER!" "optref_xmatch_%%L.exe" optref_xmatch_a.obj optref_xmatch_b.obj

    "!LINKER!" linkfail_xmatch_a.obj linkfail_xmatch_b.obj /nologo /OPT:REF /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup /OUT:linkfail_xmatch_%%L.exe >nul 2>nul
    if not errorlevel 1 (
      echo [FAIL] %%L: mismatched EXACT_MATCH COMDAT linked, expected error
      set FAILED=1
    ) else (
      echo [ok]   %%L: mismatched EXACT_MATCH COMDAT rejected, as expected
    )

    "!LINKER!" optref32.obj /nologo /OPT:REF /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup /MACHINE:X86 /OUT:optref32_%%L.exe >nul 2>nul
    if errorlevel 1 (
      echo [FAIL] %%L: optref32 did not link
      set FAILED=1
    ) else (
      .\optref32_%%L.exe
      if !errorlevel! neq 42 (echo [FAIL] %%L: optref32 exit !errorlevel!, expected 42 & set FAILED=1) else echo [ok]   %%L: optref32 exit 42
    )

    "!LINKER!" linkfail_dup_a.obj linkfail_dup_b.obj /nologo /OPT:REF /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup /OUT:linkfail_dup_%%L.exe >nul 2>nul
    if not errorlevel 1 (
      echo [FAIL] %%L: duplicate NODUPLICATES COMDAT linked, expected error
      set FAILED=1
    ) else (
      echo [ok]   %%L: duplicate NODUPLICATES COMDAT rejected, as expected
    )
  )
)

if !FAILED! neq 0 (echo === FAILURES === & popd & exit /b 1)
echo === all COFF tests passed ===
popd & exit /b 0

:run_42
rem %1 tag, %2 linker, %3 out exe, %4.. objects
set "TAG=%~1"
set "LNK=%~2"
set "EXE=%~3"
shift & shift & shift
set "OBJS="
:collect
if "%~1"=="" goto linkit
set OBJS=!OBJS! %1
shift
goto collect
:linkit
"%LNK%"%OBJS% /nologo /OPT:REF /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup /OUT:%EXE% >nul 2>nul
if errorlevel 1 (
  echo [FAIL] %TAG%: %EXE% did not link
  set FAILED=1
  goto :eof
)
.\%EXE%
if !errorlevel! neq 42 (
  echo [FAIL] %TAG%: %EXE% exit !errorlevel!, expected 42
  set FAILED=1
) else (
  echo [ok]   %TAG%: %EXE% exit 42
)
goto :eof
