# build-skia-configure.ps1 — PAINT_USE_SKIA=ON ビルドディレクトリを初期化
# Usage: .\build-skia-configure.ps1
#
# 前提: vcpkg install が完了して skia:x64-windows がインストール済みであること
#       C:\vcpkg\scripts\buildsystems\vcpkg.cmake が存在すること

$root    = $PSScriptRoot
$buildDir = "$root\build-skia"

cmake `
    -S "$root" `
    -B "$buildDir" `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET="x64-windows" `
    -DPAINT_USE_SKIA=ON `
    -DPAINT_DEBUG_SERVER=ON `
    -DPAINT_BUILD_APP=ON `
    -DPAINT_BUILD_QT_PLATFORM=ON `
    -DCMAKE_PREFIX_PATH="C:/Qt/6.7.2/msvc2019_64"

Write-Host ""
Write-Host "Configure complete. To build:"
Write-Host "  cmake --build build-skia --config Release"
Write-Host ""
Write-Host "To run:"
Write-Host "  .\build-skia\src\Release\LayeredPaintApp.exe --debug-server"
