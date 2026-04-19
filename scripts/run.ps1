param(
  [string]$CMakePath = "C:/temp/cmake_versions/cmake-4.3.1/bin/cmake.exe",
  [string]$QtPrefix = "C:/CraftRoot_KF6",
  [string]$BuildDir = "build",
  [string]$Config = "Release",
  [string]$WinDeployQtPath = "C:/CraftRoot_KF6/bin/windeployqt.exe"
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildScript = Join-Path $PSScriptRoot "build.ps1"

if (!(Test-Path $buildScript)) {
  throw "build.ps1 not found: $buildScript"
}
if (!(Test-Path $WinDeployQtPath)) {
  throw "windeployqt not found: $WinDeployQtPath"
}

& powershell -ExecutionPolicy Bypass -File $buildScript -CMakePath $CMakePath -QtPrefix $QtPrefix -BuildDir $BuildDir -Config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$candidatePaths = @(
  (Join-Path $repoRoot "$BuildDir/src/$Config/LayeredPaintApp.exe"),
  (Join-Path $repoRoot "$BuildDir/src/LayeredPaintApp.exe")
)

$exePath = $candidatePaths | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $exePath) {
  $exePath = Get-ChildItem -Path (Join-Path $repoRoot $BuildDir) -Filter "LayeredPaintApp.exe" -Recurse -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1 -ExpandProperty FullName
}
if (-not $exePath) {
  throw "LayeredPaintApp.exe not found under build directory: $BuildDir"
}

$deployMode = if ($Config -ieq "Debug") { "--debug" } else { "--release" }
& $WinDeployQtPath $deployMode --no-translations --compiler-runtime $exePath
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$env:PATH = "$QtPrefix/bin;$env:PATH"
$env:QT_PLUGIN_PATH = "$QtPrefix/plugins"
Start-Process -FilePath $exePath -WorkingDirectory (Split-Path -Parent $exePath) | Out-Null
Write-Host "Application launched: $exePath"
