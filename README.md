# Layered Paint App (MVP Foundation)

Layered paint application foundation with `Qt 6 + C++17 + CMake`.

## Stack

- C++17
- Qt 6 (`Core`, `Gui`, `Widgets`)
- CMake

## Repository Layout

- `src/core`: Qt-independent drawing core
- `src/app`: Qt UI and event handling
- `src/platform/qt`: conversion layer (`PixelBuffer` <-> `QImage`)
- `tests/core`: core-only tests

## Current MVP Scope

- New canvas creation
- Layer add/delete/select/rename/visibility toggle
- Brush drawing by mouse drag
- Stroke-level Undo (`Ctrl+Z`) / Redo (`Ctrl+Y`) for drawing
- Brush color and size controls
- Composited canvas display

## Build Quick Start (Windows / PowerShell)

For now, use `CMake 4.3.1` as the default working version.

```powershell
$cmake = "C:/temp/cmake_versions/cmake-4.3.1/bin/cmake.exe"
$qtPrefix = "C:/CraftRoot_KF6"

& $cmake -S . -B build -DCMAKE_PREFIX_PATH=$qtPrefix -DPAINT_BUILD_TESTS=OFF
& $cmake --build build --config Debug
```

Or use the helper script:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

Enable core tests:

```powershell
$cmake = "C:/temp/cmake_versions/cmake-4.3.1/bin/cmake.exe"
$qtPrefix = "C:/CraftRoot_KF6"

& $cmake -S . -B build-tests -DCMAKE_PREFIX_PATH=$qtPrefix -DPAINT_BUILD_TESTS=ON
& $cmake --build build-tests --config Debug
ctest --test-dir build-tests --output-on-failure -C Debug
```

Or use the helper script:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-tests.ps1
```

Detailed operational notes are in `docs/build.md`.
