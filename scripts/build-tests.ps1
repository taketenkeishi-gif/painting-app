param(
  [string]$CMakePath = "C:/temp/cmake_versions/cmake-4.3.1/bin/cmake.exe",
  [string]$QtPrefix = "C:/CraftRoot_KF6",
  [string]$BuildDir = "build-tests",
  [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"

if (!(Test-Path $CMakePath)) {
  throw "CMake executable not found: $CMakePath"
}

$ctestPath = Join-Path (Split-Path $CMakePath -Parent) "ctest.exe"
if (!(Test-Path $ctestPath)) {
  throw "CTest executable not found next to CMake: $ctestPath"
}

& $CMakePath -S . -B $BuildDir "-DCMAKE_PREFIX_PATH=$QtPrefix" "-DPAINT_BUILD_TESTS=ON"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $CMakePath --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $ctestPath --test-dir $BuildDir --output-on-failure -C $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Build + tests completed: $BuildDir ($Config)"
