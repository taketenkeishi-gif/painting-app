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
- Layer add/delete/duplicate/select/rename/visibility toggle/reorder
- Folder layer creation + clipping toggle + mask toggle/remove (minimum implementation)
- Layer reorder by drag & drop with active layer tracking (plus Up/Down shortcuts with natural direction)
- Raster layer + Vector layer coexistence (`LayerKind`)
- Tool system with switchable tools:
  - Brush / Eraser / Eyedropper / Fill
  - Line / RectSelection / MoveLayer
  - Hand / Zoom
- Layer-kind constrained tool usage (raster-only / vector-only / both)
- Stroke-level Undo (`Ctrl+Z`) / Redo (`Ctrl+Y`) for drawing and selection updates
- Sub-tool presets for Brush/Eraser (Normal/Hard/Soft/Airbrush etc.)
- Sub-tool management (`Duplicate`, `Rename`, `Delete`, `Reset`)
- Tool property panel with high-density sections and immediate apply (`size/stroke width`, `opacity`, `hardness`, `flow`, `spacing`, stabilization, shape/blend/erase controls, angle/roundness/taper, vector snap/simplify)
- Canvas overlay for line preview, selection preview/border, move preview, brush cursor
- Composited canvas display
- Dock-based workspace with Window menu toggles + reset workspace
- File/Edit/Tool/Select/Layer/View/Window/Help menu with practical actions (open/save/export, new-from-clipboard, import-as-layer, clipboard image copy/paste, merge/rasterize, overlay/grid toggles, shortcut summary, docs link)

## Current UI Shell

- Left docks: tool panel / sub-tool panel / tool-property panel / color panel
- Right docks: layer panel / navigator+info panel
- Center: canvas viewport
- Bottom: status bar (tool, sub-tool, guide, color, size, zoom, selection, active layer)

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
System-level implementation status is tracked in `docs/system_status.md`.

## Key Shortcuts

- `B` Brush
- `E` Eraser
- `I` Eyedropper
- `G` Fill
- `U` Line
- `R` Rect Selection
- `M` Move Layer
- `H` Hand
- `Z` Zoom
- `[` / `]` Brush size down/up
- `X` / `D` / `C` Swap FG/BG / reset black-white / transparent color
- `Ctrl+Z` Undo
- `Ctrl+Y` or `Ctrl+Shift+Z` Redo
- `Ctrl+Shift+V` / `Ctrl+Alt+O` New from clipboard / Import image as layer
- `Ctrl+O` / `Ctrl+S` / `Ctrl+Shift+S` Open / Save / Save As
- `Ctrl+Alt+Up` / `Ctrl+Alt+Down` Move layer up/down
- `Ctrl+Shift+G` New folder layer
- `Ctrl+Alt+C` / `Ctrl+Alt+M` Toggle clipping / mask on active layer
- `Ctrl++` / `Ctrl+-` / `Ctrl+0` Zoom in/out/reset
- `Ctrl+Alt+Shift+S` Save workspace layout
- `Ctrl+Alt+W` Restore last workspace layout
- Navigator panel provides mini-canvas preview and quick `100%` / `Fit` buttons.
