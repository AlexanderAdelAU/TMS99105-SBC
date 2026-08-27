# =============================================================================
# make_R99_SBC.ps1
# FLAT R99.COM BUILD - TMS99105 SBC
#
# Project layout:
#   .\src\       R99 C sources and R99-local headers
#   .\Include\   Small-C / tool-chain headers
#   .\Lib\       runtime objects, libraries and indexes
#   .\*.exe      Windows-hosted cross tools
#
# Build layout:
#   Everything required to build is copied FLAT into .\build\
#   and the complete build is performed there.
#
# Target output:
#   .\build\R99.COM
#
# R99.c contains main() and is compiled WITHOUT -M.
# All other R99 C modules are compiled WITH -M.
#
# ONE R99DATA MODULE OWNS SHARED STORAGE
# NO OVERLAYS
# FINAL R99.COM IS COPIED TO THE SBC MONITOR PROJECT
# =============================================================================

$ErrorActionPreference = "Stop"
$BuildScriptVersion = "R99-SBC-CONSOLE-20260825-2240"
Set-StrictMode -Version 2.0
Set-Location -LiteralPath $PSScriptRoot

$Root       = $PSScriptRoot
$SourceDir  = Join-Path $Root "src"
$IncludeDir = Join-Path $Root "Include"
$LibDir     = Join-Path $Root "Lib"
$BuildDir   = Join-Path $Root "build"
$DeployDir  = "C:\Development-W11DEV\Eclipse Workspace\TMS99105_SBC\MONITOR"

$MainModule = "R99"

$Modules = @(
    "R99EVAL",
    "R99GET",
    "R99SYMB",
    "R99TBLS",
    "mess",
    "R99DATA",
    "R99ASMLN",
    "R99REL",
    "R99PUT"
)

$Libraries = @(
    "iolib99.LIB",
    "clib99.LIB",
    "strlib99.LIB"
)

$RequiredSource = @(
    "R99.c",
    "R99EVAL.c",
    "R99GET.c",
    "R99SYMB.c",
    "R99TBLS.c",
    "mess.c",
    "R99DATA.c",
    "R99ASMLN.c",
    "R99REL.c",
    "R99PUT.c",
    "R99CFG.h",
    "R99Ext.h",
    "R99gbl.h",
    "REL99.h"
)

function Require-File {
    param([Parameter(Mandatory=$true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "*** BUILD FAILED: required file is missing: $Path"
    }
}

function Require-Directory {
    param([Parameter(Mandatory=$true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "*** BUILD FAILED: required directory is missing: $Path"
    }
}

function Compile-C {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [switch]$Main
    )

    Remove-Item -LiteralPath ".\$Name.A99" -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath ".\$Name.smallc.log" -Force -ErrorAction SilentlyContinue

    Write-Host ""
    Write-Host "================================================"

    if ($Main) {
        $args = @("-C", $Name)
        Write-Host "Compiling $Name.c  [MAIN - NO -M]"
    }
    else {
        $args = @("-C", "-M", $Name)
        Write-Host "Compiling $Name.c  [MODULE -M]"
    }

    Write-Host (".\smallcp.exe " + ($args -join " "))
    Write-Host "================================================"

    # STREAM every compiler line to the console while also saving it.
    # No Out-String buffering.
    & .\smallcp.exe @args 2>&1 |
        Tee-Object -FilePath ".\$Name.smallc.log"

    $compilerExit = $LASTEXITCODE

    Write-Host ""
    Write-Host "Small-C exit code: $compilerExit"
    Write-Host "Small-C log:       $Name.smallc.log"

    if ($compilerExit -ne 0) {
        throw "*** BUILD FAILED: Small-C exit code $compilerExit compiling $Name.c"
    }

    Require-File ".\$Name.A99"
}

function Assemble-R99 {
    param([Parameter(Mandatory=$true)][string]$Name)

    Remove-Item -LiteralPath ".\$Name.R99" -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath ".\$Name.L99" -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath ".\$Name.r99.log" -Force -ErrorAction SilentlyContinue

    Write-Host ""
    Write-Host "================================================"
    Write-Host "Assembling $Name.A99"
    Write-Host ".\r99.exe $Name SCHCLC"
    Write-Host "================================================"

    # STREAM assembler output to console and keep a copy.
    & .\r99.exe $Name SCHCLC 2>&1 |
        Tee-Object -FilePath ".\$Name.r99.log"

    $assemblerExit = $LASTEXITCODE

    Write-Host ""
    Write-Host "R99 exit code: $assemblerExit"
    Write-Host "R99 log:       $Name.r99.log"

    if ($assemblerExit -ne 0) {
        throw "*** BUILD FAILED: R99 exit code $assemblerExit assembling $Name.A99"
    }

    Require-File ".\$Name.R99"
}

# -----------------------------------------------------------------------------
# 1. Verify project layout
# -----------------------------------------------------------------------------

Write-Output ""
Write-Output "================================================"
Write-Output "R99 FLAT COM BUILD"
Write-Output "SCRIPT VERSION: $BuildScriptVersion"
Write-Output "================================================"
Write-Output "Project: $Root"
Write-Output "Build:   $BuildDir"

Require-Directory $SourceDir
Require-Directory $IncludeDir
Require-Directory $LibDir

foreach ($name in $RequiredSource) {
    Require-File (Join-Path $SourceDir $name)
}

foreach ($tool in @("smallcp.exe", "r99.exe", "drel.exe", "link99.exe")) {
    Require-File (Join-Path $Root $tool)
}

# -----------------------------------------------------------------------------
# 2. Recreate build and copy EVERYTHING needed into it
#
# Copy order is deliberate:
#   Include first
#   Lib second
#   src LAST
#
# src\REL99.h is the R99 source-tree header and must win over any
# Include\rel99.h name collision on Windows.
# -----------------------------------------------------------------------------

Write-Output ""
Write-Output "================================================"
Write-Output "Preparing flat build directory"
Write-Output "================================================"

if (Test-Path -LiteralPath $BuildDir) {
    Remove-Item -LiteralPath $BuildDir -Recurse -Force
}

New-Item -Path $BuildDir -ItemType Directory -Force | Out-Null

Write-Output "Copy Include\* -> build\"
Copy-Item -Path (Join-Path $IncludeDir "*") -Destination $BuildDir -Recurse -Force

Write-Output "Copy Lib\*     -> build\"
Copy-Item -Path (Join-Path $LibDir "*") -Destination $BuildDir -Recurse -Force

Write-Output "Copy src\*     -> build\"
Copy-Item -Path (Join-Path $SourceDir "*") -Destination $BuildDir -Recurse -Force

Write-Output "Copy root *.exe -> build\"
Get-ChildItem -LiteralPath $Root -Filter "*.exe" -File | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $BuildDir -Force
}

# Remove copied products from any previous source-tree build so stale files
# cannot make a failed build look successful.
foreach ($module in @($MainModule) + $Modules) {
    Remove-Item -LiteralPath (Join-Path $BuildDir "$module.A99") -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $BuildDir "$module.R99") -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $BuildDir "$module.L99") -Force -ErrorAction SilentlyContinue
}

Remove-Item -LiteralPath (Join-Path $BuildDir "R99.COM") -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath (Join-Path $BuildDir "link99.log") -Force -ErrorAction SilentlyContinue

# Check that the runtime and libraries really made it into build.
foreach ($lib in $Libraries) {
    Require-File (Join-Path $BuildDir $lib)
}

foreach ($tool in @("smallcp.exe", "r99.exe", "drel.exe", "link99.exe")) {
    Require-File (Join-Path $BuildDir $tool)
}

# -----------------------------------------------------------------------------
# 3. Build entirely inside build\
# -----------------------------------------------------------------------------

Push-Location -LiteralPath $BuildDir

try {
    # -------------------------------------------------------------------------
    # Small-C
    # -------------------------------------------------------------------------

    Compile-C $MainModule -Main

    foreach ($module in $Modules) {
        Compile-C $module
    }

    # -------------------------------------------------------------------------
    # R99
    # -------------------------------------------------------------------------

    foreach ($module in @($MainModule) + $Modules) {
        Assemble-R99 $module
    }

    # -------------------------------------------------------------------------
    # DREL - decode every R99 object in build\
    #
    # Default DREL mode is the REL/LIB object dump.  Dump the ten newly built
    # R99 program modules.  Stream output to the console and retain one
    # combined log.
    # -------------------------------------------------------------------------

    Write-Output ""
    Write-Output "================================================"
    Write-Output "DREL - R99 OBJECT DUMP"
    Write-Output "================================================"

    $DrelLog = Join-Path $BuildDir "drel_dump.log"
    Remove-Item -LiteralPath $DrelLog -Force -ErrorAction SilentlyContinue

    $R99Objects = @($MainModule) + $Modules

    foreach ($module in $R99Objects) {
        $obj = Get-Item -LiteralPath (Join-Path $BuildDir "$module.R99")
        $header = @(
            "",
            "================================================",
            ("DREL DUMP: {0}" -f $obj.Name),
            "================================================"
        )

        $header | Tee-Object -FilePath $DrelLog -Append

        & .\drel.exe $obj.Name 2>&1 |
            Tee-Object -FilePath $DrelLog -Append

        $drelExit = $LASTEXITCODE

        Write-Output ""
        Write-Output ("DREL exit code for {0}: {1}" -f $obj.Name, $drelExit)

        if ($drelExit -ne 0) {
            throw "*** BUILD FAILED: DREL exit code $drelExit dumping $($obj.Name)"
        }
    }

    Write-Output ""
    Write-Output ("DREL combined dump: {0}" -f $DrelLog)
    Write-Output ""

    # -------------------------------------------------------------------------
    # LINK99 - one flat COM file
    #
    # Proven syntax:
    #   link99 -O0x1000 -S -M output.COM main.R99 ... libraries
    # -------------------------------------------------------------------------

    Write-Output ""
    Write-Output "================================================"
    Write-Output "LINK99 - FLAT R99.COM"
    Write-Output "================================================"

    $LinkArgs = @(
        "-O0x1000",
        "-S",
        "-M",
        "R99.COM",
        "R99.R99"
    )

    $LinkArgs += ($Modules | ForEach-Object { "$_.R99" })
    $LinkArgs += $Libraries

    Write-Output (".\link99.exe " + ($LinkArgs -join " "))

    Remove-Item -LiteralPath ".\link99.log" -Force -ErrorAction SilentlyContinue

    # Stream LINK99 output live so a long link does not look like a hang,
    # while retaining the complete output in link99.log.
    & .\link99.exe @LinkArgs 2>&1 |
        Tee-Object -FilePath ".\link99.log"

    $linkExit = $LASTEXITCODE
    $linkLog = Get-Content -LiteralPath ".\link99.log" -Raw

    Write-Output ""
    Write-Output ("LINK99 exit code: {0}" -f $linkExit)

    if ($linkExit -ne 0) {
        throw "*** BUILD FAILED: LINK99 exit code $linkExit"
    }

    if ($linkLog -match '(?im)^\s*-\s*Unresolved:' -or
        $linkLog -match '(?im)^\s*-\s*Error:'      -or
        $linkLog -match '(?im)^\s*\*{3}\s*FATAL\b') {
        throw "*** BUILD FAILED: LINK99 reported a link error"
    }

    Require-File ".\R99.COM"

    $comFile = Join-Path $BuildDir "R99.COM"
    $comSize = (Get-Item -LiteralPath $comFile).Length

    Write-Output ""
    Write-Output "================================================"
    Write-Output "R99.COM SIZE"
    Write-Output "================================================"
    Write-Output ("build\R99.COM = {0} bytes (0x{1:X})" -f $comSize, $comSize)

    # Initial SBC bring-up safety limit:
    # load base >1000, image must end below >C000.
    $MaxComSize = 0xB000
    $LoadBase   = 0x1000
    $EndAddress = $LoadBase + $comSize - 1
    Write-Output ("Load span: >{0:X4}->>{1:X4}  ceiling >BFFF" -f $LoadBase, $EndAddress)

    if ($comSize -gt $MaxComSize) {
        throw ("*** BUILD FAILED: R99.COM is 0x{0:X} bytes; maximum for >1000->>BFFF is 0x{1:X}. NOT DEPLOYED." -f $comSize, $MaxComSize)
    }

    if (-not (Test-Path -LiteralPath $DeployDir -PathType Container)) {
        throw "*** BUILD FAILED: deploy directory does not exist: $DeployDir"
    }

    $deployFile = Join-Path $DeployDir "R99.COM"
    Copy-Item -LiteralPath $comFile -Destination $deployFile -Force

    $deploySize = (Get-Item -LiteralPath $deployFile).Length
    if ($deploySize -ne $comSize) {
        throw "*** BUILD FAILED: deployed R99.COM size does not match build output"
    }

    Write-Output ""
    Write-Output "================================================"
    Write-Output "R99 FLAT COM BUILD SUCCESSFUL"
    Write-Output "================================================"
    Write-Output ("Built:    {0} bytes  {1}" -f $comSize, $comFile)
    Write-Output ("Deployed: {0} bytes  {1}" -f $deploySize, $deployFile)
    Write-Output ""
}
finally {
    Pop-Location
}
