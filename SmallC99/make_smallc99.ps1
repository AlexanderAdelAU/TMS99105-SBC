# BINARY-LIB-ONLY REFACTOR V4 - switch helper CALL fix; copies R99/LIB/NDX from .\lib
# -------------------------------------------------------
#  Build SMALLC99 - M38e two-page codegen overlay
# -------------------------------------------------------

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0
$script:CompileLogs = @{}
Set-Location -LiteralPath $PSScriptRoot

function Require-File {
    param([Parameter(Mandatory=$true)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "*** BUILD FAILED: required file is missing: $Path"
    }
}

function Get-OverlayRowWords {
    param(
        [Parameter(Mandatory=$true)][string[]]$TableLines,
        [Parameter(Mandatory=$true)][int]$Id
    )

    # Parse ONE physical line at a time.  Do not use \s in a whole-file regex:
    # in .NET \s includes CR/LF and can cause pathological cross-line backtracking.
    $pattern = '^[ \t]*WORD[ \t]+([0-9, \t]+)[ \t]*;[^\r\n]*\bID[ \t]+' + $Id + '\b[^\r\n]*$'
    $rowMatch = $null
    foreach ($line in $TableLines) {
        $candidate = [regex]::Match($line, $pattern, [Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if ($candidate.Success) {
            $rowMatch = $candidate
            break
        }
    }
    if ($null -eq $rowMatch) {
        throw "*** BUILD FAILED: no OVLTABLE.INC row found for overlay ID $Id"
    }

    $words = @()
    foreach ($item in ($rowMatch.Groups[1].Value -split ',')) {
        $trimmed = $item.Trim()
        if ($trimmed -ne '') { $words += [int]$trimmed }
    }
    if ($words.Count -ne 10) {
        throw "*** BUILD FAILED: overlay ID $Id row does not contain exactly 10 words"
    }
    return $words
}

function Get-OverlayAnchor {
    param(
        [Parameter(Mandatory=$true)][string]$AddrText,
        [Parameter(Mandatory=$true)][string]$Symbol
    )

    $pattern = '(?im)^\s*' + [regex]::Escape($Symbol) + '\s+EQU\s+0*([0-9A-F]+)H\b'
    $match = [regex]::Match($AddrText, $pattern)
    if (-not $match.Success) {
        throw "*** BUILD FAILED: OVLADDR.INC does not contain a hexadecimal EQU for $Symbol"
    }
    return [Convert]::ToInt32($match.Groups[1].Value, 16)
}

function Compile-C {
    param([Parameter(Mandatory=$true)][string]$Name)
    Write-Output "`n================================================"
    Write-Output "Compiling $Name"
    Write-Output "================================================"
    $output = & .\smallcp.exe -C -M $Name 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    Write-Output $output
    $script:CompileLogs[$Name] = $output
    if ($output -notmatch "Errors:\s+0") {
        throw "*** BUILD FAILED: compile $Name"
    }
    if ($exitCode -ne 0) {
        Write-Warning "smallcp returned exit code $exitCode for $Name, but reported Errors: 0"
    }
}

function Compile-C-Main {
    param([Parameter(Mandatory=$true)][string]$Name)
    Write-Output "`n================================================"
    Write-Output "Compiling $Name (runtime main module)"
    Write-Output "================================================"
    $output = & .\smallcp.exe -C $Name 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    Write-Output $output
    $script:CompileLogs[$Name] = $output
    if ($output -notmatch "Errors:\s+0") {
        throw "*** BUILD FAILED: compile $Name"
    }
    if ($exitCode -ne 0) {
        Write-Warning "smallcp returned exit code $exitCode for $Name, but reported Errors: 0"
    }
}

function Assemble-R99 {
    param([Parameter(Mandatory=$true)][string]$Name)
    Write-Output "`n================================================"
    Write-Output "Assembling $Name"
    Write-Output "================================================"
    $output = & .\r99.exe $Name SCHC 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    Write-Output $output
    if ($output -notmatch "No error\(s\)") {
        throw "*** BUILD FAILED: assemble $Name"
    }
    if ($exitCode -ne 0) {
        Write-Warning "R99 returned exit code $exitCode for $Name, but reported No error(s)"
    }
}

$LINK99_VER = "3.9.58"   # 3.9.58: >64K code, XRPLUS, cdisk, ifilelbuf (see link99.c)
$DREL_VER   = "Version 4.0"
$MONITOR_DIR = "C:\Development-W11DEV\Eclipse Workspace\TMS99105_SBC\MONITOR"
$TOOLCHAIN_DIR = Join-Path $PSScriptRoot "toolchain"
$HOST_TOOLS = @("smallcp.exe","r99.exe","drel.exe","link99.exe")

# Runtime dependency boundary: binary-only inputs from .\lib.
# This build does not require or inspect any IOLIB99 C/A99 source tree.
$RUNTIME_OBJECTS = @(
    "CALL.R99",
    "IOCORE.R99",
    "IOOPEN.R99",
    "IOREAD.R99",
    "IOWRITE.R99",
    "CBDOS.R99"
)
$RUNTIME_LIBRARIES = @(
    "CLIB99.LIB"
)
$RUNTIME_INDEXES = @(
    "CLIB99.NDX"
)
$LIB_INPUTS = @($RUNTIME_OBJECTS) + @($RUNTIME_LIBRARIES) + @($RUNTIME_INDEXES)

# Only binary runtime/library artifacts are imported from .\lib.
# This deliberately copies matching .NDX files alongside .LIB files.
$LIB_COPY_PATTERNS = @("*.R99", "*.LIB", "*.NDX")

$OVERLAYS = @(
    @{ Name="CC_DECL";   Page=2;  Descriptor="CC_DECL.R99=2,3,dodeclare:D_DODECL" }
    @{ Name="CC_PREP";   Page=9;  Descriptor="CC_PREP.R99=9,4,preprocess:P_PREP,dodefine:P_DEFINE,doinclude:P_INCLUDE,doasm:P_DOASM" }
    @{ Name="CC_MACS";   Page=10; Descriptor="CC_MACS.R99=10,4,putmac:-" }
    @{ Name="CC_STMT";   Page=3;  Descriptor="CC_STMT.R99=3,2,statement:S_STMT" }
    @{ Name="CC_STMT_R"; Page=14; Descriptor="CC_STMT_R.R99=14,2,doexpr:-" }
    @{ Name="CC_EXPR_A"; Page=4;  Descriptor="CC_EXPR_A.R99=4,1,expression:E_EXPR,constexpr:E_CEXPR" }
    @{ Name="CC_EXPR_B"; Page=5;  Descriptor="CC_EXPR_B.R99=5,1,test:E_TEST" }
    @{ Name="CC_DFUN";   Page=6;  Descriptor="CC_DFUN.R99=6,5,dofunction:D_DOFN" }
    @{ Name="CC_EXPR_C"; Page=7;  Descriptor="CC_EXPR_C.R99=7,1,level1:-" }
    @{ Name="CC_EXPR_D"; Page=8;  Descriptor="CC_EXPR_D.R99=8,1,primary:-" }
    @{ Name="CC_CG99";   Page=11; Descriptor="CC_CG99.R99=11,6,ccout:P_CCOUT" }
    @{ Name="CC_CG99T";  Page=13; Descriptor="CC_CG99T.R99=13,6,setcodes:P_SETCODES" }
    @{ Name="CC_CL99";   Page=12; Descriptor="CC_CL99.R99=12,7,getoptions:P_GETOPTS,errrep:P_ERRREP" }
    @{ Name="CC_STRD";   Page=15; Descriptor="CC_STRD.R99=15,8,dostruct:T_STRUCT" }   # structs phase 1: struct/union declarations
)

# Overlay ownership contract.  Framed overlay dispatch saves/restores only
# CUR_WINA_ID, and CUR_WINA_ID is defined by the overlay mapped into virtual
# segment 8.  Therefore every normal/nestable overlay must begin with segment 8.
# OVL_CLI (ID 7) is a top-level one-shot parser and is the only current exception.
# New exceptions must be explicit here; all future IDs are checked automatically.
$OVERLAY_OWNER_EXCEPTIONS = @(7)

$required = @(
    "src\resident\SMALLC99.C",
    "src\resident\CC_CD99.C",
    "src\overlays\CC_CG99.C",
    "src\overlays\CC_CG99T.C",
    "src\overlays\CC_CL99.C",
    "src\fixtures\M38B.C",
    "src\fixtures\TEST.C",
    "src\fixtures\M38D.C",
    "src\harness\M38DPROBE.A99",
    "src\resident\CC_PORT.A99",
    "src\resident\CLISTUB.A99",
    "src\resident\OVLMGR.A99",
    "src\resident\OVLSTUBS.A99",
    "src\resident\SPYOUT.A99",
    "src\resident\CC_STRU.A99",
    "src\overlays\CC_STRD.C",
    "include\OVLDEFS.INC"
)
foreach ($path in $required) { Require-File $path }

# Binary-only runtime preflight.  Fail before build_tms is touched.
foreach ($name in $LIB_INPUTS) {
    Require-File (Join-Path "lib" $name)
}
Write-Output "Runtime inputs: binary-only from .\lib"
foreach ($name in $LIB_INPUTS) {
    Write-Output ("  " + $name)
}

# Host tools live in the project-root toolchain directory.  Check them before
# touching build_tms so a missing/misplaced tool fails immediately.
foreach ($tool in $HOST_TOOLS) {
    Require-File (Join-Path $TOOLCHAIN_DIR $tool)
}
Write-Output ("Host toolchain: " + $TOOLCHAIN_DIR)

if (Test-Path -LiteralPath "build_tms") {
    Remove-Item -LiteralPath "build_tms" -Recurse -Force
}
New-Item -Path "build_tms" -ItemType Directory -Force | Out-Null

$commonC = @(
    "CC_RESIDENT.C", "CC_SCAN_SYM.C", "CC_DATA.C", "CC_STMT_R.C"
)
foreach ($name in $commonC) {
    Copy-Item -LiteralPath (Join-Path "src\resident" $name) -Destination (Join-Path "build_tms" $name) -Force
}

$commonOverlays = @(
    "CC_DECL.C", "CC_PREP.C", "CC_MACS.C", "CC_STMT.C",
    "CC_EXPR_A.C", "CC_EXPR_B.C", "CC_DFUN.C",
    "CC_EXPR_C.C", "CC_EXPR_D.C", "CC_STRD.C"
)
foreach ($name in $commonOverlays) {
    Copy-Item -LiteralPath (Join-Path "src\overlays" $name) -Destination (Join-Path "build_tms" $name) -Force
}

$residentAsm = @("CC_PORT.A99", "CLISTUB.A99", "OVLMGR.A99", "OVLSTUBS.A99", "SPYOUT.A99", "CC_STRU.A99")
foreach ($name in $residentAsm) {
    Copy-Item -LiteralPath (Join-Path "src\resident" $name) -Destination (Join-Path "build_tms" $name) -Force
}

Copy-Item -LiteralPath "src\resident\SMALLC99.C" -Destination "build_tms\SMALLC99.C" -Force
Copy-Item -LiteralPath "src\resident\CC_CD99.C" -Destination "build_tms\CC_CD99.C" -Force
Copy-Item -LiteralPath "src\overlays\CC_CG99.C" -Destination "build_tms\CC_CG99.C" -Force
Copy-Item -LiteralPath "src\overlays\CC_CG99T.C" -Destination "build_tms\CC_CG99T.C" -Force
Copy-Item -LiteralPath "src\overlays\CC_CL99.C" -Destination "build_tms\CC_CL99.C" -Force
Copy-Item -LiteralPath "src\fixtures\M38B.C" -Destination "build_tms\M38B.C" -Force
Copy-Item -LiteralPath "src\fixtures\TEST.C" -Destination "build_tms\TEST.C" -Force
Copy-Item -LiteralPath "src\fixtures\M38D.C" -Destination "build_tms\M38D.C" -Force
Copy-Item -LiteralPath "src\harness\M38DPROBE.A99" -Destination "build_tms\M38DPROBE.A99" -Force
Copy-Item -LiteralPath "include\OVLDEFS.INC" -Destination "build_tms\OVLDEFS.INC" -Force
# Copy binary runtime/library artifacts only.  No IOLIB99 source is used.
# Libraries and their .NDX files are kept together in build_tms.
foreach ($pattern in $LIB_COPY_PATTERNS) {
    Get-ChildItem -LiteralPath "lib" -Filter $pattern -File | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination (Join-Path "build_tms" $_.Name) -Force
    }
}
foreach ($tool in $HOST_TOOLS) {
    Copy-Item -LiteralPath (Join-Path $TOOLCHAIN_DIR $tool) -Destination (Join-Path "build_tms" $tool) -Force
}

Set-Location -LiteralPath "build_tms"

foreach ($tool in @("smallcp.exe","r99.exe","drel.exe","link99.exe")) { Require-File ".\$tool" }
foreach ($input in @("SMALLC99.C","CC_CD99.C","CC_CG99.C","CC_CG99T.C","CC_CL99.C","M38B.C","TEST.C","M38D.C","M38DPROBE.A99","OVLDEFS.INC")) { Require-File ".\$input" }
foreach ($name in $LIB_INPUTS) { Require-File ".\$name" }

$linkAscii = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes((Resolve-Path ".\link99.exe")))
if ($linkAscii -notmatch [regex]::Escape($LINK99_VER)) {
    throw "*** BUILD FAILED: copied link99.exe does not contain version $LINK99_VER"
}
$drelAscii = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes((Resolve-Path ".\drel.exe")))
if ($drelAscii -notmatch [regex]::Escape($DREL_VER)) {
    throw "*** BUILD FAILED: copied drel.exe does not contain $DREL_VER"
}

Write-Output "`n================================================"
Write-Output "Build input preflight"
Write-Output "================================================"
Write-Output "R4-primary ABI restore keeps the two-page OVL_CGEN architecture."
Write-Output "Target contract: R4 primary/result, R3 secondary/left, FP EQU 9, SP EQU 10, R11 link, WP EQU 13."

$cgText = Get-Content -LiteralPath ".\CC_CG99.C" -Raw
$cgtText = Get-Content -LiteralPath ".\CC_CG99T.C" -Raw
$portText = Get-Content -LiteralPath ".\CC_PORT.A99" -Raw
$callAscii = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes((Resolve-Path ".\call.R99")))
if ($callAscii -notmatch '_ccusr') {
    throw "*** BUILD FAILED: CALL.R99 does not export required unsigned-shift helper _ccusr"
}

# Startup must establish an unambiguous logical overlay-owner state before any
# resident vector can dispatch through a framed overlay stub.
if ($portText -notmatch '(?is)PORT_INIT\s*:\s*CALL\s+@OVLMGR_INIT\s*CLR\s+@CUR_WINA_ID\s*CALL\s+@STUBINIT') {
    throw "*** BUILD FAILED: PORT_INIT must clear CUR_WINA_ID immediately after OVLMGR_INIT and before STUBINIT"
}
Write-Output "Overlay startup audit: CUR_WINA_ID is explicitly reset before STUBINIT."

$declText = Get-Content -LiteralPath ".\CC_DECL.C" -Raw

# The active TMS backend must never emit Intel/8086 assembler syntax.
# Search only quoted assembler/output strings so p-code names such as PUSH1/POP2
# do not cause false positives.
$activeOutputText = $cgtText + "`n" + $declText + "`n" + (Get-Content -LiteralPath ".\CC_CD99.C" -Raw)
$forbidden8086Output = @(
    '"[^"\r\n]*\b(AX|BX|CX|DX|BP|SI|DI|AL|AH|BL|BH|CL|CH|DL|DH)\b',
    '"[^"\r\n]*\b(PUSH|POP|XCHG|LEA|CWD|IDIV|IMUL|SAL|SHL|SHR)\b',
    '"[^"\r\n]*(\bDW\b|\bDB\b|DUP\(|OFFSET[ \t])',
    '"[^"\r\n]*\b(JE|JGE|JLE|JG|JL)\b'
)
foreach ($pattern in $forbidden8086Output) {
    if ($activeOutputText -match $pattern) {
        throw "*** BUILD FAILED: active output generator contains forbidden 8086 syntax: $($Matches[0])"
    }
}
if ($declText -notmatch 'pstr\(" WORD \$\+2"\)') {
    throw "*** BUILD FAILED: CC_DECL string-literal pointer is not emitted with WORD `$+2"
}
if ($declText -match 'pstr\(" DW ') {
    throw "*** BUILD FAILED: CC_DECL still contains the obsolete DW directive"
}
Write-Output "Codegen audit: native TMS9900 output only; WORD literal pointers enabled."

# Bootstrap-safety rule: the prebuilt SMALLCP used to compile the compiler
# itself predates the corrected TMS switch-helper call convention.  Therefore
# active compiler C sources must not contain switch() statements.  User source
# switch support remains enabled in the target compiler and emits CALL @_ccswitc.
$compilerSources = Get-ChildItem -LiteralPath "." -Filter "CC_*.C" -File
foreach ($src in $compilerSources) {
    $srcText = Get-Content -LiteralPath $src.FullName -Raw
    if ($srcText -match '(?im)^[^/\r\n]*\bswitch\s*\(') {
        throw "*** BUILD FAILED: bootstrap-unsafe internal switch() remains in $($src.Name)"
    }
}
Write-Output "Bootstrap audit: active compiler C sources contain no internal switch() dispatch."
$requiredCodes = @(
    "ADD12","ADDSP","ARGCNTn","ASL12","ASR12","LSR12","CALLm","DBL1","DBL2",
    "ENTER","POINT1m","GETb1m","GETb1mu","GETb1p","GETb1pu","GETw1m","GETw1n","GETw1p","GETw2n","BYTE_","BYTEn","BYTEr0","WORD_","WORDr0",
    "MOVE21","POINT1s","POP2","PUSH1","PUTbm1","PUTbp1","PUTwm1","RETURN","SUB12","SWAP12",
    "LT12u","LE12u","GT12u","GE12u","DIV12u","MOD12u"
)
foreach ($codeName in $requiredCodes) {
    if ($cgtText -notmatch ("code\[" + [regex]::Escape($codeName) + "\]\s*=")) {
        throw "*** BUILD FAILED: CC_CG99T missing required M38e template $codeName"
    }
}
if ($cgtText -notmatch 'code\[MOVE21\].*MOV R4,R3') {
    throw "*** BUILD FAILED: MOVE21 must move primary R4 to secondary R3"
}
if ($cgtText -notmatch 'code\[PUSH1\].*MOV R4,\*SP') {
    throw "*** BUILD FAILED: PUSH1 must push primary R4"
}
if ($cgtText -notmatch 'code\[POP2\].*MOV \*SP\+,R3') {
    throw "*** BUILD FAILED: POP2 must restore secondary R3"
}
# Unsigned arithmetic must stay distinct from signed helpers.  This is the
# tests\LANGTEST_LANGUAGE.C 311/312/313/315 regression contract.
if ($cgtText -notmatch 'code\[LSR12\]\s*=\s*"\.<b>usr<e>') {
    throw "*** BUILD FAILED: LSR12 must use the logical _ccusr helper"
}
if ($cgtText -notmatch 'code\[DIV12u\].*CLR R1.*MOV R3,R2.*DIV R4,R1.*MOV R1,R4') {
    throw "*** BUILD FAILED: DIV12u must use native unsigned TMS9900 DIV and return quotient in R4"
}
if ($cgtText -notmatch 'code\[MOD12u\].*CLR R1.*MOV R3,R2.*DIV R4,R1.*MOV R2,R4') {
    throw "*** BUILD FAILED: MOD12u must use native unsigned TMS9900 DIV and return remainder in R4"
}
if ($cgtText -notmatch 'code\[ENTER\].*STWP WP.*MOV FP,\*SP.*MOV SP,FP') {
    throw "*** BUILD FAILED: CC_CG99T ENTER template does not establish WP and FP"
}
if ($cgtText -notmatch 'code\[RETURN\].*MOV FP,SP.*MOV \*SP\+,FP.*RET') {
    throw "*** BUILD FAILED: CC_CG99T RETURN template does not restore SP from FP and exit with RET"
}
if ($cgtText -notmatch 'code\[CALLm\]\s*=\s*"\.\\tCALL @<m>') {
    throw "*** BUILD FAILED: CC_CG99T CALLm must emit CALL, not BL"
}
if ($cgtText -match 'code\[CALL1\]\s*=\s*"\.\\tBL ') {
    throw "*** BUILD FAILED: CC_CG99T CALL1 must emit CALL, not BL"
}
if ($cgtText -notmatch 'code\[SWITCH\]\s*=\s*"\.\\tCALL @_ccswitc<e>') {
    throw "*** BUILD FAILED: SWITCH must emit XOP6 CALL @_ccswitc##, not BL"
}
if ($cgtText -match 'code\[SWITCH\].*<b>switc') {
    throw "*** BUILD FAILED: SWITCH is incorrectly using the BL helper prefix"
}
# Each OVL_CGEN module must stay inside its own 4K virtual page. DREL reports
# both sizes below; the overlay as a whole now has 8K of address space.
if ($cgtText -match 'code\[(ENTER|RETURN|POINT1s)\].*R9') {
    throw "*** BUILD FAILED: generated frame templates still use R9 instead of FP"
}
$cdText = Get-Content -LiteralPath ".\CC_CD99.C" -Raw
if ($cdText -match 'gen\(SWAP12') {
    throw "*** BUILD FAILED: TMS staging still contains inherited x86 SUB/DIV/MOD pre-swap"
}
if ($cdText -notmatch 'outline\("FP\\tEQU 9"\)') {
    throw "*** BUILD FAILED: CC_CD99 does not emit FP EQU 9"
}
if ($cdText -match 'outline\("R9\\tEQU 9"\)') {
    throw "*** BUILD FAILED: CC_CD99 still emits R9 EQU 9"
}
if ($cdText -notmatch 'outline\("WP\\tEQU 13"\)') {
    throw "*** BUILD FAILED: CC_CD99 does not emit WP EQU 13"
}
if ($cdText -notmatch 'outline\("\\tDXOP CALL,6"\)') {
    throw "*** BUILD FAILED: CC_CD99 does not emit DXOP CALL,6"
}
if ($cdText -notmatch 'outline\("\\tDXOP RET,7"\)') {
    throw "*** BUILD FAILED: CC_CD99 does not emit DXOP RET,7"
}
if ($cgtText -notmatch 'code\[PUTbm1\].*MOVB @2\*R4\+1\(WP\),@<m>') {
    throw "*** BUILD FAILED: PUTbm1 does not store the low byte of primary R4"
}
if ($cgtText -notmatch 'code\[PUTbp1\].*MOVB @2\*R4\+1\(WP\),\*R3') {
    throw "*** BUILD FAILED: PUTbp1 does not store primary R4 through secondary pointer R3"
}
if ($cgtText -notmatch 'code\[GETb1mu\].*MOVB @<m>,R4.*SRL R4,8') {
    throw "*** BUILD FAILED: GETb1mu does not normalize primary R4 with SRL R4,8"
}
if ($cgtText -notmatch 'code\[GETb1pu\].*SRL R4,8') {
    throw "*** BUILD FAILED: GETb1pu does not use SRL R4,8"
}
if ($cgtText -match 'code\[(PUTbm1|PUTbp1)\].*SWPB') {
    throw "*** BUILD FAILED: byte stores still use SWPB instead of workspace-byte addressing"
}
if ($cgText -notmatch 'badcode\(pcode\)' -or $cgText -notmatch 'tmsfail = 1' -or $cgText -notmatch 'tmsbad = pcode') {
    throw "*** BUILD FAILED: CC_CG99 missing persistent unsupported-p-code failure flags"
}
if ($cgText -notmatch 'AORG 08000H' -or $cgtText -notmatch 'AORG 09000H') {
    throw "*** BUILD FAILED: OVL_CGEN pages are not anchored at >8000/>9000"
}
if ($cgText -match '(?m)^setcodes\(\)' -or $cgtText -notmatch '(?m)^setcodes\(\)' -or $cgtText -match '(?m)^ccout\(') {
    throw "*** BUILD FAILED: M38e OVL_CGEN split is not ccout=>page1, setcodes=>page2"
}
if ($cgtText -notmatch 'extern char \*code\[\];') {
    throw "*** BUILD FAILED: CC_CG99T does not reference page-1 code[] as an external"
}
$mainText = Get-Content -LiteralPath ".\SMALLC99.C" -Raw
if ($mainText -notmatch "compilation failed; output is not valid assembly") {
    throw "*** BUILD FAILED: SMALLC99 missing persistent backend-failure report"
}

Compile-C "CC_RESIDENT"
Compile-C "CC_SCAN_SYM"
Compile-C "CC_DECL"
Compile-C "CC_DFUN"
Compile-C "CC_STMT"
Compile-C "CC_PREP"
Compile-C "CC_MACS"
Compile-C "CC_STMT_R"
Compile-C "CC_EXPR_A"
Compile-C "CC_EXPR_B"
Compile-C "CC_EXPR_C"
Compile-C "CC_EXPR_D"
Compile-C "CC_CD99"
Compile-C "CC_CG99"
Compile-C "CC_CG99T"
Compile-C "CC_CL99"
Compile-C "CC_STRD"
Compile-C "CC_DATA"
Compile-C-Main "SMALLC99"

# -----------------------------------------------------------------------
# POST-BOOTSTRAP SEMANTICS AUDIT
#
# Do not trust source-only checks here.  smallcp.exe is a bootstrap compiler,
# so verify the A99 it actually generated before anything is assembled.
# This catches stale build products and bootstrap miscompilation.
# -----------------------------------------------------------------------
Require-File ".\CC_DATA.A99"
Require-File ".\CC_CG99.A99"
Require-File ".\CC_CG99T.A99"

$dataA99 = Get-Content -LiteralPath ".\CC_DATA.A99" -Raw
$cgA99   = Get-Content -LiteralPath ".\CC_CG99.A99" -Raw
$cgtA99  = Get-Content -LiteralPath ".\CC_CG99T.A99" -Raw

# The generated resident op2[] table is the actual signedness dispatcher.
# Index 9 MUST be 74 (LSR12); indices 14/15 MUST remain 17/50.
if ($dataA99 -notmatch '(?is)op2:.*?WORD[ \t]+57,73,3,20,55,44,23,48,36,74,6,1.*?WORD[ \t]+69,53,17,50') {
    throw "*** BUILD FAILED: generated CC_DATA.A99 does not contain unsigned op2 dispatch 74/17/50"
}

# PCODE 74 requires 75 two-byte entries.
if ($cgA99 -notmatch '(?im)^code:[ \t]+BSS[ \t]+150\b') {
    throw "*** BUILD FAILED: generated CC_CG99.A99 code[] is not 75 entries (150 bytes)"
}

function Require-GeneratedDirectTemplate {
    param(
        [Parameter(Mandatory=$true)][string]$Text,
        [Parameter(Mandatory=$true)][string]$CodeName,
        [Parameter(Mandatory=$true)][int]$ByteOffset,
        [Parameter(Mandatory=$true)][string]$NextCodeName
    )
    $start = $Text.IndexOf(";    code[$CodeName]")
    if ($start -lt 0) { throw "*** BUILD FAILED: generated CC_CG99T.A99 has no $CodeName assignment" }
    $stop = $Text.IndexOf(";    code[$NextCodeName]", $start + 1)
    if ($stop -lt 0) { throw "*** BUILD FAILED: cannot delimit generated $CodeName assignment" }
    $block = $Text.Substring($start, $stop - $start)
    if ($block -notmatch ("(?im)LI[ \t]+R3," + $ByteOffset + "\b")) {
        throw "*** BUILD FAILED: generated $CodeName assignment does not address code[] byte offset $ByteOffset"
    }
    # Any literal pool: setcodes() is split into setcodes1..7 (each within
    # SMALLC99's 255-byte literal queue), so templates load from cc<n>+N.
    if ($block -notmatch '(?im)LI[ \t]+R4,cc\d+\+\d+') {
        throw "*** BUILD FAILED: generated $CodeName is not a direct template-string assignment"
    }
    if ($block -match '(?i)indirect ccgint') {
        throw "*** BUILD FAILED: generated $CodeName still aliases another code[] entry"
    }
}

Require-GeneratedDirectTemplate -Text $cgtA99 -CodeName "LSR12"  -ByteOffset 148 -NextCodeName "CALL1"
Require-GeneratedDirectTemplate -Text $cgtA99 -CodeName "DIV12u" -ByteOffset 34  -NextCodeName "MOD12"
Require-GeneratedDirectTemplate -Text $cgtA99 -CodeName "MOD12u" -ByteOffset 100 -NextCodeName "EQ12"

Write-Output "Generated unsigned-semantics audit: op2 LSR12/DIV12u/MOD12u and 75-entry code table verified in A99."

$mainLog = $script:CompileLogs["SMALLC99"]
foreach ($fn in @("main","ask","compinit","openfile","firstline","closefile","finish")) {
    if ($mainLog -notmatch ("======\s+" + $fn + "\(\)")) {
        throw "*** BUILD FAILED: SMALLC99 did not compile $fn()"
    }
}
$cgLog = $script:CompileLogs["CC_CG99"]
foreach ($fn in @("badcode","ccout")) {
    if ($cgLog -notmatch ("======\s+" + $fn + "\(\)")) {
        throw "*** BUILD FAILED: CC_CG99 did not compile $fn()"
    }
}
$cgtLog = $script:CompileLogs["CC_CG99T"]
if ($cgtLog -notmatch "======\s+setcodes\(\)") {
    throw "*** BUILD FAILED: CC_CG99T did not compile setcodes()"
}
$cdLog = $script:CompileLogs["CC_CD99"]
foreach ($fn in @("setcodes","gen","public","external","header","trailer")) {
    if ($cdLog -notmatch ("======\s+" + $fn + "\(\)")) {
        throw "*** BUILD FAILED: CC_CD99 did not compile $fn()"
    }
}
$clLog = $script:CompileLogs["CC_CL99"]
if ($clLog -notmatch "======\s+getoptions\(\)") {
    throw "*** BUILD FAILED: CC_CL99 did not compile getoptions()"
}

Assemble-R99 "M38DPROBE"

foreach ($overlay in $OVERLAYS) { Assemble-R99 $overlay.Name }

$overlayFiles = @()
foreach ($overlay in $OVERLAYS) { $overlayFiles += "$($overlay.Name).R99" }
$overlayLint = & .\drel.exe -c @overlayFiles 2>&1 | Out-String
$overlayLintExit = $LASTEXITCODE
Write-Output "`n================================================"
Write-Output "Overlay chain lint"
Write-Output "================================================"
Write-Output $overlayLint
if ($overlayLint -notmatch "all chains conform" -or $overlayLintExit -ne 0) {
    throw "*** BUILD FAILED: TMS overlay chain lint"
}
if ($overlayLint -match '(?im)XREFS\s+group:\s+_ccswitc\b') {
    throw "*** BUILD FAILED: bootstrap-generated compiler overlay still references _ccswitc"
}
Write-Output "Bootstrap audit: compiler overlays have no _ccswitc XREFs."

$drelArgs = @()
foreach ($overlay in $OVERLAYS) { $drelArgs += $overlay.Descriptor }
$includeOutput = & .\drel.exe -i OVLADDR.INC @drelArgs 2>&1 | Out-String
$includeExit = $LASTEXITCODE
Write-Output "`n================================================"
Write-Output "Generating TMS overlay include files"
Write-Output "================================================"
Write-Output $includeOutput
if ($includeExit -ne 0) { throw "*** BUILD FAILED: DREL include generation" }
Require-File ".\OVLADDR.INC"
Require-File ".\OVLTABLE.INC"
$addrText = Get-Content -LiteralPath ".\OVLADDR.INC" -Raw
foreach ($anchor in @("P_CCOUT","P_SETCODES","P_GETOPTS")) {
    if ($addrText -notmatch ("\b" + $anchor + "\b")) {
        throw "*** BUILD FAILED: OVLADDR.INC does not contain $anchor"
    }
}
$tableText = Get-Content -LiteralPath ".\OVLTABLE.INC" -Raw

# DREL 4.0 currently emits only the first page for OVL_STMT when the
# second physical page is 14, even though LINK99 accepts -P14 and the
# target mapper supports pages 0..15.  CC_STMT_R is AORG >9000 and is
# linked on physical page 14, so make the generated runtime table match
# the actual link layout.  If DREL is later fixed and emits both pages,
# this block leaves the row unchanged.
if ($tableText -notmatch 'WORD\s+8,3,9,14,0,0,0,0,0,0\s*;ID 2') {
    $oldStmtRow = '(?im)^\s*WORD\s+8,3,0,0,0,0,0,0,0,0\s*;ID 2[^\r\n]*$'
    if ($tableText -notmatch $oldStmtRow) {
        throw "*** BUILD FAILED: unexpected DREL OVL_STMT ID 2 row"
    }
    $tableText = [regex]::Replace(
        $tableText,
        $oldStmtRow,
        "`tWORD`t8,3,9,14,0,0,0,0,0,0`t;ID 2 (2 pages)",
        [Text.RegularExpressions.RegexOptions]::IgnoreCase -bor [Text.RegularExpressions.RegexOptions]::Multiline
    )
    [IO.File]::WriteAllText((Resolve-Path ".\OVLTABLE.INC"), $tableText, [Text.Encoding]::ASCII)
}

$tableText = Get-Content -LiteralPath ".\OVLTABLE.INC" -Raw
$stmtRow = [regex]::Match($tableText, '(?im)^\s*WORD\s+8,3[^\r\n]*;ID 2[^\r\n]*$').Value
Write-Output ("OVL_STMT final row: " + $stmtRow)
if ($tableText -notmatch 'WORD\s+8,3,9,14,0,0,0,0,0,0\s*;ID 2') {
    throw "*** BUILD FAILED: OVL_STMT ID 2 is not mapped as >8000/page3 + >9000/page14"
}

# HARD INVARIANT: every normal/nestable overlay must own segment 8.
# Parse OVLTABLE line-by-line.  This is intentionally simple and bounded:
# no whole-file regex, no multiline whitespace, no backtracking across rows.
$tableLines = Get-Content -LiteralPath ".\OVLTABLE.INC"
$overlayIds = @()
foreach ($line in $tableLines) {
    $idMatch = [regex]::Match(
        $line,
        '^[ \t]*WORD[ \t]+[0-9, \t]+[ \t]*;[^\r\n]*\bID[ \t]+(\d+)\b',
        [Text.RegularExpressions.RegexOptions]::IgnoreCase
    )
    if ($idMatch.Success) {
        $overlayIds += [int]$idMatch.Groups[1].Value
    }
}
if ($overlayIds.Count -eq 0) {
    throw "*** BUILD FAILED: no overlay rows found in OVLTABLE.INC"
}
foreach ($id in $overlayIds) {
    if ($id -eq 0 -or $OVERLAY_OWNER_EXCEPTIONS -contains $id) { continue }

    $words = Get-OverlayRowWords -TableLines $tableLines -Id $id
    if ($words[0] -ne 8) {
        throw "*** BUILD FAILED: overlay ID $id violates window-owner invariant: first mapped segment is $($words[0]), expected 8"
    }

    $segment8Count = 0
    foreach ($index in @(0,2,4,6,8)) {
        if ($words[$index] -eq 8) { $segment8Count++ }
    }
    if ($segment8Count -ne 1) {
        throw "*** BUILD FAILED: overlay ID $id must map segment 8 exactly once"
    }
}
Write-Output "Overlay ownership audit: every non-exempt overlay begins at segment 8 and owns it exactly once."

# CGEN is the regression that established this invariant.  Keep both its
# physical pages and its virtual owner slots exact.
$cgenWords = Get-OverlayRowWords -TableLines $tableLines -Id 6
$expectedCgen = @(8,11,9,13,0,0,0,0,0,0)
for ($i = 0; $i -lt 10; $i++) {
    if ($cgenWords[$i] -ne $expectedCgen[$i]) {
        throw "*** BUILD FAILED: OVL_CGEN ID 6 must be exactly 8,11,9,13,0,0,0,0,0,0"
    }
}

# The exported CGEN entry points must agree with the table: ccout in segment 8,
# templates/setcodes in segment 9.  This catches an AORG regression even if a
# hand-edited/generated table happens to look correct.
$ccoutAddr = Get-OverlayAnchor -AddrText $addrText -Symbol "P_CCOUT"
$setcodesAddr = Get-OverlayAnchor -AddrText $addrText -Symbol "P_SETCODES"
if ($ccoutAddr -lt 0x8000 -or $ccoutAddr -gt 0x8FFF) {
    throw ("*** BUILD FAILED: P_CCOUT is >{0:X4}; expected >8000..>8FFF" -f $ccoutAddr)
}
if ($setcodesAddr -lt 0x9000 -or $setcodesAddr -gt 0x9FFF) {
    throw ("*** BUILD FAILED: P_SETCODES is >{0:X4}; expected >9000..>9FFF" -f $setcodesAddr)
}
Write-Output ("CGEN ownership audit: ID 6 = 8,11,9,13; P_CCOUT=>{0:X4}; P_SETCODES=>{1:X4}." -f $ccoutAddr,$setcodesAddr)

if ($tableText -notmatch 'WORD\s+10,12,0,0,0,0,0,0,0,0\s*;ID 7') {
    throw "*** BUILD FAILED: overlay ID 7 is not mapped to page 12"
}

foreach ($name in @("SMALLC99","CC_PORT","CLISTUB","CC_CD99","CC_RESIDENT","CC_SCAN_SYM","CC_DATA","OVLMGR","OVLSTUBS","SPYOUT","CC_STRU")) {
    Assemble-R99 $name
}

$residentFiles = @("SMALLC99.R99","CC_PORT.R99","CLISTUB.R99","CC_CD99.R99","CC_RESIDENT.R99","CC_SCAN_SYM.R99","CC_DATA.R99","CC_STRU.R99")
$residentLint = & .\drel.exe -c @residentFiles 2>&1 | Out-String
$residentLintExit = $LASTEXITCODE
Write-Output "`n================================================"
Write-Output "Resident chain lint"
Write-Output "================================================"
Write-Output $residentLint
if ($residentLint -notmatch "all chains conform" -or $residentLintExit -ne 0) {
    throw "*** BUILD FAILED: TMS resident chain lint"
}

Write-Output "`n================================================"
Write-Output "Overlay budgets"
Write-Output "================================================"
foreach ($overlay in $OVERLAYS) {
    $pattern = "MODULE\s+" + [regex]::Escape($overlay.Name) + "\s+size=([0-9A-F]+)"
    $match = [regex]::Match($overlayLint, $pattern, [Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if (-not $match.Success) { throw "*** BUILD FAILED: no size for $($overlay.Name)" }
    $used = [Convert]::ToInt32($match.Groups[1].Value,16)
    $chainFree = 0x0FC0 - $used
    Write-Output ("  {0,-10} used {1,4} chain-free {2,4}" -f $overlay.Name,$used,$chainFree)
    if ($chainFree -lt 0) {
        throw "*** BUILD FAILED: $($overlay.Name) exceeds LINK99 chain-safe limit >0FC0"
    }
}

$pageArgs = @()
foreach ($overlay in $OVERLAYS) {
    $pageArgs += "-P$($overlay.Page)"
    $pageArgs += "$($overlay.Name).R99"
}

$linkArgs = @(
    "-O1000", "-M", "-S", "SMALLC99.EXE",
    "SMALLC99.R99", "CC_PORT.R99", "CLISTUB.R99", "OVLMGR.R99", "OVLSTUBS.R99",
    "CC_RESIDENT.R99", "CC_SCAN_SYM.R99", "CC_CD99.R99", "CC_DATA.R99"
)
$linkArgs += $RUNTIME_OBJECTS
$linkArgs += "SPYOUT.R99"
$linkArgs += "CC_STRU.R99"          # structs: resident elsize()
$linkArgs += $pageArgs
$linkArgs += $RUNTIME_LIBRARIES

Write-Output "`n================================================"
Write-Output "LINK99 command: SMALLC99"
Write-Output "================================================"
Write-Output (".\link99.exe " + ($linkArgs -join " "))
$linkOutput = & .\link99.exe @linkArgs 2>&1 | Out-String
$linkExit = $LASTEXITCODE
Write-Output $linkOutput
$linkOutput | Set-Content -LiteralPath ".\LINK.LOG"   # kept for tools\sc99.ps1 (symbol addresses)
if ($linkOutput -match '(?im)^\s*-\s*Unresolved:' -or
    $linkOutput -match '(?im)^\s*-\s*Error:' -or
    $linkOutput -match '(?im)^\s*\*{3}\s*FATAL\b' -or
    $linkExit -ne 0) {
    throw "*** BUILD FAILED: SMALLC99 link"
}
if ($linkOutput -notmatch ("Version\s+" + [regex]::Escape($LINK99_VER) + "\b")) {
    throw "*** BUILD FAILED: wrong LINK99 version"
}
foreach ($overlay in $OVERLAYS) {
    if ($linkOutput -notmatch ("Page mode: curpage=\s*" + $overlay.Page + "\s")) {
        throw "*** BUILD FAILED: LINK99 did not report page $($overlay.Page) for $($overlay.Name)"
    }
}
Require-File ".\SMALLC99.EXE"

if (-not (Test-Path -LiteralPath $MONITOR_DIR -PathType Container)) {
    throw "*** BUILD FAILED: monitor destination does not exist: $MONITOR_DIR"
}
$exeDest = Join-Path $MONITOR_DIR "SMALLC99.EXE"
$srcDest = Join-Path $MONITOR_DIR "M38B.C"
$testDest = Join-Path $MONITOR_DIR "TEST.C"
$byteDest = Join-Path $MONITOR_DIR "M38D.C"
$langTestDest = Join-Path $MONITOR_DIR "LANGTEST.C"
$strTestDest = Join-Path $MONITOR_DIR "STRTEST.C"
$ptrTestDest = Join-Path $MONITOR_DIR "PTRTEST.C"
$arrTestDest = Join-Path $MONITOR_DIR "ARRTEST.C"
Copy-Item -LiteralPath ".\SMALLC99.EXE" -Destination $exeDest -Force
Copy-Item -LiteralPath ".\M38B.C" -Destination $srcDest -Force
Copy-Item -LiteralPath ".\TEST.C" -Destination $testDest -Force
Copy-Item -LiteralPath ".\M38D.C" -Destination $byteDest -Force
Copy-Item -LiteralPath "..\tests\LANGTEST_LANGUAGE.C" -Destination $langTestDest -Force
Copy-Item -LiteralPath "..\tests\STRUCTTEST.C" -Destination $strTestDest -Force
Copy-Item -LiteralPath "..\tests\PTRTEST.C" -Destination $ptrTestDest -Force
Copy-Item -LiteralPath "..\tests\ARRTEST.C" -Destination $arrTestDest -Force
if ((Get-FileHash ".\SMALLC99.EXE" -Algorithm SHA256).Hash -ne (Get-FileHash $exeDest -Algorithm SHA256).Hash) {
    throw "*** BUILD FAILED: deployed SMALLC99 hash mismatch"
}
if ((Get-FileHash ".\M38B.C" -Algorithm SHA256).Hash -ne (Get-FileHash $srcDest -Algorithm SHA256).Hash) {
    throw "*** BUILD FAILED: deployed M38B.C hash mismatch"
}
if ((Get-FileHash ".\TEST.C" -Algorithm SHA256).Hash -ne (Get-FileHash $testDest -Algorithm SHA256).Hash) {
    throw "*** BUILD FAILED: deployed TEST.C hash mismatch"
}
if ((Get-FileHash ".\M38D.C" -Algorithm SHA256).Hash -ne (Get-FileHash $byteDest -Algorithm SHA256).Hash) {
    throw "*** BUILD FAILED: deployed M38D.C hash mismatch"
}
if ((Get-FileHash "..\tests\LANGTEST_LANGUAGE.C" -Algorithm SHA256).Hash -ne (Get-FileHash $langTestDest -Algorithm SHA256).Hash) {
    throw "*** BUILD FAILED: deployed LANGTEST.C hash mismatch"
}

# -------------------------------------------------------
# Release directory lock back to script root before exiting
# -------------------------------------------------------
Set-Location -LiteralPath $PSScriptRoot


Write-Output "`n================================================"
Write-Output "BUILD SUCCESSFUL - MILESTONE 38e TWO-PAGE CODEGEN OVERLAY - "
Write-Output "================================================"
Write-Output "SMALLC99.EXE deployed to:"
Write-Output $exeDest
Write-Output "M38B.C deployed to:"
Write-Output $srcDest
Write-Output "TEST.C deployed to:"
Write-Output $testDest
Write-Output "M38D.C deployed to:"
Write-Output $byteDest
Write-Output "Run minimal: SMALLC99 M38B M38B /V"
Write-Output "Run word core: SMALLC99 TEST TEST /V"
Write-Output "Run byte test: SMALLC99 M38D M38D /V"
Write-Output "Expected output files: M38B.A99, TEST.A99 and M38D.A99"

# Explicitly terminate the dedicated PowerShell build host on success.
exit 0