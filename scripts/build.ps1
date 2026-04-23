param(
  [string]$CMakePath = "C:/temp/cmake_versions/cmake-4.3.1/bin/cmake.exe",
  [string]$QtPrefix = "C:/CraftRoot_KF6",
  [string]$BuildDir = "build",
  [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

function Stop-RunningAppIfLocked {
  param(
    [string]$RepoRoot,
    [string]$BuildDirectory,
    [string]$BuildConfig
  )

  $targetExe = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot "$BuildDirectory/src/$BuildConfig/LayeredPaintApp.exe"))
  $running = Get-Process LayeredPaintApp -ErrorAction SilentlyContinue
  if ($null -eq $running) {
    return
  }

  foreach ($proc in $running) {
    try {
      if ($proc.Path -and ([System.IO.Path]::GetFullPath($proc.Path) -ieq $targetExe)) {
        Stop-Process -Id $proc.Id -Force
        Write-Host "Stopped running app to avoid linker lock: $targetExe"
      }
    } catch {
      # Ignore processes we cannot inspect (permissions / exited)
    }
  }
}

if (!(Test-Path $CMakePath)) {
  throw "CMake executable not found: $CMakePath"
}

Push-Location $repoRoot
try {
  Stop-RunningAppIfLocked -RepoRoot $repoRoot -BuildDirectory $BuildDir -BuildConfig $Config

  & $CMakePath -S . -B $BuildDir "-DCMAKE_PREFIX_PATH=$QtPrefix" "-DPAINT_BUILD_TESTS=OFF"
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  & $CMakePath --build $BuildDir --config $Config
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Write-Host "Build completed: $BuildDir ($Config)"
} finally {
  Pop-Location
}
