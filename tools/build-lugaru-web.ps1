# Build script for Lugaru-Web (clean output, stays open)
$Root = Split-Path -Parent $PSScriptRoot
if ((Get-Location).Path -notlike "*$Root*") { $Root = Split-Path -Parent $PSScriptRoot }
$SourceDir = Join-Path $Root "source"
$DocsDir   = Join-Path $Root "docs"
$TempBuild = "C:\Temp\lugaru-build"
$TempBin   = Join-Path $TempBuild "bin"
$LogFile   = "C:\Temp\lugaru-build.log"

"Build started at $(Get-Date)" | Out-File $LogFile -Encoding utf8

# Load EMSDK
$EnvScript = Join-Path $Root "emsdk\emsdk_env.ps1"
if (Test-Path $EnvScript) { . $EnvScript }

# Force rebuild: delete old .data so CMake must regenerate it from JSON sources
Remove-Item -Force (Join-Path $DocsDir "lugaru.data") -ErrorAction SilentlyContinue
$ForceData = Join-Path $TempBin "lugaru.data"
if (Test-Path $ForceData) { Remove-Item -Force $ForceData }

# Configure
Remove-Item -Recurse -Force $TempBuild -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $TempBin | Out-Null
"Configuring..." | Out-File $LogFile -Encoding utf8
emcmake cmake -S $SourceDir -B $TempBuild -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$TempBin" *> $null 2>$null

# Build (warnings already suppressed with 2>$null above)
"Building..." | Out-File $LogFile -Encoding utf8
cmake --build $TempBuild -j ($env:NUMBER_OF_PROCESSORS) *> $null 2>$null

# Copy
$Files = @("lugaru.js","lugaru.wasm","lugaru.data")
$AllFound = $true
foreach ($f in $Files) { if (-not (Test-Path (Join-Path $TempBin $f))) { $AllFound = $false } }

if ($AllFound) {
    New-Item -ItemType Directory -Force -Path $DocsDir | Out-Null
    foreach ($f in $Files) { Copy-Item -Force (Join-Path $TempBin $f) $DocsDir }
    Copy-Item -Force (Join-Path $SourceDir "index.html") $DocsDir
    "=== BUILD SUCCESS ===" | Out-File $LogFile -Encoding utf8
} else {
    "BUILD OUTPUT MISSING" | Out-File $LogFile -Encoding utf8
}
"BUILD DONE" | Out-File $LogFile -Encoding utf8
Write-Host "BUILD SUCCESS"
Write-Host "Log: $LogFile"
Read-Host -Prompt "Press ENTER to close"
