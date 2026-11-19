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
# OVLDEMO.EXE and OVLDEMO.PAK are also copied to the SBC MONITOR folder if
# one is found: $env:SBC_MONITOR_DIR, or a TMS99105_SBC\MONITOR folder
# above this one.
#
# The link arguments are read from OVLDEMO.LNK, the same file the SBC's
# LINK99 reads, so the two links cannot drift apart.
$ErrorActionPreference = "Stop"
$Here  = if($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
$Build = Join-Path $Here "build"

$MonitorDir = $env:SBC_MONITOR_DIR
if(-not $MonitorDir) {
    $up = $Here
    while($up) {
        $c = Join-Path $up "TMS99105_SBC\MONITOR"
        if(Test-Path -LiteralPath $c -PathType Container) { $MonitorDir = $c; break }
        $next = Split-Path -Parent $up
        if(-not $next -or $next -eq $up) { break }
        $up = $next
    }
}

if(Test-Path -LiteralPath $Build) { Remove-Item -LiteralPath $Build -Recurse -Force }
New-Item -ItemType Directory -Path $Build | Out-Null
Copy-Item (Join-Path $Here "tools\*.exe") $Build -Force
Copy-Item (Join-Path $Here "lib\*") $Build -Force
Copy-Item (Join-Path $Here "src\*") $Build -Force
Copy-Item (Join-Path $Here "runtime\*") $Build -Force
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
    & (Join-Path $Here "tools\pack99.ps1") "OVLDEMO.PAK" @pak

    if($MonitorDir -and (Test-Path -LiteralPath $MonitorDir -PathType Container)) {
        Copy-Item ".\OVLDEMO.EXE", ".\OVLDEMO.PAK" $MonitorDir -Force
        Write-Output ("Copied OVLDEMO.EXE and OVLDEMO.PAK to " + $MonitorDir)
    } else {
        Write-Output ("No MONITOR folder found; OVLDEMO.EXE and OVLDEMO.PAK are in " + $Build)
    }
}
finally {
    Pop-Location
    [Environment]::CurrentDirectory = (Get-Location).Path
}
