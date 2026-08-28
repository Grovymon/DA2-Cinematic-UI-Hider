[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$artifactDirectory = Join-Path $projectRoot 'artifacts'
$outputPath = Join-Path $artifactDirectory 'DA2CinematicUIHider.asi'
$linkerOutputPath = Join-Path $artifactDirectory 'DA2CinematicUIHider.dll'
$zig = & (Join-Path $PSScriptRoot 'find-zig.ps1')

New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null
$sources = @(
    (Join-Path $projectRoot 'src\mod.cpp'),
    (Join-Path $projectRoot 'src\core\hold_state.cpp'),
    (Join-Path $projectRoot 'src\core\key_chord.cpp'),
    (Join-Path $projectRoot 'src\core\da2_bindings.cpp')
)

& $zig c++ -target x86-windows-gnu -std=c++17 -O2 -shared `
    ('-I' + (Join-Path $projectRoot 'src')) @sources -luser32 -lshell32 -o $linkerOutputPath
if ($LASTEXITCODE -ne 0) {
    throw "ASI build failed with exit code $LASTEXITCODE."
}
Move-Item -LiteralPath $linkerOutputPath -Destination $outputPath -Force

$bytes = [System.IO.File]::ReadAllBytes($outputPath)
if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
    throw 'Built ASI is not a valid PE file.'
}
$peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
$machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
if ($machine -ne 0x014C) {
    throw ('Built ASI is not x86 PE32. Machine=0x{0:X4}' -f $machine)
}

$file = Get-Item -LiteralPath $outputPath
$hash = Get-FileHash -LiteralPath $outputPath -Algorithm SHA256
Write-Output "Built: $($file.FullName)"
Write-Output "Architecture: PE32 x86 (0x014C)"
Write-Output "Size: $($file.Length) bytes"
Write-Output "SHA-256: $($hash.Hash)"
