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
  - `Folder`: non-rendering organizational placeholder layer (baseline)
  - `Adjustment`: non-destructive color correction (BrightnessContrast, HueSaturation, Levels, Invert, Threshold, Vibrance)
  - `Text`: editable text, rasterized to PixelBuffer on commit
- Layer flags include `clippedToBelow` and optional mask (`hasMask/maskEnabled` + mask buffer), and renderer applies these during compositing.
- `LineTool` writes vector paths on vector layers and raster pixels on raster layers.
- `Renderer` rasterizes vector paths into a temporary buffer and composites it with standard alpha blending.
- Layer order changes are centralized in `Document::moveLayer` and consumed by both button moves and LayerPanel drag/drop reorder.

### ⚠️ Canvas-Bound Layer Model（現在の制約）

現在 (`2026-06-10` 時点)、ラスターレイヤーはすべてキャンバスサイズに固定・原点(0,0)整列。
`Layer` クラスに offset フィールドはなく、`PixelBuffer` は常にキャンバスサイズで生成される。

- ペースト時にキャンバス外ピクセルが破棄される
- レイヤー移動はピクセルの物理コピー（端が消失、不可逆）
- CSP/Photoshop の「off-canvas ピクセル保持」動作が実現できない

### 次期: Layer Offset Model（ADR-008 ACCEPTED）

**[ADR-008](adr/ADR-008-layer-offset-model.md)** で `offsetX/offsetY` フィールド追加と
独立サイズ `PixelBuffer` への移行が決定済み（未実装）。

移行フェーズ:
- **Phase 0:** `Layer` に `offsetX/offsetY` 追加（デフォルト 0 → 既存動作維持）
- **Phase 1:** `Renderer` 全ピクセルアクセスに `(x - offsetX, y - offsetY)` 変換
- **Phase 2:** `MoveLayerTool` をオフセット変更に置換。ペーストを元サイズ保持に変更
- **Phase 3:** ブラシのバッファ動的拡張（最高リスク、十分なテスト必須）
- **Phase 4:** LPA v2 フォーマット更新、PSD 完全対応

## History Scope

- Undo/Redo currently covers:
  - pixel-changing operations (`Brush`, `Eraser`, `Line`, `Fill`, `MoveLayer`)
  - layer visibility toggle
  - selection changes (`RectSelection`, clear, invert)
- History payload is currently snapshot-based (`before/after`) for safety and simplicity.

## UI Shell

- `QDockWidget` workspace:
  - Left docks: Tool / Sub Tool / Tool Property / Color
  - Right docks: Layer / Navigator+Info
  - Center: Canvas
  - Bottom: status bar
- `Window` menu controls panel visibility and supports reset workspace.
- `Window` menu includes workspace layout save/load/delete and restore-last for dock workflow continuity.
- Navigator preview uses composited output and provides quick zoom actions (`100%` / `Fit`).
- Layer panel displays top-most layers at the top row and maps row/index explicitly, so button move and drag/drop reorder match user expectations.
- Canvas view supports optional grid/overlay toggles through the `View` menu without changing core rendering data.
- File/Edit menus are wired to controller-level operations (selection fill/delete, merge/rasterize, new-from-clipboard, import-as-layer, clipboard image import/export as raster layer, flattened export).
- Color operations (`Swap FG/BG`, `Reset B/W`, `Transparent`) are command actions, so panel buttons and shortcuts share the same path.
- Tool/sub-tool/property panels surface layer-kind compatibility through enabled/disabled state, so raster/vector mismatches are visible before execution.
- Shortcut bindings are action-driven and persisted via `QSettings`, so menu and key operations follow one command path.

## Known Architecture Debt

Architecture Audit (2026-06-14) で確認された既知の技術的負債。
段階移行で解消予定。全面 rewrite 禁止。

### [HIGH] AppController 肥大化

- `AppController.cpp`: 約7,400行 / public メソッド 311個
- レイヤー管理・ブラシ設定・選択・変形・AI操作・Undo/Redo が全混在
- 将来: LayerService / SelectionService / BrushSettingsService への段階分割
- **現状維持中**: 新機能を追加する際は既存パターンに従い、新規サービス分割はスコープを明示してから着手

### [HIGH] ComfyUI 二重経路

- 旧経路: `AppController` が `ComfyUiClient`（WebSocket）を直接保持
- 新経路: `AiService → ComfyProvider → ComfyClient`（HTTP polling）
- 正式経路: 新経路（ADR-010 に基づく）
- 解消手順: `ComfyUiClient` と `MinimalWebSocket` を除去し、旧経路依存箇所を新経路に切り替え

### [HIGH] core/ の Qt 依存

- 違反ファイル: `src/core/ai/GenerativeFillEngine.h/cpp`（`<QObject>` `<QTimer>` `<QThread>` を include）
- 方針: `src/core/` は Qt 非依存を維持する設計原則（`architecture.md` App Boundary 参照）
- 解消手順: Qt 依存部分を `src/app/` 側に移動し、core は純粋なロジックのみ保持

### [HIGH] panels/mainwindow が platform/ を直接 include

- 違反箇所: `UpscaleDialog.cpp:19`, `GenerativeFillDialog.cpp:22`, `MainWindow.cpp:92`
- 全て `platform/qt/QtImageConverter.h` を直接 include
- 方針: Panel / Dialog は `AppController` のみ依存。platform 層への直接アクセス禁止（ADR-010）
- 解消手順: QtImageConverter の呼び出しを bridge 層に移動

### [MEDIUM] 未使用コード候補

- `src/app/panels/AiGenerateDialog.h/cpp`（約464行）: インスタンス化箇所が0件。GenerativeFillDialog に置き換えられた旧ファイルの可能性
- `ComfyUiClient` / `MinimalWebSocket`: 上記 ComfyUI 二重経路解消時に同時削除予定

### 解消優先順位

新機能開発をブロックしない順に解消する。

1. 未使用 AiGenerateDialog 削除（安全、リスク低）
2. ComfyUI 旧経路（ComfyUiClient）除去（新経路に一本化）
3. panels/mainwindow の platform 直接依存を bridge 経由に変更
4. core/ai Qt 依存を app/ 側に移動
5. AppController 分割（最後、最大リスク）

---

## Extensibility Direction

- Add new sub-tools through `ToolCatalog` without touching deep UI logic.
- Add new property controls by extending descriptor keys and `ToolPropertyPanel` bindings.
- Current structure is prepared for future external definition loading (JSON) without changing core behavior contracts.
