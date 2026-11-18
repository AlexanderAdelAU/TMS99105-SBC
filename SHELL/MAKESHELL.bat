@ECHO OFF
REM ====================================================================
REM  MAKESHELL.BAT - build the Shell 6.5 system volume in one run
REM
REM  Project layout:
REM    SRC\          SHELLV65.A99 and command .A99 files
REM    Includes\     DECLS.INC and reference/generated include files
REM    ..\Toolchain\  Shared A99.EXE, MKCINC.EXE, SYSGEN.EXE
REM    ..\SYSGEN\    Source for the system image packer
REM    SystemFiles\  Known-good .H99 inputs for standalone Shell builds
REM    Build\        disposable working copies and build outputs
REM
REM  A99 reads/writes C:<name>.* and source files use INCL
REM  "C:DECLS.INC" and "C:CORE.INC".  We therefore copy the required
REM  files into Build\ and run all build steps from that directory.
REM  Source files and original tools are not modified.
REM  Optional argument 1: folder containing freshly built system .H99
REM  files.  Without it, the checked-in SystemFiles\ copies are used.
REM
REM  Requires the project to be on drive C: and MinGW gcc on PATH.
REM  Steps: prepare Build, compile SYSGEN, assemble core, generate
REM  CORE.INC, assemble ten pages, pack PAYLOAD65.HEX, publish to the
REM  MONITOR directory alongside the project folder.
REM  A failed step must never publish the payload.
REM ====================================================================
SETLOCAL EnableExtensions
CD /D "%~dp0" || EXIT /B 1

SET "ROOT=%CD%"
SET "WORK=%ROOT%\Build"
SET "DEST=%ROOT%\..\MONITOR"
SET "TOOLCHAIN=%ROOT%\..\Toolchain"
SET "SYSGEN_SRC=%ROOT%\..\SYSGEN\sysgen.c"
SET "SYSTEM_FILES=%ROOT%\SystemFiles"
IF NOT "%~1"=="" SET "SYSTEM_FILES=%~1"
SET "OUT=PAYLOAD65.HEX"
SET "PAGES=DIR SAVE ERA REN MKDIR CHDIR RMDIR LOAD TYPE COPY"
SET "FAIL="
SET "IN_BUILD="

IF /I NOT "%~d0"=="C:" (
    ECHO *** Project must be on drive C: because A99 uses C: paths.
    GOTO FAILED
)

REM --- Check master inputs, never rely on older working copies -------
FOR %%F IN (a99.exe mkcinc.exe) DO IF NOT EXIST "%TOOLCHAIN%\%%F" (ECHO *** Shared Toolchain\%%F missing & SET "FAIL=1")
IF NOT EXIST "%SYSGEN_SRC%" (ECHO *** SYSGEN\sysgen.c missing & SET "FAIL=1")
FOR %%F IN (DECLS.INC) DO IF NOT EXIST "%ROOT%\Includes\%%F" (ECHO *** Includes\%%F missing & SET "FAIL=1")
FOR %%F IN (DSKINIT62.H99 BDOS61.H99 XMODEM59.H99 DIR2.H99) DO IF NOT EXIST "%SYSTEM_FILES%\%%F" (ECHO *** System input %%F missing & SET "FAIL=1")
IF NOT EXIST "%ROOT%\SRC\SHELLV65.A99" (ECHO *** SRC\SHELLV65.A99 missing & SET "FAIL=1")
FOR %%P IN (%PAGES%) DO IF NOT EXIST "%ROOT%\SRC\%%P.A99" (ECHO *** SRC\%%P.A99 missing & SET "FAIL=1")
IF DEFINED FAIL GOTO FAILED

REM --- Populate Build with working copies of every required input ----
IF NOT EXIST "%WORK%\" (
    MKDIR "%WORK%"
    IF ERRORLEVEL 1 GOTO FAILED
)
ECHO --- prepare Build
COPY /Y "%ROOT%\SRC\*.A99" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%SYSGEN_SRC%" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%ROOT%\Includes\*.INC" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%TOOLCHAIN%\*.EXE" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%SYSTEM_FILES%\*.H99" "%WORK%\" > NUL || GOTO FAILED

PUSHD "%WORK%" || GOTO FAILED
SET "IN_BUILD=1"

REM --- 1. host tool (MinGW); A99 and MKCINC were copied prebuilt -----
ECHO --- tools
gcc -O2 -o SYSGEN.EXE sysgen.c || GOTO FAILED

REM --- 2. the core - also writes SHELLV65.L99 for MKCINC ------------
ECHO --- core
CALL :ASM SHELLV65
IF DEFINED FAIL GOTO FAILED

REM --- 3. CORE.INC: core addresses not already in DECLS.INC ----------
ECHO --- CORE.INC
IF EXIST CORE.INC DEL CORE.INC
IF EXIST CORE.INC GOTO FAILED
MKCINC SHELLV65.L99 DECLS.INC CORE.INC || GOTO FAILED
IF NOT EXIST CORE.INC GOTO FAILED

REM --- 4. ten command pages (AORG >D000, DECLS.INC + CORE.INC) ------
ECHO --- command pages
FOR %%P IN (%PAGES%) DO CALL :ASM %%P
IF DEFINED FAIL GOTO FAILED

REM --- 5. pack the volume -------------------------------------------
REM  SHELL.SYS and BDOS.SYS MUST stay entries 0 and 1 (boot loader finds
REM  them by position).  The .SHC pages are loaded by name at cold start.
ECHO --- SYSGEN
IF EXIST "%OUT%" DEL "%OUT%"
IF EXIST "%OUT%" GOTO FAILED
SYSGEN %OUT% DSKINIT62.H99 SHELL.SYS=SHELLV65.H99 BDOS.SYS=BDOS61.H99 ^
  DIR.SHC=DIR.H99 SAVE.SHC=SAVE.H99 ERA.SHC=ERA.H99 REN.SHC=REN.H99 ^
  MKDIR.SHC=MKDIR.H99 CHDIR.SHC=CHDIR.H99 RMDIR.SHC=RMDIR.H99 ^
  LOAD.SHC=LOAD.H99 TYPE.SHC=TYPE.H99 COPY.SHC=COPY.H99 XMODEM.COM=XMODEM59.H99 DIR2.COM=DIR2.H99
IF ERRORLEVEL 1 GOTO FAILED
IF NOT EXIST "%OUT%" GOTO FAILED

REM --- 6. publish to the sibling MONITOR directory -------------------
IF NOT EXIST "%DEST%\" (
    ECHO *** MONITOR directory not found: "%DEST%"
    GOTO FAILED
)
COPY /Y "%OUT%" "%DEST%\%OUT%" > NUL || GOTO FAILED
IF NOT EXIST "%DEST%\%OUT%" GOTO FAILED

POPD
SET "IN_BUILD="
ECHO.
ECHO === BUILD OK: Build\%OUT% copied to %DEST% ===
EXIT /B 0

:FAILED
IF DEFINED IN_BUILD POPD
ECHO.
ECHO === BUILD FAILED - nothing copied to %DEST% ===
EXIT /B 1

REM --------------------------------------------------------------------
REM  :ASM name - assemble one file.  A99 writes a .H99 even when it has
REM  errors, so the old one is deleted first and success is judged from
REM  A99's "No error(s)." line.  The full output is kept in name.LOG.
REM --------------------------------------------------------------------
:ASM
IF EXIST %1.H99 DEL %1.H99
A99 %1 SCHCLC > %1.LOG
FINDSTR /C:"No error(s)." %1.LOG > NUL
IF ERRORLEVEL 1 GOTO ASM_BAD
IF NOT EXIST %1.H99 GOTO ASM_BAD
ECHO     %1 ok
GOTO :EOF
:ASM_BAD
ECHO *** %1 FAILED:
TYPE %1.LOG
SET "FAIL=1"
GOTO :EOF
