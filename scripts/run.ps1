param(
  [string]$CMakePath       = "",
  [string]$QtPrefix        = "",
  [string]$BuildDir        = "build",
  [string]$Config          = "Release",
  [string]$WinDeployQtPath = ""
)

$ErrorActionPreference = "Stop"
$repoRoot    = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildScript = Join-Path $PSScriptRoot "build.ps1"

if (-not (Test-Path $buildScript)) {
  throw "build.ps1 not found: $buildScript"
}

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

# ── Auto-detect windeployqt ──────────────────────────────────────────────────
if (-not $WinDeployQtPath) {
  $WinDeployQtPath = Join-Path $QtPrefix "bin\windeployqt.exe"
}
if (-not (Test-Path $WinDeployQtPath)) {
  throw "windeployqt not found at: $WinDeployQtPath"
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

Write-Host "cmake        : $CMakePath"
Write-Host "Qt           : $QtPrefix"
Write-Host "windeployqt  : $WinDeployQtPath"
Write-Host "MSVC         : $(if ($vcvarsPath) { $vcvarsPath } else { '(not found — assuming environment already set)' })"
Write-Host "Build        : $BuildDir ($Config)"

# ── Build ────────────────────────────────────────────────────────────────────
& powershell -ExecutionPolicy Bypass -File $buildScript `
    -CMakePath $CMakePath -QtPrefix $QtPrefix -BuildDir $BuildDir -Config $Config
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# ── Locate built exe ─────────────────────────────────────────────────────────
$candidatePaths = @(
  (Join-Path $repoRoot "$BuildDir\src\$Config\LayeredPaintApp.exe"),
  (Join-Path $repoRoot "$BuildDir\src\LayeredPaintApp.exe")
)
$exePath = $candidatePaths | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $exePath) {
  $exePath = Get-ChildItem -Path (Join-Path $repoRoot $BuildDir) -Filter "LayeredPaintApp.exe" -Recurse -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1 -ExpandProperty FullName
}
if (-not $exePath) {
  throw "LayeredPaintApp.exe not found under: $BuildDir"
}

# ── Deploy Qt DLLs ───────────────────────────────────────────────────────────
$deployMode = if ($Config -ieq "Debug") { "--debug" } else { "--release" }
$wdCmd = "`"$WinDeployQtPath`" $deployMode --no-translations --compiler-runtime `"$exePath`""
if ($vcvarsPath) {
  cmd /c "call `"$vcvarsPath`" && $wdCmd" 2>$null | Out-Null
} else {
  Invoke-Expression $wdCmd 2>$null | Out-Null
}
if ($LASTEXITCODE -ne 0) {
  Write-Warning "windeployqt exited $LASTEXITCODE — Qt DLLs may be missing, but continuing."
}

# ── Launch ───────────────────────────────────────────────────────────────────
Start-Process -FilePath $exePath -WorkingDirectory (Split-Path -Parent $exePath) | Out-Null
Write-Host "Application launched: $exePath"
