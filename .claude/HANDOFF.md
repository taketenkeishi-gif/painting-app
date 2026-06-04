# 引き継ぎタスク

全体ビジョン・技術選定は `docs/SPEC.md` を参照。

---

## 完了済み

### 描画エンジン AA 修正
- `src/core/render/RenderUtils.h` 新設 — `brushCoverage()` 共有関数
- `Renderer.cpp` ベクター描画: `stampCircle`（整数）→ `stampCircleAA`（float + 1px AA fringe）
- `BrushTool.cpp`: `gaussianFalloff` 削除 → `brushCoverage` に統一、`antiAlias` フラグが実機能に接続

### UI テーマ全面刷新
- `MainWindow::applyUiChrome()`: デザイントークンベースのモダンダークテーマ
  - Base: `#13151c` / Surface: `#1a1d27` / Accent: `#4e8ef7`
- `ToolPanel.cpp` / `LayerPanel.cpp` も新テーマに統一

### VectorPath float 化（commit 219cb34）
全ツールの VectorPath::points を FPoint に移行。

### 筆圧ダイナミクス UI（commit ad85c34）
ToolPropertyPanel に「筆圧ダイナミクス」セクション追加。

### SubToolPanel グリッド表示（commit ccafa36）
リスト → アイコングリッドへ刷新。90×74 タイル、ベジェストロークプレビュー付き。

### 全画面モード回避 / ツールパネルボタン整列修正 / spacing float 化

### EraserTool AA 向上
`eraseCircleAA(FPoint center, float radius)` 新設。

### 範囲選択ツール強化（commit b714fb9）
- 投げ縄ドラッグ中リアルタイムポリゴン表示
- マーチングアンツアニメーション（80ms Tick）
- サブツール名日本語化

---

## 完了（Phase 0-2: ブラシ品質向上）commit 98f89c0

### 速度感応ダイナミクス
- `BrushTool`: `std::chrono::steady_clock` で px/ms 速度計測、指数平滑化
- velocity → サイズ・透明度を動的変化
- UI: `m_velocitySection` (ON/OFF + 最小値スライダー)

### テクスチャグレイン
- ハッシュベースの決定論的セルノイズ (`grainNoise`)
- 外部ファイル不要。strength / scale で制御
- UI: `m_textureSection`

### ウェットミックス / スメア
- WetMix: `lerpColor(brushColor, canvasColor, rate)` per stamp
- Smear: スタンプ中心のキャンバス色をサンプリングしてブレンド
- UI: `m_wetSection`

### UiState / AppController / ToolPropertyPanel への配線
- `UiState` に 10 フィールド追加
- `AppController::applyUiStateToTools()` で全 setter 呼び出し
- `ToolStateViewModel` に対応フィールド追加

---

## 完了（ComfyUI + AiSelectTool）commit 8a94aca

### ComfyUiClient（HTTP ポーリング方式）
- Qt6WebSockets 未インストール → WebSocket 廃止、HTTP ポーリングに変更
- `connectToServer()`: GET /system_stats
- `queuePrompt()`: POST /prompt → prompt_id 取得
- `onPollTimer()`: 1 秒間隔で GET `/history/{id}` をポーリング（300 回タイムアウト）
- `buildInpaintWorkflow()` / `buildSamWorkflow()`: JSON ワークフロービルダー

### AiSelectTool
- Sobel エッジ検出 + 分散適応型 BFS フラッドフィル
- `onPointerPress` で即時スタブセグメンテーション実行
- `m_inferenceCallback` が設定されていれば ComfyUI SAM2 を非同期起動
- `AppController::applyAiSelectResult()` で精製マスクを後から適用

---

## 完了（Phase 0-1: Skia バックエンド整備）commit ff2b231

### 新設ファイル
- `src/platform/skia/SkiaPixelBuffer.h/.cpp`: `SkBitmap` を `core::PixelBuffer` 互換 API でラップ
- `src/platform/skia/SkiaRenderer.h/.cpp`: `SkCanvas` + `SkBlendMode` でレイヤー合成
- `src/platform/skia/SkiaIntegration.h`: `platform::skia::available()` 検出ヘルパ

### CMake
- `option(PAINT_USE_SKIA "..." OFF)` 追加
- `PAINT_USE_SKIA=ON` 時: `paint_skia_platform` static lib ビルド・リンク
- vcpkg toolchain 経由で `unofficial::skia::skia` にリンク

### ビルド確認済み
- `PAINT_USE_SKIA=OFF`（デフォルト）: 既存ビルドに影響なし ✅
- `PAINT_USE_SKIA=ON`: `paint_skia_platform.lib` + `LayeredPaintApp.exe` ✅

### Skia ON ビルドコマンド
```powershell
cmake .. -DPAINT_USE_SKIA=ON `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  "-DCMAKE_PREFIX_PATH=C:/Users/KEISHI/6.7.2/msvc2019_64;C:/vcpkg/installed/x64-windows"
```

---

## 次のタスク（優先順）

### Phase 0-3: Skia レンダリング移植（中期）
- `SkiaRenderer` は現在 CPU PixelBuffer → SkBitmap へのブリットで実装（コピーあり）
- 将来: レイヤーごとに `SkBitmap` を保持してコピーをゼロにする
- `CanvasWidget::paintEvent` で `platform::skia::available()` により切り替え

### ComfyUI 画像アップロード未実装
- `POST /upload/image` エンドポイントが未実装
- inpaint / SAM ワークフローで実際に画像を送るのに必要

### フォルダレイヤー入れ子構造
- `LayerKind::Folder` は存在するが `Layer` にチャイルドリストがない
- UI ボタンはあるがネスト動作しない

### クリッピンググループ合成
- `toggleLayerClip` UI はあるが `Renderer` が未実装

---

## 既知の問題

- マウス入力の pressure は常に 1.0f（仕様）。ペンタブは `tabletEvent` 経由で正常取得済み。
- Qt バージョン: `C:\Users\KEISHI\6.7.2\msvc2019_64`（C:\Qt には存在しない）
