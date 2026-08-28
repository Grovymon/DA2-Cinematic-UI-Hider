[CmdletBinding()]
param()

$command = Get-Command zig -ErrorAction SilentlyContinue
if ($command) {
    return $command.Source
}

if ($env:ZIG_EXE -and (Test-Path -LiteralPath $env:ZIG_EXE)) {
    return [System.IO.Path]::GetFullPath($env:ZIG_EXE)
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$workspaceRoot = Split-Path -Parent (Split-Path -Parent $projectRoot)
$bundledCandidate = Join-Path $workspaceRoot 'work\tools\zig-0.16.0\zig.exe'
if (Test-Path -LiteralPath $bundledCandidate) {
    return $bundledCandidate
}

throw 'Zig was not found. Install Zig 0.16+, put zig.exe on PATH, or set ZIG_EXE.'
