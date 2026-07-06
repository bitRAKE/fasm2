@echo off
rem NEWCOFF harness tests - see readme.md
rem   _build.cmd         assemble + link + run all tests, expect exit 97
rem   _build.cmd clean
setlocal
pushd "%~dp0"
set ROOT=%~dp0..\..

if /i "%~1"=="clean" (
  del /q *.obj *.exe *.map *.pdb 2>nul
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

rem --- smoke32: same backend, i386 via use32; links with no libraries --
rem --- at all (return from entry exits with eax) ------------------------
call "%ROOT%\fasm2.cmd" smoke32.asm smoke32_new.obj || goto :err
call "%ROOT%\fasm2.cmd" -i"SMOKE_LEGACY=1" smoke32.asm smoke32_legacy.obj || goto :err

%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /SAFESEH:NO /ENTRY:start32 /OUT:smoke32_new.exe smoke32_new.obj || goto :err
%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /SAFESEH:NO /ENTRY:start32 /OUT:smoke32_legacy.exe smoke32_legacy.obj || goto :err

.\smoke32_new.exe
if not "%errorlevel%"=="97" echo [FAIL] smoke32_new exit %errorlevel%, expected 97 & goto :err
.\smoke32_legacy.exe
if not "%errorlevel%"=="97" echo [FAIL] smoke32_legacy exit %errorlevel%, expected 97 & goto :err

rem --- fold: two objects define the same EXACT_MATCH COMDAT; each ------
rem --- references its own copy and both rebind to the survivor ---------
call "%ROOT%\fasm2.cmd" fold_a.asm fold_a.obj || goto :err
call "%ROOT%\fasm2.cmd" fold_b.asm fold_b.obj || goto :err

%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /MAP:fold.map /OUT:fold.exe fold_b.obj fold_a.obj kernel32.lib || goto :err

.\fold.exe
if not "%errorlevel%"=="97" echo [FAIL] fold exit %errorlevel%, expected 97 & goto :err

rem --- weak: b aliases 'maybe_get' weakly to a's real_get; c calls it --
call "%ROOT%\fasm2.cmd" weak_a.asm weak_a.obj || goto :err
call "%ROOT%\fasm2.cmd" weak_b.asm weak_b.obj || goto :err
call "%ROOT%\fasm2.cmd" weak_c.asm weak_c.obj || goto :err

%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /OUT:weak.exe weak_c.obj weak_b.obj weak_a.obj kernel32.lib || goto :err

.\weak.exe
if not "%errorlevel%"=="97" echo [FAIL] weak exit %errorlevel%, expected 97 & goto :err

rem --- cv: cvline markers become .debug$S; /DEBUG builds a PDB whose ---
rem --- line table maps the code back to cv.asm ---------------------------
call "%ROOT%\fasm2.cmd" cv.asm cv.obj || goto :err

%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /DEBUG:FULL /PDB:cv.pdb /OUT:cv.exe cv.obj kernel32.lib || goto :err

.\cv.exe
if not "%errorlevel%"=="97" echo [FAIL] cv exit %errorlevel%, expected 97 & goto :err

rem --- ovfl: 65600 relocations in one section (IMAGE_SCN_LNK_NRELOC_OVFL)
call "%ROOT%\fasm2.cmd" ovfl.asm ovfl.obj || goto :err

%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /OUT:ovfl.exe ovfl.obj kernel32.lib || goto :err

.\ovfl.exe
if not "%errorlevel%"=="97" echo [FAIL] ovfl exit %errorlevel%, expected 97 & goto :err

rem --- crc_vectors: byte-for-byte clang COMDAT sections; the checksums --
rem --- must match the values clang-cl wrote (see the source comments) ----
call "%ROOT%\fasm2.cmd" crc_vectors.asm crc_vectors.obj || goto :err

%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /ENTRY:mainCRTStartup /OUT:crc_vectors.exe crc_vectors.obj || goto :err

.\crc_vectors.exe
if not "%errorlevel%"=="42" echo [FAIL] crc_vectors exit %errorlevel%, expected 42 & goto :err

set "READOBJ="
where llvm-readobj >nul 2>nul && set "READOBJ=llvm-readobj"
if not defined READOBJ if exist "C:\Program Files\LLVM\bin\llvm-readobj.exe" set "READOBJ=C:\Program Files\LLVM\bin\llvm-readobj.exe"
if defined READOBJ (
  "%READOBJ%" --symbols crc_vectors.obj > crc_sym.txt
  for %%C in (0x7F8535B7 0xD940D5A4 0x186DBB85 0x26CD321D 0x1B85DBCF) do (
    findstr /C:"%%C" crc_sym.txt >nul || echo [FAIL] crc_vectors: clang checksum %%C missing && findstr /C:"%%C" crc_sym.txt >nul || goto :err
  )
  del crc_sym.txt
  echo [ok]   crc_vectors checksums match clang-cl
)

rem --- any: duplicate COMDAT ANY definitions, linker picks one ----------
call "%ROOT%\fasm2.cmd" optref_any_a.asm optref_any_a.obj || goto :err
call "%ROOT%\fasm2.cmd" optref_any_b.asm optref_any_b.obj || goto :err

%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /OUT:optref_any.exe optref_any_a.obj optref_any_b.obj || goto :err

.\optref_any.exe
if not "%errorlevel%"=="42" echo [FAIL] optref_any exit %errorlevel%, expected 42 & goto :err

echo [ok] newcoff: smoke x64 + x86, fold, weak, cv, ovfl, crc, any - all pass
popd & exit /b 0

:err
echo [error] newcoff tests failed
popd & exit /b 1
