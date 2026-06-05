# OpenCV インストール完了後にこのスクリプトを実行してください
# Usage: powershell -File scripts\rebuild_with_opencv.ps1

param(
    [string]$QtPath = "C:\Qt\6.7.2\msvc2019_64",
    [string]$VcpkgRoot = "C:\vcpkg"
)

Write-Host "=== OpenCV + Skia 有効ビルド ===" -ForegroundColor Cyan

# vcpkg インストール確認
$opencvInclude = "$VcpkgRoot\installed\x64-windows\include\opencv2"
if (-not (Test-Path $opencvInclude)) {
    Write-Host "OpenCV not found at $opencvInclude" -ForegroundColor Red
    Write-Host "Run: cd '$PSScriptRoot\..' ; C:\vcpkg\vcpkg.exe install --triplet x64-windows" -ForegroundColor Yellow
    exit 1
}
Write-Host "OpenCV found: $opencvInclude" -ForegroundColor Green

# CMake 再構成（OpenCV 有効）
$buildDir = "$PSScriptRoot\..\build_opencv"
$toolchain = "$VcpkgRoot\scripts\buildsystems\vcpkg.cmake"

Write-Host "Configuring with OpenCV..." -ForegroundColor Cyan
cmake -S "$PSScriptRoot\.." -B $buildDir `
    -DCMAKE_TOOLCHAIN_FILE="$toolchain" `
    -DCMAKE_PREFIX_PATH="$QtPath" `
    -DPAINT_USE_OPENCV=ON `
    -DPAINT_BUILD_TESTS=OFF `
    -DCMAKE_BUILD_TYPE=Release

if ($LASTEXITCODE -ne 0) { Write-Host "CMake configure failed" -ForegroundColor Red; exit 1 }

Write-Host "Building..." -ForegroundColor Cyan
cmake --build $buildDir --config Release

if ($LASTEXITCODE -eq 0) {
    Write-Host "=== Build SUCCESS ===" -ForegroundColor Green
    Write-Host "Executable: $buildDir\src\Release\LayeredPaintApp.exe" -ForegroundColor Green

    # DLL コピー（vcpkg の DLL を exe と同じ場所に）
    $dllSrc = "$VcpkgRoot\installed\x64-windows\bin"
    $dllDst = "$buildDir\src\Release"
    Copy-Item "$dllSrc\opencv_*.dll" $dllDst -ErrorAction SilentlyContinue
    Write-Host "OpenCV DLLs copied to $dllDst" -ForegroundColor Green
} else {
    Write-Host "Build FAILED" -ForegroundColor Red
    exit 1
}
