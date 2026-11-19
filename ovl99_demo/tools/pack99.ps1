# pack99.ps1 -- bundle files into one PACK99 archive for the TMS99105 SBC.
#
#   & .\pack99.ps1 ARCHIVE.PAK file1 file2 dir\*.R99 ...
#
# XMODEM the archive to the SBC once, then:   UNPACK ARCHIVE
# UNPACK (UNPACK.A99) writes every file back out under its own name.
#
# Names on the SBC are 8.3, upper case, A-Z 0-9 and _ only: a longer name
# is cut to 8 + 3 characters (OVL_NAMEIO.R99 -> OVL_NAME.R99), exactly as
# the SBC's own FCB code cuts it, so a program that names OVL_NAMEIO.R99
# still finds it.  Two files that cut to the same name are refused.
#
# Format (UNPACK.A99 has the details): for each file one 512-byte header
# sector ('PK99', name, type, load address, sector count, length), then
# its data padded to whole sectors with >1A as an XMODEM transfer pads
# it; at the end one header sector with an empty name.
$ErrorActionPreference = "Stop"
if($args.Count -lt 2) { throw "usage: pack99.ps1 ARCHIVE.PAK file..." }
$out = [string]$args[0]
$files = @()
foreach($a in $args[1..($args.Count - 1)]) {
    $hit = @(Get-ChildItem -Path ([string]$a) -File)
    if($hit.Count -eq 0) { throw "no file matches $a" }
    $files += $hit
}
$types = @{ "COM" = 1; "EXE" = 2; "TXT" = 3; "BAS" = 4; "PRO" = 5 }
$seen = @{}
$ms = New-Object System.IO.MemoryStream
function Put-Header([byte[]]$name11, [int]$type, [int]$nsec, [long]$len) {
    $h = New-Object byte[] 512
    $h[0] = 0x50; $h[1] = 0x4B; $h[2] = 0x39; $h[3] = 0x39          # 'PK99'
    if($name11) { [Array]::Copy($name11, 0, $h, 4, 11) }
    $h[15] = $type
    $h[16] = 0x10; $h[17] = 0x00                                    # load address >1000
    $h[18] = ($nsec -shr 8) -band 0xFF; $h[19] = $nsec -band 0xFF
    $h[20] = ($len -shr 24) -band 0xFF; $h[21] = ($len -shr 16) -band 0xFF
    $h[22] = ($len -shr 8) -band 0xFF;  $h[23] = $len -band 0xFF
    $ms.Write($h, 0, 512)
}
Write-Output ("PACK99 " + $out)
foreach($f in $files) {
    $base = [IO.Path]::GetFileNameWithoutExtension($f.Name).ToUpper()
    $ext  = [IO.Path]::GetExtension($f.Name).TrimStart('.').ToUpper()
    if($base -notmatch '^[A-Z0-9_]+$' -or ($ext -ne '' -and $ext -notmatch '^[A-Z0-9_]+$')) {
        throw "$($f.Name): the SBC takes only A-Z 0-9 and _ in a name"
    }
    if($base.Length -gt 8) { $base = $base.Substring(0, 8) }
    if($ext.Length -gt 3)  { $ext  = $ext.Substring(0, 3) }
    $sbc = $base + $(if($ext) { "." + $ext } else { "" })
    if($seen.ContainsKey($sbc)) { throw "$($f.Name) and $($seen[$sbc]) are both $sbc on the SBC" }
    $seen[$sbc] = $f.Name
    $name11 = [Text.Encoding]::ASCII.GetBytes($base.PadRight(8) + $ext.PadRight(3))
    $type = 0
    if($types.ContainsKey($ext)) { $type = $types[$ext] }
    $data = [IO.File]::ReadAllBytes($f.FullName)
    $nsec = [int][Math]::Ceiling($data.Length / 512.0)
    if($nsec -gt 65535) { throw "$($f.Name) is too large" }
    Put-Header $name11 $type $nsec $data.Length
    $ms.Write($data, 0, $data.Length)
    $pad = $nsec * 512 - $data.Length
    for($i = 0; $i -lt $pad; $i++) { $ms.WriteByte(0x1A) }
    Write-Output ("  {0,-12} {1,7} bytes {2,4} sectors{3}" -f $sbc, $data.Length, $nsec, $(if($sbc -ne $f.Name.ToUpper()) { "   (" + $f.Name + ")" } else { "" }))
}
Put-Header $null 0 0 0                                              # end sector
[IO.File]::WriteAllBytes($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($out), $ms.ToArray())
Write-Output ("{0} files, {1} bytes -> {2}" -f $files.Count, $ms.Length, $out)
