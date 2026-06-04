param(
  [string]$CMakePath = "",
  [string]$QtPrefix  = "",
  [string]$BuildDir  = "build-tests",
  [string]$Config    = "Debug"
)

$ErrorActionPreference = "Stop"
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

# ── Auto-detect cmake ────────────────────────────────────────────────────────
if (-not $CMakePath) {
  $found = Get-Command cmake -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
  if ($found) { $CMakePath = $found }
}
if (-not $CMakePath -or -not (Test-Path $CMakePath)) {
  throw "cmake not found. Install CMake and ensure it is on PATH, or pass -CMakePath."
}

# ── Auto-detect Qt prefix ────────────────────────────────────────────────────
if (-not $QtPrefix) {
  $candidates = @(
    $env:QTDIR,
    $env:Qt6_DIR,
    "C:\Qt\6.7.2\msvc2019_64",
    "C:\Qt\6.8.0\msvc2019_64",
    "$env:USERPROFILE\6.7.2\msvc2019_64",
    "$env:USERPROFILE\6.8.0\msvc2019_64"
  )
  foreach ($c in $candidates) {
    if ($c -and (Test-Path (Join-Path $c "bin\qmake.exe"))) {
      $QtPrefix = $c
      break
    }
  }
}
if (-not $QtPrefix -or -not (Test-Path (Join-Path $QtPrefix "bin\qmake.exe"))) {
  throw "Qt not found. Set QTDIR env var to your Qt msvc2019_64 prefix, or pass -QtPrefix."
}

# ── Auto-detect MSVC vcvars64 ────────────────────────────────────────────────
$vcvarsPath = ""
$vsSearchRoots = @(
  "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools",
  "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community",
  "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Professional",
  "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Enterprise",
  "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools",
  "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community"
)
foreach ($root in $vsSearchRoots) {
  $candidate = Join-Path $root "VC\Auxiliary\Build\vcvars64.bat"
  if (Test-Path $candidate) { $vcvarsPath = $candidate; break }
}

$ctestPath = Join-Path (Split-Path $CMakePath -Parent) "ctest.exe"
if (-not (Test-Path $ctestPath)) {
  throw "ctest not found next to cmake: $ctestPath"
}

Write-Host "cmake  : $CMakePath"
Write-Host "ctest  : $ctestPath"
Write-Host "Qt     : $QtPrefix"
Write-Host "MSVC   : $(if ($vcvarsPath) { $vcvarsPath } else { '(not found — assuming environment already set)' })"
Write-Host "Build  : $BuildDir ($Config)"

# ── Kill running app to release linker lock ──────────────────────────────────
function Stop-AppIfLocked {
  param([string]$RepoRoot, [string]$BuildDirectory, [string]$BuildConfig)
  $exe = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot "$BuildDirectory\src\$BuildConfig\LayeredPaintApp.exe"))
  $procs = Get-Process LayeredPaintApp -ErrorAction SilentlyContinue
  if (-not $procs) { return }
  foreach ($p in $procs) {
    try {
      if ($p.Path -and ([System.IO.Path]::GetFullPath($p.Path) -ieq $exe)) {
        Stop-Process -Id $p.Id -Force
        Write-Host "Stopped running app: $exe"
      }
    } catch {}
  }
}

Stop-AppIfLocked -RepoRoot $repoRoot -BuildDirectory $BuildDir -BuildConfig $Config

# ── Configure + Build + Test ─────────────────────────────────────────────────
$configureCmd = "`"$CMakePath`" -S `"$repoRoot`" -B `"$repoRoot\$BuildDir`" -G Ninja `"-DCMAKE_PREFIX_PATH=$QtPrefix`" -DPAINT_BUILD_TESTS=ON `"-DCMAKE_BUILD_TYPE=$Config`""
$buildCmd     = "`"$CMakePath`" --build `"$repoRoot\$BuildDir`" --config $Config"
$testCmd      = "`"$ctestPath`" --test-dir `"$repoRoot\$BuildDir`" --output-on-failure -C $Config"

if ($vcvarsPath) {
  $combined = "call `"$vcvarsPath`" && $configureCmd && $buildCmd && $testCmd"
  cmd /c $combined
  $exitCode = $LASTEXITCODE
} else {
  Invoke-Expression $configureCmd
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  Invoke-Expression $buildCmd
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  Invoke-Expression $testCmd
  $exitCode = $LASTEXITCODE
}

if ($exitCode -ne 0) { exit $exitCode }
Write-Host "Build + tests completed: $BuildDir ($Config)"
