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

1. Window opens with canvas and layer panel.
2. New Canvas changes width and height.
3. Add Layer creates a new selectable layer.
4. Selecting another layer changes the active highlight and draw target.
5. Double-clicking a layer name renames it.
6. Visibility checkbox toggles composited display.
7. Delete removes selected layer but never removes the last remaining layer.
8. Brush color button changes drawing color.
9. Brush size spinner affects stroke thickness.
10. Draw one stroke, press `Ctrl+Z`, stroke disappears.
11. Press `Ctrl+Y`, undone stroke returns.

## Notes

- `PAINT_BUILD_TESTS=ON` now hard-fails for CMake `< 4.3.1` to avoid a known crash path in this environment.
- `subst` is no longer required when using CMake `4.3.1` in this project setup.
