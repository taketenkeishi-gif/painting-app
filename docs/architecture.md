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

## History Scope

- Undo/Redo currently covers:
  - pixel-changing operations (`Brush`, `Eraser`, `Line`, `Fill`, `MoveLayer`)
  - layer visibility toggle
  - selection changes (`RectSelection`, clear, invert)
- History payload is currently snapshot-based (`before/after`) for safety and simplicity.

## UI Shell

- Left: tool selection panel
- Top: current tool/sub-tool info bar
- Right: layer + sub-tool + tool-property panels
- Center: canvas
- Bottom: status bar (tool, guide, color, size, zoom, active layer)

## Extensibility Direction

- Add new sub-tools through `ToolCatalog` without touching deep UI logic.
- Add new property controls by extending descriptor keys and `ToolPropertyPanel` bindings.
- Current structure is prepared for future external definition loading (JSON) without changing core behavior contracts.
