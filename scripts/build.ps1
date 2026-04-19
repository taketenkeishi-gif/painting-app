param(
  [string]$CMakePath = "C:/temp/cmake_versions/cmake-4.3.1/bin/cmake.exe",
  [string]$QtPrefix = "C:/CraftRoot_KF6",
  [string]$BuildDir = "build",
  [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

if (!(Test-Path $CMakePath)) {
  throw "CMake executable not found: $CMakePath"
}

Push-Location $repoRoot
try {
  & $CMakePath -S . -B $BuildDir "-DCMAKE_PREFIX_PATH=$QtPrefix" "-DPAINT_BUILD_TESTS=OFF"
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  & $CMakePath --build $BuildDir --config $Config
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Write-Host "Build completed: $BuildDir ($Config)"
} finally {
  Pop-Location
}
