[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$artifactDirectory = Join-Path $projectRoot 'artifacts\tests'
$outputPath = Join-Path $artifactDirectory 'DA2CinematicUIHiderTests.exe'
$zig = & (Join-Path $PSScriptRoot 'find-zig.ps1')

New-Item -ItemType Directory -Force -Path $artifactDirectory | Out-Null
$sources = @(
    (Join-Path $projectRoot 'tests\test_main.cpp'),
    (Join-Path $projectRoot 'src\core\hold_state.cpp'),
    (Join-Path $projectRoot 'src\core\key_chord.cpp'),
    (Join-Path $projectRoot 'src\core\da2_bindings.cpp')
)

& $zig c++ -target x86-windows-gnu -std=c++17 -O2 `
    ('-I' + (Join-Path $projectRoot 'src')) @sources -o $outputPath
if ($LASTEXITCODE -ne 0) {
    throw "Test build failed with exit code $LASTEXITCODE."
}

& $outputPath
if ($LASTEXITCODE -ne 0) {
    throw "Tests failed with exit code $LASTEXITCODE."
}
