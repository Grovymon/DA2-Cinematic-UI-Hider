[CmdletBinding()]
param(
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$artifactRoot = [System.IO.Path]::GetFullPath((Join-Path $projectRoot 'artifacts'))
$expectedParent = [System.IO.Path]::GetFullPath($projectRoot)
if (-not $artifactRoot.StartsWith($expectedParent + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to use artifact directory outside the project: $artifactRoot"
}

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot 'test.ps1')
    & (Join-Path $PSScriptRoot 'build.ps1')
    & (Join-Path $PSScriptRoot 'smoke-test.ps1')
}

$asiPath = Join-Path $artifactRoot 'DA2CinematicUIHider.asi'
if (-not (Test-Path -LiteralPath $asiPath)) {
    throw "Built ASI was not found: $asiPath"
}

$loaderUrl = 'https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/v9.7.4/Ultimate-ASI-Loader.zip'
$loaderExpectedHash = '952cebfc30d525afc2bdbaca954329d405ded3aa688a83027354dae14dfd5c5f'
$cacheDirectory = Join-Path $artifactRoot 'cache'
$loaderArchive = Join-Path $cacheDirectory 'Ultimate-ASI-Loader-v9.7.4.zip'
New-Item -ItemType Directory -Force -Path $cacheDirectory | Out-Null
if (-not (Test-Path -LiteralPath $loaderArchive)) {
    Invoke-WebRequest -Uri $loaderUrl -OutFile $loaderArchive
}
$loaderArchiveHash = (Get-FileHash -LiteralPath $loaderArchive -Algorithm SHA256).Hash.ToLowerInvariant()
if ($loaderArchiveHash -ne $loaderExpectedHash) {
    throw "Ultimate ASI Loader archive SHA-256 mismatch: $loaderArchiveHash"
}

$loaderExtractDirectory = Join-Path $artifactRoot 'loader-v9.7.4'
if (-not (Test-Path -LiteralPath (Join-Path $loaderExtractDirectory 'dinput8.dll'))) {
    if (Test-Path -LiteralPath $loaderExtractDirectory) {
        Remove-Item -LiteralPath $loaderExtractDirectory -Recurse -Force
    }
    Expand-Archive -LiteralPath $loaderArchive -DestinationPath $loaderExtractDirectory
}
$loaderDll = Join-Path $loaderExtractDirectory 'dinput8.dll'
if (-not (Test-Path -LiteralPath $loaderDll)) {
    throw 'The verified Ultimate ASI Loader archive did not contain dinput8.dll.'
}

$stagingRoot = Join-Path $artifactRoot 'package'
$stagingDirectory = Join-Path $stagingRoot 'DA2-Cinematic-UI-Hider'
$binShipDirectory = Join-Path $stagingDirectory 'bin_ship'
$docsDirectory = Join-Path $stagingDirectory 'docs'
$thirdPartyDirectory = Join-Path $stagingDirectory 'THIRD_PARTY_LICENSES'
$archivePath = Join-Path $artifactRoot 'DA2-Cinematic-UI-Hider-v0.1.0.zip'

if (Test-Path -LiteralPath $stagingRoot) {
    Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}
if (Test-Path -LiteralPath $archivePath) {
    Remove-Item -LiteralPath $archivePath -Force
}
New-Item -ItemType Directory -Force -Path $binShipDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $docsDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $thirdPartyDirectory | Out-Null

Copy-Item -LiteralPath $asiPath -Destination $binShipDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot 'DA2CinematicUIHider.ini') -Destination $binShipDirectory
Copy-Item -LiteralPath $loaderDll -Destination $binShipDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot 'README.md') -Destination $stagingDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot 'README_RU.md') -Destination $stagingDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot 'LICENSE') -Destination $stagingDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot 'docs\IN-GAME-TEST-CHECKLIST.md') -Destination $docsDirectory
Copy-Item -LiteralPath (Join-Path $projectRoot 'THIRD_PARTY_LICENSES\UltimateASILoader_LICENSE.md') -Destination $thirdPartyDirectory
New-Item -ItemType File -Force -Path (Join-Path $stagingDirectory 'EXTRACT_TO_DRAGON_AGE_II_FOLDER') | Out-Null

$loaderIni = "[GlobalSets]`r`nDontLoadFromDllMain=0`r`n"
[System.IO.File]::WriteAllText((Join-Path $binShipDirectory 'dinput8.ini'), $loaderIni, [System.Text.UTF8Encoding]::new($false))

$asiHash = (Get-FileHash -LiteralPath (Join-Path $binShipDirectory 'DA2CinematicUIHider.asi') -Algorithm SHA256).Hash.ToLowerInvariant()
$loaderDllHash = (Get-FileHash -LiteralPath (Join-Path $binShipDirectory 'dinput8.dll') -Algorithm SHA256).Hash.ToLowerInvariant()
$checksums = @(
    "$asiHash  bin_ship/DA2CinematicUIHider.asi",
    "$loaderDllHash  bin_ship/dinput8.dll"
) -join "`r`n"
[System.IO.File]::WriteAllText((Join-Path $stagingDirectory 'SHA256SUMS.txt'), $checksums + "`r`n", [System.Text.UTF8Encoding]::new($false))

$loaderSourceNotice = @"
Ultimate ASI Loader v9.7.4
Source and releases: https://github.com/ThirteenAG/Ultimate-ASI-Loader
Downloaded archive: $loaderUrl
Verified source archive SHA-256: $loaderExpectedHash
Included dinput8.dll SHA-256: $loaderDllHash
License: MIT; see UltimateASILoader_LICENSE.md in this directory.
"@
[System.IO.File]::WriteAllText((Join-Path $thirdPartyDirectory 'UltimateASILoader_SOURCE.txt'), $loaderSourceNotice.Trim() + "`r`n", [System.Text.UTF8Encoding]::new($false))

Compress-Archive -Path (Join-Path $stagingDirectory '*') -DestinationPath $archivePath -CompressionLevel Optimal
$archive = Get-Item -LiteralPath $archivePath
$archiveHash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash
Write-Output "Packaged: $($archive.FullName)"
Write-Output "Size: $($archive.Length) bytes"
Write-Output "SHA-256: $archiveHash"
Write-Output "Ultimate ASI Loader source archive verified: $loaderArchiveHash"
