# Architecture (Current MVP Core)

## Layering

- `src/core`: Qt-independent document/layer/pixel/render/tool logic
- `src/app`: Qt Widgets shell, input handling, panel wiring
- `src/platform/qt`: conversion boundary (`PixelBuffer <-> QImage`)

## App Boundary

- `AppController` is the bridge between UI and core.
- Panels and `CanvasWidget` call `AppController` APIs only.
- Core types remain Qt-free.

## Tool System

- Runtime tool switching is handled by `ToolManager` (`core`).
- UI-facing definitions are data-driven in `app/ui`:
  - `ToolDescriptor`
  - `SubToolDescriptor`
  - `ToolCatalog`
  - `UiState`
- Brush/Eraser behavior is driven by sub-tool presets (`size/opacity/hardness/flow/spacing`).
- Sub-tools carry `targetLayerKind` (`Raster` / `Vector` / `Both`) so tool availability can follow active layer type.
- Sub-tool presets are runtime-editable (`duplicate/rename/delete/reset`) through `ToolCatalog`.

## Layer Model

- `Layer` now carries `LayerKind`:
  - `Raster`: pixel buffer-based painting
  - `Vector`: path list (`VectorPath`) rendered at composite time
- `LineTool` writes vector paths on vector layers and raster pixels on raster layers.
- `Renderer` rasterizes vector paths into a temporary buffer and composites it with standard alpha blending.

## History Scope

- Undo/Redo currently covers:
  - pixel-changing operations (`Brush`, `Eraser`, `Line`, `Fill`, `MoveLayer`)
  - layer visibility toggle
  - selection changes (`RectSelection`, clear, invert)
- History payload is currently snapshot-based (`before/after`) for safety and simplicity.

## UI Shell

- `QDockWidget` workspace:
  - Left docks: Tool / Sub Tool / Tool Property / Color
  - Right docks: Layer / Info
  - Center: Canvas
  - Bottom: status bar
- `Window` menu controls panel visibility and supports reset workspace.

## Extensibility Direction

- Add new sub-tools through `ToolCatalog` without touching deep UI logic.
- Add new property controls by extending descriptor keys and `ToolPropertyPanel` bindings.
- Current structure is prepared for future external definition loading (JSON) without changing core behavior contracts.
