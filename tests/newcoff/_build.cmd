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

rem --- cv: byte-backed/coalesced lines + local/const/data .debug$S -----
rem --- PDB maps code to cv.asm and contributing cv_lines.inc ------------
call "%ROOT%\fasm2.cmd" cv.asm cv.obj || goto :err

%LK% /NOLOGO /SUBSYSTEM:CONSOLE /OPT:REF /NODEFAULTLIB /DEBUG:FULL /PDB:cv.pdb /OUT:cv.exe cv.obj kernel32.lib || goto :err

.\cv.exe
if not "%errorlevel%"=="97" echo [FAIL] cv exit %errorlevel%, expected 97 & goto :err

set "PDBUTIL="
where llvm-pdbutil >nul 2>nul && set "PDBUTIL=llvm-pdbutil"
if not defined PDBUTIL if exist "C:\Program Files\LLVM\bin\llvm-pdbutil.exe" set "PDBUTIL=C:\Program Files\LLVM\bin\llvm-pdbutil.exe"
if defined PDBUTIL (
  "%PDBUTIL%" dump -symbols cv.pdb > cv_symbols.txt
  "%PDBUTIL%" dump -globals cv.pdb > cv_globals.txt
  "%PDBUTIL%" dump -l cv.pdb > cv_lines.txt
  findstr /C:"S_REGREL32" cv_symbols.txt >nul || echo [FAIL] cv: S_REGREL32 records missing && goto :err
  findstr /R /C:"`seed`" cv_symbols.txt >nul || echo [FAIL] cv: PROC parameter seed missing from PDB && goto :err
  findstr /C:"saved_seed" cv_symbols.txt >nul || echo [FAIL] cv: LOCALS symbol saved_seed missing from PDB && goto :err
  findstr /C:"exit_code" cv_symbols.txt >nul || echo [FAIL] cv: LOCALS symbol exit_code missing from PDB && goto :err
  findstr /C:"S_CONSTANT" cv_globals.txt >nul || echo [FAIL] cv: S_CONSTANT records missing && goto :err
  findstr /C:"CV_EXIT_CODE" cv_globals.txt >nul || echo [FAIL] cv: constant CV_EXIT_CODE missing from PDB && goto :err
  findstr /C:"S_GDATA32" cv_globals.txt >nul || echo [FAIL] cv: S_GDATA32 records missing && goto :err
  findstr /C:"cv_global_exit_code" cv_globals.txt >nul || echo [FAIL] cv: global data symbol missing from PDB && goto :err
  findstr /C:"S_LDATA32" cv_symbols.txt >nul || echo [FAIL] cv: S_LDATA32 records missing && goto :err
  findstr /C:"cv_local_delta" cv_symbols.txt >nul || echo [FAIL] cv: local data symbol missing from PDB && goto :err
  powershell -NoProfile -ExecutionPolicy Bypass -File check_cv_lines.ps1 cv_lines.txt || goto :err
  del cv_symbols.txt cv_globals.txt cv_lines.txt
  echo [ok]   cv locals, constants and data symbols harvested into CodeView
)

rem Native frame regressions require Python and MSVC from an x64 dev prompt.
python check_proc_frames.py || goto :err

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
