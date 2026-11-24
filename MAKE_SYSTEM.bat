@ECHO OFF
REM ====================================================================
REM  MAKE_SYSTEM.BAT - build the complete TMS99105 Shell 6.5 OS volume
REM
REM  Master files are organised consistently:
REM    SRC\<component>\    assembly and C source
REM    Includes\          hand-maintained include files
REM    Toolchain\         a99.exe and mkcinc.exe
REM    Build\             all staging files and generated outputs
REM    ..\MONITOR\        sibling upload staging for payload, utilities
REM                       and the Disc Monitor ROM image
REM
REM  A99 and the assembly INCL statements use C: relative filenames.
REM  Stage all sources and tools together in Build and run from there.
REM  Requires drive C: and MinGW gcc on PATH.
REM ====================================================================
SETLOCAL EnableExtensions
CD /D "%~dp0" || EXIT /B 1

SET "ROOT=%CD%"
SET "WORK=%ROOT%\Build"
SET "DEST=%ROOT%\..\MONITOR"
SET "OUT=PAYLOAD65.HEX"
SET "PAGES=DIR SAVE ERA REN MKDIR CHDIR RMDIR LOAD TYPE COPY"
REM Disc Monitor ROM source in SRC\DISC_MONITOR; not packed into PAYLOAD65.HEX.
SET "MONITOR_ROM=DISC_MONITOR_V55F"
SET "FAIL="
SET "IN_BUILD="

REM Standalone programs for upload; not packed into PAYLOAD65.HEX.
REM Every .A99 in SRC\Utilities is built, so a new utility needs no edit here.
SET "UTILITIES="
FOR %%F IN ("%ROOT%\SRC\Utilities\*.A99") DO CALL SET "UTILITIES=%%UTILITIES%% %%~nF"

IF /I NOT "%~d0"=="C:" (
    ECHO *** Project must be on drive C: because A99 uses C: paths.
    GOTO FAILED
)

REM --- Check master inputs before touching the generated outputs -----
FOR %%F IN (a99.exe mkcinc.exe) DO IF NOT EXIST "%ROOT%\Toolchain\%%F" (ECHO *** Toolchain\%%F missing & SET "FAIL=1")
IF NOT EXIST "%ROOT%\Includes\DECLS.INC" (ECHO *** Includes\DECLS.INC missing & SET "FAIL=1")
IF NOT EXIST "%ROOT%\SRC\SYSGEN\sysgen.c" (ECHO *** SRC\SYSGEN\sysgen.c missing & SET "FAIL=1")
FOR %%F IN (DSKINIT\DSKINIT62.A99 BDOS\BDOS61.A99 XMODEM\XMODEM59.A99 SHELL\DIR2.A99 SHELL\SHELLV65.A99) DO IF NOT EXIST "%ROOT%\SRC\%%F" (ECHO *** SRC\%%F missing & SET "FAIL=1")
FOR %%P IN (%PAGES%) DO IF NOT EXIST "%ROOT%\SRC\SHELL\%%P.A99" (ECHO *** SRC\SHELL\%%P.A99 missing & SET "FAIL=1")
IF NOT DEFINED UTILITIES (ECHO *** no .A99 files in SRC\Utilities & SET "FAIL=1")
IF NOT EXIST "%ROOT%\SRC\DISC_MONITOR\%MONITOR_ROM%.A99" (ECHO *** SRC\DISC_MONITOR\%MONITOR_ROM%.A99 missing & SET "FAIL=1")
IF DEFINED FAIL GOTO FAILED

REM --- Stage working copies; master sources are not modified ---------
IF NOT EXIST "%WORK%\" (
    MKDIR "%WORK%"
    IF ERRORLEVEL 1 GOTO FAILED
)
IF NOT EXIST "%DEST%\" (
    MKDIR "%DEST%"
    IF ERRORLEVEL 1 GOTO FAILED
)
ECHO --- prepare Build
COPY /Y "%ROOT%\Toolchain\*.EXE" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%ROOT%\Includes\*.INC" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%ROOT%\SRC\SYSGEN\sysgen.c" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%ROOT%\SRC\DSKINIT\DSKINIT62.A99" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%ROOT%\SRC\BDOS\BDOS61.A99" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%ROOT%\SRC\XMODEM\XMODEM59.A99" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%ROOT%\SRC\SHELL\*.A99" "%WORK%\" > NUL || GOTO FAILED
COPY /Y "%ROOT%\SRC\Utilities\*.A99" "%WORK%\" > NUL || GOTO FAILED
REM Only the monitor itself: SRC\DISC_MONITOR also holds a reference copy of
REM IDE_DEBUG.A99, which must not replace the SRC\Utilities copy just staged.
COPY /Y "%ROOT%\SRC\DISC_MONITOR\%MONITOR_ROM%.A99" "%WORK%\" > NUL || GOTO FAILED

PUSHD "%WORK%" || GOTO FAILED
SET "IN_BUILD=1"

REM --- 1. Compile the Windows system image packer --------------------
ECHO --- tools
gcc -O2 -o SYSGEN.EXE sysgen.c || GOTO FAILED

REM --- 2. Assemble all four non-Shell system components ---------------
ECHO --- system components
CALL :ASM DSKINIT62
IF DEFINED FAIL GOTO FAILED
CALL :ASM BDOS61
IF DEFINED FAIL GOTO FAILED
CALL :ASM XMODEM59
IF DEFINED FAIL GOTO FAILED
CALL :ASM DIR2
IF DEFINED FAIL GOTO FAILED

REM --- 3. Assemble Shell core and export its current addresses --------
ECHO --- Shell core
CALL :ASM SHELLV65
IF DEFINED FAIL GOTO FAILED

ECHO --- CORE.INC
IF EXIST CORE.INC DEL CORE.INC
IF EXIST CORE.INC GOTO FAILED
MKCINC SHELLV65.L99 DECLS.INC CORE.INC || GOTO FAILED
IF NOT EXIST CORE.INC GOTO FAILED

REM --- 4. Assemble all ten Shell command pages -----------------------
ECHO --- Shell command pages
FOR %%P IN (%PAGES%) DO CALL :ASM %%P
IF DEFINED FAIL GOTO FAILED

REM --- 5. Assemble standalone utilities (outside SYSGEN) --------------
ECHO --- utilities:%UTILITIES%
FOR %%U IN (%UTILITIES%) DO CALL :ASM %%U
IF DEFINED FAIL GOTO FAILED

REM --- 5a. Assemble the Disc Monitor ROM image (outside SYSGEN) --------
ECHO --- Disc Monitor ROM
CALL :ASM %MONITOR_ROM%
IF DEFINED FAIL GOTO FAILED

REM --- 6. Generate the complete volume -------------------------------
REM  SHELL.SYS and BDOS.SYS MUST be the first two directory entries.
ECHO --- SYSGEN
IF EXIST "%OUT%" DEL "%OUT%"
IF EXIST "%OUT%" GOTO FAILED
SYSGEN %OUT% DSKINIT62.H99 SHELL.SYS=SHELLV65.H99 BDOS.SYS=BDOS61.H99 ^
  DIR.SHC=DIR.H99 SAVE.SHC=SAVE.H99 ERA.SHC=ERA.H99 REN.SHC=REN.H99 ^
  MKDIR.SHC=MKDIR.H99 CHDIR.SHC=CHDIR.H99 RMDIR.SHC=RMDIR.H99 ^
  LOAD.SHC=LOAD.H99 TYPE.SHC=TYPE.H99 COPY.SHC=COPY.H99 XMODEM.COM=XMODEM59.H99 DIR2.COM=DIR2.H99
IF ERRORLEVEL 1 GOTO FAILED
IF NOT EXIST "%OUT%" GOTO FAILED

REM --- 7. Publish only after the complete build succeeds ------------
REM Utility .H99 files are loaded individually with the Disc Monitor.
FOR %%U IN (%UTILITIES%) DO (
    COPY /Y "%%U.H99" "%DEST%\%%U.H99" > NUL || GOTO FAILED
    IF NOT EXIST "%DEST%\%%U.H99" GOTO FAILED
)
REM The Disc Monitor image is for the ROM programmer, not for upload.
COPY /Y "%MONITOR_ROM%.H99" "%DEST%\%MONITOR_ROM%.H99" > NUL || GOTO FAILED
IF NOT EXIST "%DEST%\%MONITOR_ROM%.H99" GOTO FAILED
REM Publish the OS payload last, once utility staging has succeeded.
COPY /Y "%OUT%" "%DEST%\%OUT%" > NUL || GOTO FAILED
IF NOT EXIST "%DEST%\%OUT%" GOTO FAILED

POPD
SET "IN_BUILD="
ECHO.
ECHO === BUILD OK: %OUT%, utilities and %MONITOR_ROM%.H99 copied to %DEST% ===
EXIT /B 0

:FAILED
IF DEFINED IN_BUILD POPD
ECHO.
ECHO === BUILD FAILED - check errors above ===
EXIT /B 1

REM --------------------------------------------------------------------
REM  A99 can output a .H99 despite assembly errors.  Delete the old
REM  file first, then require both the success message and new .H99.
REM --------------------------------------------------------------------
:ASM
IF EXIST %1.H99 DEL %1.H99
IF EXIST %1.H99 GOTO ASM_BAD
A99 %1 SCHCLC > %1.LOG
FINDSTR /C:"No error(s)." %1.LOG > NUL
IF ERRORLEVEL 1 GOTO ASM_BAD
IF NOT EXIST %1.H99 GOTO ASM_BAD
ECHO     %1 ok
GOTO :EOF
:ASM_BAD
ECHO *** %1 FAILED:
IF EXIST %1.LOG TYPE %1.LOG
SET "FAIL=1"
GOTO :EOF
