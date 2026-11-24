# make_ovldemo.ps1 -- build the OVL99 demo (OVLDEMO) on Windows.
#
#   cd <this folder>
#   .\make_ovldemo.ps1          (or paste the whole file into PowerShell)
#
# Writes into .\build:
#   OVLDEMO.EXE   the program.  XMODEM it to the SBC and type OVLDEMO.
#   OVLDEMO.PAK   everything LINK99 needs to link OVLDEMO.EXE on the SBC
#               itself, in one file:  UNPACK OVLDEMO  then  LINK99 @OVLDEMO.LNK
#
# Shared with the assembler demo, so not kept in this folder:
#   ..\Toolchain\      smallcp.exe, r99.exe, link99.exe, pack99.ps1
#   ..\ovl99_runtime\  OVLMGR.A99, OVCALL.A99 (the OVL99 runtime)
#
# OVLDEMO.EXE and OVLDEMO.PAK are also copied to the staging folder:
# $env:SBC_STAGING_DIR if it is set, otherwise STAGING next to the
# repository (the folder MAKE_SYSTEM.bat uses).
#
# The link arguments are read from OVLDEMO.LNK, the same file the SBC's
# LINK99 reads, so the two links cannot drift apart.
$ErrorActionPreference = "Stop"
$Here  = if($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
$Build = Join-Path $Here "build"
$Root  = Split-Path -Parent $Here
$Tools = Join-Path $Root "Toolchain"
$Runtime = Join-Path $Root "ovl99_runtime"
foreach($f in @("smallcp.exe", "r99.exe", "link99.exe", "pack99.ps1")) {
    if(-not (Test-Path -LiteralPath (Join-Path $Tools $f))) { throw "missing $Tools\$f" }
}

$Staging = $env:SBC_STAGING_DIR
if(-not $Staging) { $Staging = Join-Path (Split-Path -Parent $Root) "STAGING" }

if(Test-Path -LiteralPath $Build) { Remove-Item -LiteralPath $Build -Recurse -Force }
New-Item -ItemType Directory -Path $Build | Out-Null
Copy-Item (Join-Path $Tools "smallcp.exe"), (Join-Path $Tools "r99.exe"), (Join-Path $Tools "link99.exe") $Build -Force
Copy-Item (Join-Path $Here "lib\*") $Build -Force
Copy-Item (Join-Path $Here "src\*") $Build -Force
Copy-Item (Join-Path $Runtime "*.A99") $Build -Force
Copy-Item (Join-Path $Here "OVLMAP.TXT"), (Join-Path $Here "OVLDEMO.LNK") $Build -Force

Push-Location $Build
[Environment]::CurrentDirectory = (Get-Location).Path
try {
    # 1. compile.  Overlays are modules (-M): no main, and they keep
    #    the AORG their #asm block gives them.
    foreach($c in @("OVLHELLO","OVLGRET2","OVLBYE")) {
        $o = & .\smallcp.exe -C -M $c 2>&1 | Out-String
        if($o -notmatch "Errors:\s+0") { Write-Output $o; throw "compile failed: $c" }
    }
    $o = & .\smallcp.exe -C MAIN 2>&1 | Out-String
    if($o -notmatch "Errors:\s+0") { Write-Output $o; throw "compile failed: MAIN" }
    Write-Output "compiled   MAIN OVLHELLO OVLGRET2 OVLBYE"

    # 2. assemble the compiler output, the demo's own assembler and
    #    the OVL99 runtime
    $asm = @("MAIN","OVLHELLO","OVLGRET2","OVLBYE","STARTUP","CONOUT","OVLMGR","OVCALL")
    foreach($a in $asm) {
        $o = & .\r99.exe $a SCHC 2>&1 | Out-String
        if($o -notmatch "No error\(s\)") { Write-Output $o; throw "assemble failed: $a" }
    }
    Write-Output ("assembled  " + ($asm -join " "))

    # 3. link with the arguments in OVLDEMO.LNK
    $args99 = @()
    foreach($line in Get-Content ".\OVLDEMO.LNK") {
        $line = ($line -split '#')[0]
        $args99 += @($line -split '\s+' | Where-Object { $_ })
    }
    Write-Output ("link99 " + ($args99 -join " "))
    $o = & .\link99.exe @args99 2>&1 | Out-String
    Write-Output $o
    if($o -match '(?im)^\s*-\s*Unresolved:|\*{3}\s*FATAL') { throw "link failed" }
    $exe = Get-Item ".\OVLDEMO.EXE"
    Write-Output ("OVLDEMO.EXE  {0} bytes" -f $exe.Length)

    # 4. the same inputs in one archive, for linking on the SBC
    $pak = @("STARTUP.R99","MAIN.R99","OVLMGR.R99","OVCALL.R99","CONOUT.R99","CALL.R99",
             "OVLHELLO.R99","OVLGRET2.R99","OVLBYE.R99","CLIB99.LIB","CLIB99.NDX",
             "OVLMAP.TXT","OVLDEMO.LNK")
    & (Join-Path $Tools "pack99.ps1") "OVLDEMO.PAK" @pak

    New-Item -ItemType Directory -Path $Staging -Force | Out-Null
    Copy-Item ".\OVLDEMO.EXE", ".\OVLDEMO.PAK" $Staging -Force
    Write-Output ("Copied OVLDEMO.EXE and OVLDEMO.PAK to " + $Staging)
}
finally {
    Pop-Location
    [Environment]::CurrentDirectory = (Get-Location).Path
}
