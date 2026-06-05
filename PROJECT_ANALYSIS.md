# PROJECT ANALYSIS — LayeredPaintApp

> Manager 初回分析。耐久試験フェーズ（ai-night-test ブランチ）用。

---

## 1. アーキテクチャ概要

### レイヤー構成

```
src/
  core/        Qt非依存の描画コア（Document/Layer/Renderer/Tool/Selection）
  app/
    bridge/    AppController — UI ↔ core の唯一の橋渡し
    canvasview/ CanvasWidget — 描画・オーバーレイ表示
    mainwindow/ MainWindow — ウィンドウ・メニュー・Dock管理
    panels/    ToolPanel / SubToolPanel / LayerPanel / ColorWheelWidget / ToolPropertyPanel
    ui/        ToolDescriptor / UiState / IconLoader / Theme
  platform/
    qt/        PixelBuffer ↔ QImage 変換境界
    skia/      Skia バックエンド（PAINT_USE_SKIA=OFF デフォルト）
```

### 主要クラスの役割

| クラス | 役割 |
|---|---|
| `core::Document` | キャンバスサイズ・DPI・Layerリスト・SelectionMask 保持 |
| `core::Layer` | Raster/Vector/Folder/Adjustment 種別・PixelBuffer・VectorPath・マスク・ロック状態 |
| `core::Renderer` | Document を PixelBuffer に合成（クリッピング・マスク・ブレンドモード適用） |
| `core::BrushTool` | ストローク記録、ラスター/ベクター両対応 |
| `core::ToolManager` | ToolKind 切り替え、アクティブツールの dispatch |
| `app::bridge::AppController` | UI からの全操作受付・履歴管理・rerender トリガー |
| `app::canvasview::CanvasWidget` | paintEvent → QImage 描画・入力イベント → AppController |
| `app::mainwindow::MainWindow` | QDockWidget 構成・メニュー・パネル配線 |

---

## 2. 描画パイプライン

```
ユーザー入力 (mouse/tablet)
  ↓ CanvasWidget::mousePressEvent / tabletEvent
  ↓ AppController::beginStrokeF / continueStrokeF / endStroke
  ↓ ToolManager → BrushTool::strokeTo (core)
  ↓   ├─ Raster: stampCircleAA / drawSegmentAA → PixelBuffer に直書き
  ↓   └─ Vector: VectorPath に点追記 + ToolOverlayState にライブプレビュー
  ↓ AppController::rerender / rerenderDirty
  ↓ Renderer::compositeInto (Document → m_composited PixelBuffer)
  ↓ emit canvasChanged()
  ↓ CanvasWidget::refreshFromController → QImage → QPainter::drawImage
  ↓ 画面表示
```

**現在の実装の限界:**
- `stampCircle` / `drawSegment` は整数演算 CPU ループ（ジャギあり）
- `BrushSettings.antiAlias` フラグは定義のみ、stampCircleAA 呼び分けは未実装
- 筆圧 pressure は mouse 常時 1.0f（tabletEvent 経由では正常取得）

---

## 3. UI 構成

### Dock 構成 (QDockWidget)

```
MainWindow
  ├─ 左側
  │    ├─ m_toolDock         (ToolPanel)
  │    ├─ m_toolSliderDock   (ToolPanel quick slider)
  │    ├─ m_subToolDock      (SubToolPanel)
  │    ├─ m_toolPropertyDock (ToolPropertyPanel)
  │    ├─ m_colorDock        (ColorWheelWidget)
  │    ├─ m_colorSliderDock  (HSV sliders + alpha)
  │    └─ m_colorHistoryDock (色履歴グリッド)
  ├─ 右側
  │    ├─ m_layerDock        (LayerPanel)
  │    ├─ m_infoDock         (Navigator preview + info)
  │    └─ m_aiDock           (AiPanel)
  └─ 中央: CanvasWidget
```

### メニュー構成
- **File**: New/Open/Save/SaveAs/ExportPNG/ExportFlattened/Recent/Exit
- **Edit**: Undo/Redo/Cut/Copy/Paste/Fill/Delete/SelectAll/Deselect/Invert
- **Layer**: Add(Raster/Vector/Folder)/Duplicate/Delete/MergeDown/Rasterize/Clip/Mask/Lock
- **View**: ZoomIn/Out/Reset/FitToScreen/Grid/Overlay/Mirror/ResetRotation
- **Window**: Workspace save/load/reset + Dock toggle
- **Image**: BrightnessContrast/HueSatLight + AdjustmentLayer 追加
- **AI**: GenerativeFill/ConnectComfyUI

---

## 4. レイヤー構造

### LayerKind 種別

| Kind | 動作 |
|---|---|
| `Raster` | PixelBuffer に直接ペイント。合成時はそのまま alpha blend |
| `Vector` | VectorPath リスト保持。Renderer が合成時に一時 PixelBuffer へラスタライズ |
| `Folder` | 子レイヤーなし（フラット配列）。現状は組織化用プレースホルダのみ |
| `Adjustment` | AdjustmentParams 保持。Renderer が下位レイヤーへ非破壊フィルタ適用 |

### Layer フラグ

| フラグ | 実装状況 |
|---|---|
| `visible` | DONE |
| `opacity` | DONE |
| `blendMode` | DONE |
| `clippedToBelow` | DONE（UI toggle あり）|
| `hasMask / maskEnabled` | DONE（create/toggle/remove）|
| `locked / alphaLocked / positionLocked` | DONE（toggle UI あり）|
| フォルダ入れ子 (childrenList) | 未実装 — Layer に子リストなし |
| クリッピンググループ合成 | Renderer 未実装 |

---

## 5. 未完成部分（system_status.md より整理）

### WIP（着手中・未完成）

| 項目 | 内容 |
|---|---|
| dark theme state colors | テーマの状態色ヒエラルキーが未整理 |
| panel spacing consistency | ボタン/リスト間隔のばらつきあり |

### PARTIAL（骨格はあるが機能が不完全）

| 項目 | 不足内容 |
|---|---|
| angle/roundness/taper UI | AppController setters あり・ToolPropertyPanel スライダー未接続 |
| vector erase mode split | ラスター消去と分岐実装が不完全 |
| selection-limited fill | SelectionMask が active でも無視して全域塗りつぶし |
| layer move tool | 移動処理の基礎はあるが不完全 |
| snap-angle presets | スナップ角度のプリセット UI が不完全 |
| navigator quick zoom | ナビゲーターに 100%/Fit ボタンが未接続 |
| color system commands | FG/BG swap/reset の panel ボタン未接続 |
| mouse-first smoothing | tablet以外の平滑化フックが不完全 |

### TODO（未実装）

- 投げ縄・多角形選択 / クイックマスク
- 筆圧カーブ設定ダイアログ
- フォルダ入れ子・クリッピンググループ合成
- AntiAlias フラグ実機能（stampCircleAA 呼び分け）
- ペンタブ Wintab/WinInk ネイティブ対応

---

## 6. SPEC との差分

### Phase 0（現在地）

| SPEC 項目 | 状態 | 備考 |
|---|---|---|
| 0-1. Skia バックエンド移行 | PARTIAL | src/platform/skia/ 整備済み、PAINT_USE_SKIA=OFF デフォルト。実描画は未切替 |
| 0-2. libmypaint 統合 | NOT STARTED | gaussianFalloff 手書きループのまま |
| 0-3. ベクター AA 描画 (Skia Path) | NOT STARTED | stampCircleAA は自前 AA のみ |
| 0-4. AntiAlias 制御 | PARTIAL | BrushSettings.antiAlias 定義あり・実コード未反映 |

### Phase 1（標準機能）

| SPEC 項目 | 状態 |
|---|---|
| レイヤー: フォルダ入れ子 | NOT STARTED |
| レイヤー: クリッピンググループ | NOT STARTED |
| レイヤー: ロックモード | DONE |
| 選択: 投げ縄・多角形・自動 | PARTIAL（矩形のみ実装）|
| 選択: 拡張・縮小・ぼかし | NOT STARTED |
| 変形ハンドル | NOT STARTED |
| グラデーションツール | 実装済み (GradientTool.h 存在) |
| トーンカーブ・レベル補正 | AdjustmentLayer として定義あり・Renderer 適用は要確認 |
| ペンタブ筆圧/傾き完全対応 | PARTIAL (tabletEvent あり・Wintab 未対応) |

---

*生成: 2026-06-05 Manager 初回セッション*
