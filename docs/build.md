# Build Operations (Pinned Workflow)

This document fixes the day-to-day build procedure after validating CMake variants on April 19, 2026.

## Recommended Toolchain

- Recommended CMake: `4.3.1`
- Previous stable checked: `4.2.4`
- Qt prefix: `C:/CraftRoot_KF6`

## Validation Summary

- `CMake 4.3.1`:
  - `PAINT_BUILD_TESTS=OFF`: configure/build OK
  - `PAINT_BUILD_TESTS=ON`: configure/build OK
  - Japanese path direct (`C:\...`): configure OK
- `CMake 4.2.4`:
  - `PAINT_BUILD_TESTS=OFF`: configure/build OK
  - `PAINT_BUILD_TESTS=ON`: configure crash reproduced (`-1073740791`) when `CTest` is included
- Existing RC in PATH (`4.1.0-rc1`):
  - unstable in this environment (configure crash reproduced)

## Standard Daily Commands

```powershell
$cmake = "C:/temp/cmake_versions/cmake-4.3.1/bin/cmake.exe"
$qtPrefix = "C:/CraftRoot_KF6"

& $cmake -S . -B build -DCMAKE_PREFIX_PATH=$qtPrefix -DPAINT_BUILD_TESTS=OFF
& $cmake --build build --config Debug
```

Equivalent helper script:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

## Test Build Commands

```powershell
$cmake = "C:/temp/cmake_versions/cmake-4.3.1/bin/cmake.exe"
$qtPrefix = "C:/CraftRoot_KF6"

& $cmake -S . -B build-tests -DCMAKE_PREFIX_PATH=$qtPrefix -DPAINT_BUILD_TESTS=ON
& $cmake --build build-tests --config Debug
ctest --test-dir build-tests --output-on-failure -C Debug
```

Equivalent helper script:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-tests.ps1
```

`PAINT_BUILD_TESTS=ON` includes:

- `core_tests` (core model + renderer + brush)
- `app_smoke_tests` (AppController flow smoke without UI automation)

## Runtime Smoke Check (MVP)

After build, run `LayeredPaintApp.exe` and verify:

1. Window opens with `left/right dock panels + center canvas + bottom status`.
2. Window menu can hide/show docks and `Reset Workspace` restores default layout.
3. New Canvas changes width and height.
4. Layer panel can add Raster/Vector layers, duplicate, delete, and reorder (buttons + drag/drop).
5. Selecting another layer changes active highlight and draw target; row prefix shows `[R]` or `[V]`.
6. Visibility checkbox toggles composited display.
7. Brush on Vector layer is blocked (status guide shows compatibility hint).
8. Line tool on Vector layer creates vector path and appears in composited canvas.
9. Sub-tool panel supports duplicate/rename/delete/reset.
10. Tool property panel applies changes immediately (including angle/roundness/taper and line snap/simplify where supported).
11. Draw one stroke, press `Ctrl+Z`, stroke disappears.
12. Press `Ctrl+Y`, undone stroke returns.
13. File/Edit menus work for implemented operations (`Open`, `Save`, `Save As`, `Copy/Paste image`, `Fill`, `Delete Pixels`, `Merge Down`, `Rasterize Layer`).

Helper launch script:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run.ps1 -Config Debug
```

Note: In this environment, `windeployqt` may fail under non-ASCII workspace paths. The script falls back to PATH-based Qt runtime and still launches the app.

## Notes

- `PAINT_BUILD_TESTS=ON` now hard-fails for CMake `< 4.3.1` to avoid a known crash path in this environment.
- `subst` is no longer required when using CMake `4.3.1` in this project setup.
