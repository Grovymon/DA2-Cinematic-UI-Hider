[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$artifactDirectory = Join-Path $projectRoot 'artifacts\smoke'
$harnessPath = Join-Path $artifactDirectory 'DragonAge2.exe'
$logPath = Join-Path $artifactDirectory 'DA2CinematicUIHider.log'
$asiPath = Join-Path $projectRoot 'artifacts\DA2CinematicUIHider.asi'
$zig = & (Join-Path $PSScriptRoot 'find-zig.ps1')

if (-not (Test-Path -LiteralPath $asiPath)) {
    throw "Built ASI was not found: $asiPath"
}

New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null
Copy-Item -LiteralPath $asiPath -Destination (Join-Path $artifactDirectory 'DA2CinematicUIHider.asi') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'tests\smoke\DA2CinematicUIHider.ini') -Destination $artifactDirectory -Force
if (Test-Path -LiteralPath $logPath) {
    Remove-Item -LiteralPath $logPath -Force
}

& $zig c++ -target x86-windows-gnu -std=c++17 -O2 `
    (Join-Path $projectRoot 'tests\load_smoke.cpp') -o $harnessPath
if ($LASTEXITCODE -ne 0) {
    throw "Load smoke harness build failed with exit code $LASTEXITCODE."
}

& $harnessPath
if ($LASTEXITCODE -ne 0) {
    throw "ASI LoadLibrary smoke test failed with exit code $LASTEXITCODE."
}
if (-not (Test-Path -LiteralPath $logPath)) {
    throw 'ASI load smoke test did not create its debug log.'
}

$log = Get-Content -LiteralPath $logPath -Raw
if ($log -notmatch 'DA2 Cinematic UI Hider v0\.1\.0 loaded' -or $log -notmatch 'Dragon Age II detected') {
    throw "ASI load smoke log is missing expected startup messages:`n$log"
}

Write-Output 'PASS: x86 Windows loaded DA2CinematicUIHider.asi via LoadLibrary.'
Write-Output 'PASS: ASI read its adjacent INI and accepted DragonAge2.exe as the host.'
Write-Output $log.Trim()
