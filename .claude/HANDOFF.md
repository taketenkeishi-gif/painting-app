# 引き継ぎタスク

全体ビジョン・技術選定は `docs/SPEC.md` を参照。

---

## 完了済み

### 描画エンジン AA 修正
- `src/core/render/RenderUtils.h` 新設 — `brushCoverage()` 共有関数
- `Renderer.cpp` ベクター描画: `stampCircle`（整数）→ `stampCircleAA`（float + 1px AA fringe）
- `Renderer.cpp` ベクターセグメント: `drawSegment`（階段状）→ `drawSegmentAA`（float精度スタンプ列）
- `BrushTool.cpp`: `gaussianFalloff` 削除 → `brushCoverage` に統一、`antiAlias` フラグが実機能に接続

### UI テーマ全面刷新
- `MainWindow::applyUiChrome()`: デザイントークンベースのモダンダークテーマ
  - Base: `#13151c` / Surface: `#1a1d27` / Accent: `#4e8ef7`
  - スライダー、コンボボックス、チェックボックス、スクロールバー全刷新
- `ToolPanel.cpp`: ツールボタンを透明ベース＋ホバー・選択状態を新テーマに統一
- `LayerPanel.cpp`: レイヤーリスト、アイコンボタン、グループボックスを新テーマに統一

### その他
- ブラシサイズ上限: 128 → 2048

---

## 次のタスク（優先順）

### 1. VectorPath の座標を float 化（UX 直結）
現状 `VectorPath::points` は `std::vector<Point>`（整数）のため、
ベクターストロークの座標が入力時点でスナップされジャギが残る。

**変更ファイル:**
- `src/core/layer/Layer.h` — `VectorPath::points: std::vector<FPoint>`
- `src/core/layer/Layer.cpp` — `moveVectorPathsBy` を FPoint 対応に
- `src/core/tools/BrushTool.h` — `m_vectorPoints: std::vector<FPoint>`
- `src/core/tools/BrushTool.cpp` — 各イベントで `event.fpoint` を push
- `src/core/tools/EraserTool.cpp` — `samplePathPoint` / `slicePath` を FPoint 対応に（要注意）
- `src/core/render/Renderer.cpp` — `rasterizeVectorLayer` の Point→float キャスト削除

### 2. 筆圧カーブダイアログ（UX 直結）
`BrushDynamics` に `pressureSizeMin` / `pressureOpacityMin` はあるが UI がない。
ベジェ曲線エディタ（最低限はスライダー）を `ToolPropertyPanel` に追加。

**変更ファイル:**
- `src/app/panels/ToolPropertyPanel.cpp` — pressureSizeMin/pressureOpacityMin スライダー追加
- `src/app/bridge/AppController.h/.cpp` — setPressureSizeMin 等のブリッジ追加

### 3. SubToolPanel のビジュアル刷新
現状テキストリストのみ。サムネイル付きグリッド表示にするとプロらしい見た目になる。

### 4. Skia バックエンド移行（中期）
`src/platform/skia/` を新設し、`PixelBuffer` 裏側を `SkSurface` に置換。
vcpkg で Skia を導入: `vcpkg install skia`

---

## 既知の問題

- マウス入力の pressure は常に 1.0f（仕様）。ペンタブは `tabletEvent` 経由で正常取得済み。
- `BrushTool` の `spacingPx` が整数ブラシサイズ基準のため、小さいブラシで spacing が粗い場合がある。
  → spacing を float radius ベースに変更するとより滑らか。
