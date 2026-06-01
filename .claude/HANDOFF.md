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

## 完了（追加2）

### 全画面モード回避（Aero Snap 防止）
`TitleBarDragArea::mouseMoveEvent` に Y 座標クランプを追加。
ドラッグ先 Y が画面 availableGeometry.top()+4 を下回らないよう制限。

### ツールパネルボタン整列修正
`ToolPanel::relayoutButtons()`:
- `setFixedWidth` 削除 → ホスト幅を Expanding で dock 全体に広がる
- `buttonGridHost` の alignLeft 削除 → Qt::AlignHCenter で中央揃え
- グリッドレイアウトのアライメントも HCenter に変更

### spacing float 化
- `BrushTool.cpp`: `spacing * size_int` → `spacing * baseRadius * 2.0f`
- `EraserTool.cpp`: 同様に `fRadius * 2.0F` 基準に

---

## 完了（追加3）

### EraserTool AA 向上（commit）
- `eraseCircleAA(FPoint center, float radius)` 新設 — `brushCoverage()` で 1px AA fringe
- `eraseStroke` の stamp 位置を float 化（`FPoint fp` でサブピクセル精度）
- `RenderUtils.h` を include して brushCoverage を共有

---

## 完了（追加4）

### 範囲選択ツール強化（commit b714fb9）
- `ToolOverlayState` にポリゴンフィールド追加（hasPolygon / polygonClosed / polygonPoints）
- 投げ縄ドラッグ中: 全パスをリアルタイムポリゴンで表示（開放パス＋閉じるヒント破線）
- 投げ縄コミット後: 輪郭ポリゴンを保持表示（サブツール切替でクリア）
- CanvasWidget にマーチングアンツアニメーション（80ms Tick、矩形選択・投げ縄両対応）
- サブツール名日本語化: 矩形選択 / 投げ縄 / 自動選択

---

## 次のタスク（優先順）

### 1. Skia バックエンド移行（中期）
`src/platform/skia/` を新設し、`PixelBuffer` 裏側を `SkSurface` に置換。
vcpkg で Skia を導入: `vcpkg install skia`

---

## 既知の問題

- マウス入力の pressure は常に 1.0f（仕様）。ペンタブは `tabletEvent` 経由で正常取得済み。
