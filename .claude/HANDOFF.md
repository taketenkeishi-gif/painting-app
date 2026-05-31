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

## 完了（追加）

### VectorPath float 化（commit 219cb34）
全ツールの VectorPath::points を FPoint に移行。描画座標がピクセルグリッドにスナップしなくなった。

### 筆圧ダイナミクス UI（commit ad85c34）
ToolPropertyPanel に「筆圧ダイナミクス」セクション追加。
pressureSize / pressureOpacity の ON/OFF チェックボックスと最小値スライダー（0–100%）。
詳細表示モードで表示。

### SubToolPanel グリッド表示（commit ccafa36）
リスト → アイコングリッドへ刷新。90×74 タイル、ベジェストロークプレビュー付き。

---

## 次のタスク（優先順）

### 1. EraserTool AA 向上（UX 直結）
`eraseCircle` が整数半径ループで旧 `stampCircle` と同じ問題。
`brushCoverage` を使った float 精度 + 1px AA fringe に置換。

**変更ファイル:**
- `src/core/tools/EraserTool.cpp` — `eraseCircle` を `eraseCircleAA(FPoint center, float radius)` に

### 2. spacing を float radius ベースに（UX 直結）
現状 `spacingPx = spacing * size_int` で小さいブラシでスタンプが粗い。
`spacing * baseRadius * 2.0f` に変更するだけで滑らかになる。

**変更ファイル:**
- `src/core/tools/BrushTool.cpp` — `strokeSegment` 内の `spacingPx` 計算
- `src/core/tools/EraserTool.cpp` — `eraseStroke` 内の `spacingPixels` 計算

### 3. Skia バックエンド移行（中期）
`src/platform/skia/` を新設し、`PixelBuffer` 裏側を `SkSurface` に置換。
vcpkg で Skia を導入: `vcpkg install skia`

---

## 既知の問題

- マウス入力の pressure は常に 1.0f（仕様）。ペンタブは `tabletEvent` 経由で正常取得済み。
- `BrushTool` の `spacingPx` が整数ブラシサイズ基準のため、小さいブラシで spacing が粗い場合がある。
  → spacing を float radius ベースに変更するとより滑らか。
