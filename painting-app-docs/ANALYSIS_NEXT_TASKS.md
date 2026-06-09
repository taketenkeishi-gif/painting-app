# Painting-app 次タスク分析レポート

**分析対象:** PROJECT_STATUS.md / SPEC.md / SPEC2.md  
**分析日:** 2026-06-08  
**スコープ:** AI 選択ツール除外、CSP/Krita 基本機能ギャップ分析

---

## 📊 1. 現在の完了済み領域

### Phase 0 — 描画エンジン根本再構築（進捗度 70%）

**✅ 完了:**
- **Skia 統合基盤** — `SkiaPixelBuffer`, `SkiaRenderer` 実装（デフォルト OFF）
- **ブラシ品質向上** — velocity dynamics, texture grain, wet-mix, AA 描画（stampCircleAA, drawSegmentAA）
- **ベクターレイヤー記録** — VectorPath float 化、ライブプレビュー、ラスタライズ合成

**⚠️ 部分完了:**
- **ベクターレイヤー AA** — ラスタライズ品質は OK、編集ツール未実装
- **AA 制御** — `BrushSettings.antiAlias` フラグ定義済み、UI 未公開

### Phase 1 — 標準機能の実装（進捗度 30%）

#### ✅ 実装済み

**レイヤーシステム（基本）:**
- Raster / Vector / Folder 型定義
- Visibility toggle / duplicate / merge down / rasterize
- Opacity スライダー + undo/redo
- Drag/drop reorder

**選択範囲:**
- 矩形選択 + clear / invert / all / deselect
- SelectionMask システム（undo/redo 対応）
- マーチングアンツ表示

**ツール:**
- ブラシ（Raster + Vector）
- 消しゴム、直線、塗りつぶし（SelectionMask 対応）
- 移動ツール（Layer Move）

**その他:**
- HSV カラーシステム・HiDPI スウォッチ
- Dark theme（color token ベース）
- ドック・ワークスペース管理（save/load）
- Shortcut 実行時設定・保存

#### ❌ 不足

**レイヤーシステム:**
- フォルダ階層（ネスト）
- クリッピンググループ合成
- 複数レイヤー選択・一括操作
- ロックモード（位置固定 / 透明保護）
- レイヤーカラーラベル

**選択範囲:**
- なげなわ / 多角形選択
- 自動選択（隣接 / 全体）
- クイックマスク
- Feather / Grow / Shrink

**ベクター編集:**
- Point select / move / delete
- Path split / connect / simplify
- テーパー形状適用

**変形:**
- Bounding box handles（rotate / scale / pivot）
- 自由変形 / メッシュ変形
- 変形プレビュー

**ツール:**
- グラデーションツール
- テキストツール
- 調整レイヤー（トーンカーブ・色相彩度）

**ペンタブ:**
- Wintab / Windows Ink API
- 筆圧・傾き・回転取得
- 筆圧カーブ設定ダイアログ

**ファイル I/O:**
- PSD/PSB インポート
- ネイティブフォーマット（`.paintml` 等）

---

## 🎯 2. CSP/Krita との機能ギャップ分析

### 優先度 S（core レベル、必須）

| 機能 | CSP/Krita の実装レベル | Painting-app の状態 | ギャップ | 理由 |
|---|---|---|---|---|
| **ベクター編集** | フル（point edit / split / simplify） | Record only（編集 0%） | 🔴 Critical | 描画品質の次に重要な機能 |
| **フォルダ階層** | フル（階層管理・色ラベル） | 型定義のみ（0%） | 🔴 Critical | Layer 組織のため |
| **複数レイヤー選択** | フル（Shift/Ctrl+Click） | 未実装（0%） | 🔴 Critical | 高速編集のための基本 |
| **Wintab/Windows Ink** | フル | Mouse only（0%） | 🔴 Critical | 描画入力品質を決める |

### 優先度 A（Phase 1 後半、重要）

| 機能 | 状態 | ギャップ | 工数予測 |
|---|---|---|---|
| **クリッピンググループ合成** | UI なし、Renderer 対応 | 合成ロジックのみ | 8h |
| **ロックモード** | UI なし | フラグ + UI + Renderer | 6h |
| **投げ縄・多角形選択** | なし | 完全新規 | 12h |
| **自動選択** | なし | 完全新規（flood-fill algorithm） | 10h |
| **変形ハンドル** | なし | UI + Renderer 対応 | 15h |

### 優先度 B（Phase 1+、便利機能）

| 機能 | 状態 | 工数予測 |
|---|---|---|
| **グラデーション** | なし | 15h |
| **テキストツール** | なし | 20h |
| **調整レイヤー** | なし | 25h |
| **Feather / Grow / Shrink** | なし | 10h |

---

## ⚠️ 3. アーキテクチャリスク分析

### 🟢 良好（設計的に健全）

✅ **Platform 層の厳格な区分**
- `src/core/` が Qt フリー
- Skia / ONNX ラッパーが `src/platform/` に隔離
- 「core は Qt フリー」が守られている

✅ **ツール体系の拡張性**
- `ToolDescriptor` で code-driven 定義
- SubTool プリセット実行時編集可能
- 新規サブツール追加が容易

✅ **History System**
- Snapshot-based (`before / after`)
- シンプルで信頼性高い（デバッグ容易）

### 🟡 改善候補（運用で対応可能）

⚠️ **Skia は optional デフォルト OFF**
- 現在：CPU レンダラーがメイン
- 計画：Phase 1 中に Skia を default にしたい
- リスク：CPU レンダラーの技術債が蓄積（AA 描画の手動実装など）

**対応策:**
- Phase 1-A（短期）：ベクター編集実装時に Skia 移植完了
- CPU レンダラーのメンテは最小限に

⚠️ **History が snapshot-based → メモリ効率課題**
- 100+ 操作で数百 MB メモリ消費
- Phase 1 後期に差分記録へ改善予定
- 現在：小プロジェクト用途では問題なし

**対応策:**
- Phase 1 中は snapshot のままで OK
- Phase 2 で差分ベース化

### 🔴 検討が必要（アーキテクチャ）

⚠️ **フォルダ階層実装時の Layer::parent 指針**
- 現在：Linear array で管理（`std::vector<Layer>` ）
- 問題：Nesting 時に tree 構造が必要
- 選択肢：
  1. `Layer::parent` ポインタで tree 構造化
  2. Index-based tree（parent index 保持）
  3. `std::deque<Layer>` で reorder 効率化

**推奨:**
- Option 1（`Layer::parent` ポインタ）— シンプル、C++ 標準
- Renderer は recursive traversal で対応

---

## 🚀 4. 推奨される次の実装順序

### フェーズ分割（AI 選択除外）

#### **Phase 1-A: ベクター編集基盤化**（推定 3-4 weeks）

**優先度:** 🔴 Critical  
**理由:** Vector レイヤーの記録は完了、編集ツール不在では実用性ゼロ

**スコープ:**
1. **Vector Select Tool**（新規）
   - Point hover / selection
   - Selection UI（highlight）
   - Undo/redo 対応

2. **Vector Edit Tool**（新規）
   - Point move（drag）
   - Point delete（Del キー）
   - Point add（Shift+Click）

3. **Vector Split/Simplify**（新規）
   - Split at point
   - Simplify stroke（Douglas-Peucker）
   - Merge paths（未定）

4. **Polish**
   - Multi-point selection（Shift+Click）
   - Bounding box preview
   - `View` > `Show Vector Points` toggle

**コード範囲:**
- `src/core/tools/VectorEditTool.h/.cpp`（新規）
- `src/app/panels/ToolPropertyPanel.cpp`（Vector 用パラメータ追加）
- `VectorPath` アルゴリズム（simplify / split）

**テスト項目:**
- Vector layer でのPoint edit
- Undo/redo 動作
- Split 後の composite 正常

---

#### **Phase 1-B: フォルダ階層実装**（推定 1-2 weeks）

**優先度:** 🔴 Critical  
**理由:** 複数レイヤー整理のための基本機能

**スコープ:**
1. **Data Model**
   - `Layer::parent` ポインタ追加
   - `Layer::children` （optional）

2. **LayerPanel**
   - Recursive 行表示（インデント）
   - Expand/collapse toggle
   - Drag drop-into-folder

3. **Document**
   - `moveLayer()` で tree 対応
   - Undo/redo for reparent

4. **Renderer**
   - Tree traversal でのコンポジット

**コード範囲:**
- `src/core/layer/Layer.h` 修正
- `src/app/panels/LayerPanel.cpp` 重構成
- `src/core/document/Document.cpp` tree logic

**テスト項目:**
- Folder 作成 / delete
- Layer を folder へ drag
- Composite が正しい順序

---

#### **Phase 1-C: 複数レイヤー選択**（推定 1 week）

**優先度:** 🔴 Critical  
**理由:** 高速編集の基本

**スコープ:**
1. **LayerPanel**
   - Shift+Click / Ctrl+Click で複選
   - Visual highlight（複数行を背景色変更）

2. **AppController**
   - `setActiveLayerRange()` API
   - 複数 layer への操作（visibility toggle など）

3. **Undo/redo**
   - 複数 layer への同時操作記録

**コード範囲:**
- `LayerPanel.cpp` selection logic
- `AppController.h/.cpp` multi-layer API

**テスト項目:**
- Shift+Click で range 選択
- Ctrl+Click で individual 選択
- Visibility toggle on 複数 layer

---

#### **Phase 1-D: ペンタブ筆圧統合**（推定 2-3 weeks）

**優先度:** 🟡 High  
**理由:** 描画品質を大幅に向上（入力精度）

**スコープ:**
1. **Input Handling**
   - `QTabletEvent` 取得
   - `pressure / tilt / rotation` parse

2. **BrushTool**
   - Pressure → `BrushSettings::hardness / opacity` に反映
   - Tilt → `angle` に反映

3. **UI**
   - ToolPropertyPanel に「筆圧曲線」エディタ追加
   - 筆圧プレビュー（小ウィンドウ）

4. **Settings**
   - 筆圧曲線設定の保存 / 復元

**コード範囲:**
- `src/app/canvasview/CanvasWidget.cpp` event handling
- `src/core/tools/BrushTool.cpp` pressure processing
- `ToolPropertyPanel` 拡張

**テスト項目：**
- ペンタブでの pressure 値取得
- Brush size/opacity が pressure と連動
- 設定保存・復元

---

#### **Phase 1-E: Skia 本移植**（推定 3-4 weeks）

**優先度:** 🟡 High  
**理由:** GPU レンダリング・品質向上・CPU レンダラー技術債削減

**スコープ:**
1. **CPU Renderer 廃止**
   - `stampCircle()` → Skia `drawOval()`
   - `drawSegment()` → Skia `drawPath()`
   - `composite()` → Skia `drawBitmap()`

2. **Skia Default化**
   - `PAINT_USE_SKIA=ON` を cmake default
   - CPU fallback は画面外合成のみ

3. **Performance Test**
   - 4K canvas で 60 FPS 確認
   - Memory profile

4. **Cross-platform**
   - Vulkan / Metal バックエンド動作確認

**コード範囲:**
- `src/core/renderer/Renderer.cpp` 大幅改修
- `src/platform/skia/SkiaRenderer.cpp` feature complete
- CMAKE CPU renderer 削除

**テスト項目:**
- Raster / Vector 描画品質
- Performance（FPS、メモリ）
- Undo/redo 正常動作

---

### 時間軸イメージ

```
Week 1-4:    Phase 1-A（Vector 編集基盤）
Week 2-3:    Phase 1-B（Folder 階層）      ← 1-A と並列可能
Week 3-4:    Phase 1-C（複数 Layer 選択）  ← 1-A, B と並列可能
Week 4-7:    Phase 1-D（Pen tablet）
Week 5-8:    Phase 1-E（Skia 本移植）      ← 1-D と並列可能

※ 実際にはスケジューリング・優先度で調整
```

---

## 📈 5. 機能完成度レーダーチャート（概略）

```
                  Brush Quality
                        █ 90%
                       /   \
  Undo/Redo           /       \        Selection
   100% ███████████                  (30%)
        │             \             /
        │              \           /
        │               \         /
        │                \       /
  File I/O              \     /
   (80%)  ──────────────  Vector Edit
                          (0%)
                          ↑ ← Critical gap

Layer System:    [████░░░░░] 50%
Transform:       [██░░░░░░░░] 20%
Advanced:        [░░░░░░░░░░]  5% (Text, Gradient, Adjustment)
```

---

## 🎯 6. 最終推奨

### 即座に開始すべき（次 1-2 セッション）

**1️⃣ Vector 編集基盤（1-A）**
- Vector layer の実用化（記録→編集）
- 技術的リスク：低（data model 既存）
- UX 効果：高（実用度が劇的向上）
- リード時間：短（3-4 weeks）

**2️⃣ フォルダ階層（1-B）**（1-A と並列推奨）
- Layer 整理機能
- 技術的リスク：中（tree structure 新規）
- UX 効果：高（必須機能）
- リード時間：中（1-2 weeks）

**3️⃣ 複数 Layer 選択（1-C）**（1-B 後）
- 高速編集
- 技術的リスク：低
- UX 効果：中
- リード時間：短（1 week）

### 次フェーズ（4-8 weeks 後）

**4️⃣ Pen tablet 筆圧（1-D）**
- 入力品質向上
- 実装コスト：中（2-3 weeks）

**5️⃣ Skia 本移植（1-E）**
- GPU レンダリング
- 実装コスト：高（3-4 weeks）
- 利益：CPU レンダラー技術債削減 + GPU サポート

### 保留推奨（Phase 2+）

- クリッピンググループ
- ロックモード
- グラデーション / テキスト
- 調整レイヤー
- PSD インポート

---

## 結論

**AI 選択ツール除外時の優先度:**

🥇 **Vector 編集（1-A）** → 最優先  
🥈 **Folder 階層（1-B）** → 同時実行推奨  
🥉 **複数 Layer 選択（1-C）** → 1-B 後  

これらの 3 つを完成させると、**CSP/Krita の基本機能の 60～70%** に達し、実用的な描画アプリとしての評価が大幅向上します。

