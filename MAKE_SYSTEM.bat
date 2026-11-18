@ECHO OFF
REM ====================================================================
REM  Build the TMS99105 SBC operating system entirely from source.
REM
REM  1. Assemble DSKINIT62, BDOS61, XMODEM59 and DIR2 into Build\.
REM  2. Pass their fresh .H99 files to SHELL\MAKESHELL.bat.
REM  3. The Shell builds its core and ten commands, runs SYSGEN and
REM     publishes PAYLOAD65.HEX to MONITOR\.
REM
REM  A99 requires C: paths.  As with the Shell build, all assembly is
REM  performed from a local staging directory on drive C:.
REM  MinGW gcc must be on PATH for the Shell's SYSGEN compilation.
REM  A failed component stops the build before the system is packaged.
REM ====================================================================
SETLOCAL EnableExtensions
CD /D "%~dp0" || EXIT /B 1

SET "ROOT=%CD%"
SET "WORK=%ROOT%\Build\Modules"
SET "PACK=%ROOT%\Build\SystemFiles"
SET "MONITOR=%ROOT%\MONITOR"

IF /I NOT "%~d0"=="C:" (
    ECHO *** Put the project on drive C: because A99 uses C: paths.
    GOTO FAILED
)
IF NOT EXIST "%ROOT%\Toolchain\a99.exe" (
    ECHO *** Toolchain\a99.exe missing
    GOTO FAILED
)
IF NOT EXIST "%ROOT%\SHELL\MAKESHELL.bat" (
    ECHO *** SHELL\MAKESHELL.bat missing
    GOTO FAILED
)

IF NOT EXIST "%WORK%\" (
    MKDIR "%WORK%"
    IF ERRORLEVEL 1 GOTO FAILED
)
IF NOT EXIST "%PACK%\" (
    MKDIR "%PACK%"
    IF ERRORLEVEL 1 GOTO FAILED
)
IF NOT EXIST "%MONITOR%\" (
    MKDIR "%MONITOR%"
    IF ERRORLEVEL 1 GOTO FAILED
)

COPY /Y "%ROOT%\Toolchain\a99.exe" "%WORK%\" > NUL || GOTO FAILED

ECHO.
ECHO === Building system modules from source ===
CALL :ASM DSKINIT62 "%ROOT%\DSKINIT\SRC\DSKINIT62.A99" || GOTO FAILED
CALL :ASM BDOS61 "%ROOT%\BDOS\SRC\BDOS61.A99" || GOTO FAILED
CALL :ASM XMODEM59 "%ROOT%\XMODEM\SRC\XMODEM59.A99" || GOTO FAILED
CALL :ASM DIR2 "%ROOT%\DIR2\SRC\DIR2.A99" || GOTO FAILED

REM  The four .H99 inputs have all been rebuilt successfully.
REM  Pass the collection directly: checked-in Shell inputs stay intact.
ECHO.
ECHO === Building Shell and packing system volume ===
CALL "%ROOT%\SHELL\MAKESHELL.bat" "%PACK%" || GOTO FAILED

IF NOT EXIST "%MONITOR%\PAYLOAD65.HEX" GOTO FAILED
ECHO.
ECHO === SYSTEM BUILD OK: MONITOR\PAYLOAD65.HEX ===
EXIT /B 0

:FAILED
ECHO.
ECHO === SYSTEM BUILD FAILED - see errors above ===
EXIT /B 1

REM --------------------------------------------------------------------
REM  :ASM NAME SOURCE - stage and assemble one independent source.
REM  Delete the prior .H99 first: A99 can emit one even on an error.
REM --------------------------------------------------------------------
:ASM
SET "NAME=%~1"
SET "SOURCE=%~2"
IF NOT EXIST "%SOURCE%" (
    ECHO *** Missing source: "%SOURCE%"
    EXIT /B 1
)
COPY /Y "%SOURCE%" "%WORK%\%NAME%.A99" > NUL || EXIT /B 1
PUSHD "%WORK%" || EXIT /B 1
IF EXIST "%NAME%.H99" DEL "%NAME%.H99"
IF EXIST "%NAME%.H99" GOTO ASM_BAD
A99 %NAME% SCHCLC > "%NAME%.LOG"
FINDSTR /C:"No error(s)." "%NAME%.LOG" > NUL
IF ERRORLEVEL 1 GOTO ASM_BAD
IF NOT EXIST "%NAME%.H99" GOTO ASM_BAD
POPD
COPY /Y "%WORK%\%NAME%.H99" "%PACK%\%NAME%.H99" > NUL || EXIT /B 1
IF NOT EXIST "%PACK%\%NAME%.H99" EXIT /B 1
ECHO     %NAME% ok
EXIT /B 0

:ASM_BAD
ECHO *** %NAME% FAILED:
IF EXIST "%NAME%.LOG" TYPE "%NAME%.LOG"
POPD
EXIT /B 1
