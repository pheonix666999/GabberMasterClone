$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Join-Path $repoRoot "GabbermasterClone"
$outRoot = "H:\BUILDS\GC"
$buildDir = Join-Path $outRoot "build"

New-Item -ItemType Directory -Force -Path $outRoot | Out-Null
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

Write-Host "Configuring..." -ForegroundColor Cyan
cmake -S $projectDir -B $buildDir -DCMAKE_BUILD_TYPE=Release

Write-Host "Building..." -ForegroundColor Cyan
cmake --build $buildDir --config Release

$artefactsDir = Join-Path $buildDir "GabbermasterClone_artefacts\Release"
if (!(Test-Path $artefactsDir)) {
  throw "Build artefacts not found at $artefactsDir"
}

Write-Host "Copying artefacts to $outRoot..." -ForegroundColor Cyan
$dest = Join-Path $outRoot "artefacts\Release"
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item -Path (Join-Path $artefactsDir "*") -Destination $dest -Recurse -Force

Write-Host "Done." -ForegroundColor Green
Write-Host "VST3: $dest\VST3"
Write-Host "Standalone: $dest\Standalone"
