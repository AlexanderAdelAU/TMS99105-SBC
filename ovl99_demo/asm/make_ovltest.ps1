# make_ovltest.ps1 -- build the OVL99 assembler overlay demo (OVLTEST) on Windows.
#
#   cd <this folder>
#   .\make_ovltest.ps1
#
# Uses the OVL99 demo's own toolchain and runtime, one folder up, so
# this folder holds no executables and no second copy of the runtime:
#   ..\tools\     r99.exe (assembler), link99.exe (linker), pack99.ps1
#   ..\runtime\   OVLMGR.A99, OVCALL.A99
#   ..\src\       CONOUT.A99 (pstr)
#
# Writes into .\build:
#   OVLTEST.EXE   the program.  XMODEM it to the SBC and type OVLTEST.
#   OVLTEST.PAK   everything LINK99 needs to link OVLTEST.EXE on the SBC:
#                 UNPACK OVLTEST  then  LINK99 @OVLTEST.LNK
#
# Both are also copied to the SBC MONITOR folder if one is found:
# $env:SBC_MONITOR_DIR, or a TMS99105_SBC\MONITOR folder above this one.
$ErrorActionPreference = "Stop"
$Here  = if($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
$Demo  = Split-Path -Parent $Here
$Tools = Join-Path $Demo "tools"
$Build = Join-Path $Here "build"

foreach($f in @("r99.exe", "link99.exe", "pack99.ps1")) {
    if(-not (Test-Path -LiteralPath (Join-Path $Tools $f))) { throw "missing $Tools\$f" }
}

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
Copy-Item (Join-Path $Tools "r99.exe"), (Join-Path $Tools "link99.exe") $Build -Force
Copy-Item (Join-Path $Here "src\*") $Build -Force
Copy-Item (Join-Path $Demo "runtime\*") $Build -Force
Copy-Item (Join-Path $Demo "src\CONOUT.A99") $Build -Force
Copy-Item (Join-Path $Here "OVLMAP.TXT"), (Join-Path $Here "OVLTEST.LNK") $Build -Force

Push-Location $Build
[Environment]::CurrentDirectory = (Get-Location).Path
try {
    # 1. assemble the program, its overlays, CONOUT and the OVL99 runtime.
    #    r99's S and H flags name the DOS drive for the source and the
    #    output (SCHC = drive C:), so use the drive this folder is on.
    $drive = ([IO.Path]::GetPathRoot((Get-Location).Path)).Substring(0,1).ToUpperInvariant()
    $r99Flags = "S${drive}H${drive}"
    $asm = @("OVLTEST","OVLA","OVLB","CONOUT","OVLMGR","OVCALL")
    foreach($a in $asm) {
        $o = & .\r99.exe $a $r99Flags 2>&1 | Out-String
        if($o -notmatch "No error\(s\)") { Write-Output $o; throw "assemble failed: $a" }
    }
    Write-Output ("assembled  " + ($asm -join " "))

    # 2. link with the arguments in OVLTEST.LNK, the file LINK99 reads on the SBC
    $args99 = @()
    foreach($line in Get-Content ".\OVLTEST.LNK") {
        $line = ($line -split '#')[0]
        $args99 += @($line -split '\s+' | Where-Object { $_ })
    }
    Write-Output ("link99 " + ($args99 -join " "))
    $o = & .\link99.exe @args99 2>&1 | Out-String
    Write-Output $o
    if($o -match '(?im)^\s*-\s*Unresolved:|\*{3}\s*FATAL') { throw "link failed" }
    $exe = Get-Item ".\OVLTEST.EXE"
    Write-Output ("OVLTEST.EXE  {0} bytes" -f $exe.Length)

    # 3. the same inputs in one archive, for linking on the SBC
    $pak = @("OVLTEST.R99","OVLMGR.R99","OVCALL.R99","CONOUT.R99",
             "OVLA.R99","OVLB.R99","OVLMAP.TXT","OVLTEST.LNK")
    & (Join-Path $Tools "pack99.ps1") "OVLTEST.PAK" @pak

    if($MonitorDir -and (Test-Path -LiteralPath $MonitorDir -PathType Container)) {
        Copy-Item ".\OVLTEST.EXE", ".\OVLTEST.PAK" $MonitorDir -Force
        Write-Output ("Copied OVLTEST.EXE and OVLTEST.PAK to " + $MonitorDir)
    } else {
        Write-Output ("No MONITOR folder found; OVLTEST.EXE and OVLTEST.PAK are in " + $Build)
    }
}
finally {
    Pop-Location
    [Environment]::CurrentDirectory = (Get-Location).Path
}
