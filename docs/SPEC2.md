# 実装仕様 — Paint App

> 詳細なビジョンは `SPEC.md` を参照。
> このドキュメントは技術実装の設計書。

---

## アーキテクチャ概要

### レイヤリング

- **`src/core`** — Qt 非依存な描画コア（Document / Layer / Pixel / Render / Tool ロジック）
- **`src/app`** — Qt Widgets UI シェル、入力処理、パネルメニュー
- **`src/platform/qt`** — 変換境界（`PixelBuffer ↔ QImage`）
- **`src/platform/skia`** — Skia バックエンド（`PAINT_USE_SKIA=ON` 時）

### App Boundary

**AppController が UI と core を橋渡し。**

- Panels / CanvasWidget は AppController API のみを呼び出し
- core 型は Qt フリーを維持

---

## ツールシステム

### ツール体系

- **ToolManager** (`core`) が実行時のツール切り替えを制御
- **UI 定義は data-driven** (`app/ui`):
  - `ToolDescriptor` — ツールメタデータ
  - `SubToolDescriptor` — サブツール（プリセット）定義
  - `ToolCatalog` — ツール / サブツール管理
  - `UiState` — UI 状態

### ブラシシステム

**サブツールプリセット駆動:**

- `size / opacity / hardness / flow / spacing` — 基本パラメータ
- `stabilization / post-correction / velocity-correction` — 補正制御
- `angle / roundness / taper` — 形状パラメータ（UI 公開）
- `targetLayerKind` (`Raster` / `Vector` / `Both`) — アクティブレイヤー型に応じた可用性制御

**サブツールプリセットは実行時編集可能:**
- duplicate / rename / delete / reset

### ブラシパラメータ詳細

| パラメータ | 型 | 説明 |
|---|---|---|
| `size` | float | ブラシ径（px） |
| `opacity` | float | 0.0~1.0 |
| `hardness` | float | 0.0~1.0（0=ガウス、1=ハード） |
| `flow` | float | 0.0~1.0（インク流量） |
| `spacing` | float | スタンプ間隔（%） |
| `angle` | float | ブラシ角度（度） |
| `roundness` | float | 0.0~1.0（楕円率） |
| `taper` | float | 0.0~1.0（先細） |
| `velocity` | bool | 速度感応 ON/OFF |
| `antiAlias` | bool | アンチエイリアス ON/OFF |

---

## レイヤーモデル

### Layer 型定義

```cpp
enum class LayerKind {
  Raster,    // ピクセルバッファベース描画
  Vector,    // パスリスト VectorPath をコンポジット時にレンダリング
  Folder,    // 非レンダリング組織プレースホルダー
};
```

### Layer フラグ

- `clippedToBelow` — クリッピングマスク有効
- `hasMask / maskEnabled` — レイヤーマスク
- `visibility` — 表示 / 非表示
- `opacity` — 透明度（0.0~1.0）

### Raster レイヤー

- ピクセルバッファ（`PixelBuffer`）を保持
- ブラシ / 消しゴム / 直線ツール で直接描画

### Vector レイヤー

- `VectorPath` リスト（ストロークレコード）を保持
- コンポジット時に一時バッファにラスタライズ
- **ライブプレビュー** — 描画中にオーバーレイ表示

### Folder レイヤー

- 非レンダリング（バウンディングボックスのみ）
- 将来の階層構造対応を想定

### コンポジット

- `Renderer::composite()` が前景レイヤーから背景へ合成
- クリッピング・マスク・透明度を適用
- アルファブレンディングで結合

---

## ツール詳細

### ブラシツール

**対応レイヤー:** Raster / Vector / Both

**Raster レイヤー:**
- ピクセルバッファに直接描画
- ガウシアンブラシで柔らかいエッジ

**Vector レイヤー:**
- ストロークを `VectorPath` に記録
- リアルタイムプレビュー（ベクターオーバーレイ）
- コンポジット時にラスタライズ

### 消しゴムツール

**対応モード:**
- `Normal` — アルファ削減
- `Vector erase` — ベクターレイヤー上で削除（要実装）

### 直線ツール

**対象:**
- Raster: ピクセル直線描画
- Vector: ベクター直線記録

### 選択ツール

**実装済み:**
- 矩形選択
- clear / invert / all / deselect
- undo / redo

**未実装:**
- なげなわ / 多角形 / クイックマスク
- feather / grow / shrink

### 塗りつぶしツール（Fill）

**機能:**
- contiguous fill （隣接領域）
- non-contiguous fill （同色全体）
- gap close （小さな隙間を埋める）
- refer-layer mode （参照レイヤー指定）
- tolerance threshold （色の許容値）
- erase mode （透明に塗りつぶし）
- Selection mask 対応 （選択範囲内のみ）

### 移動ツール（Move）

**対象:**
- アクティブレイヤー移動
- ピクセル + ベクターレイヤー対応

---

## UI 構成

### ドック構成（デフォルト）

```
┌─────────────────────────────────┐
│           メニューバー           │
├────────────────────┬──────────────┤
│ Left Docks         │  Canvas      │ Right Docks
│ ┌────────────────┐ │              │ ┌──────────┐
│ │ Tool Panel     │ │              │ │ Layer    │
│ │ Sub-Tool Panel │ │              │ │ Panel    │
│ │ Tool Property  │ │              │ │          │
│ │ Color Wheel    │ │              │ │ Navigator│
│ └────────────────┘ │              │ └──────────┘
├────────────────────┼──────────────┤
│              StatusBar           │
└─────────────────────────────────┘
```

### パネル詳細

#### ToolPanel
- アイコングリッド表示
- 16px × 16px アイコン
- レイヤー型非対応ツールはグレーアウト（opacity 0.30）

#### SubToolPanel
- 選択中ツールのプリセット一覧
- duplicate / rename / delete / reset アクション
- ドラッグ並び替え（未実装）

#### ToolPropertyPanel
- スライダー / スピンボックス / コンボボックス
- リアルタイム UI 反映
- アイコンヘッダー（20×20px）

#### ColorSwatchWidget
- FG / BG スウォッチ（48×48px HiDPI）
- 角丸、スワップボタン
- 右クリック色選択

#### LayerPanel
- レイヤー行リスト
- 行頭: `[R]` / `[V]` / `[F]` で LayerKind を表示
- visibility checkbox / eye icon
- opacity スライダー + スピンボックス
- buttons: Add / Delete / Duplicate / Merge Down / Rasterize
- Up / Down buttons — レイヤー順序変更

#### NavigatorPanel
- コンポジット出力のプレビュー
- マウス領域で canvas ズーム制御
- クイックボタン: `100%` / `Fit Screen`

### Window メニュー

- **Dock Panel Control** — 各パネル表示 / 非表示
- **Reset Workspace** — デフォルトレイアウト復元
- **Workspace Layout Save/Load/Delete** — カスタムレイアウト保存
- **Restore Last Workspace** — 最後のレイアウト復元

---

## UI テーマ・トークン

### カラーシステム

| トークン | RGB | 用途 |
|---|---|---|
| `Base` | `#13151c` | 背景（最暗） |
| `Surface` | `#1a1d27` | パネル背景 |
| `SurfaceVariant` | `#26293a` | 区切り線・ボーダー |
| `Accent` | `#4e8ef7` | ハイライト・活性状態 |
| `Text` | `#e8eaed` | フォーカス時テキスト |
| `TextSecondary` | `#9aa0a6` | 非フォーカステキスト |
| `Border` | `#42474f` | ボーダー / セパレータ |

### コンポーネント

- **Button** — `Surface` 背景、`Accent` ホバー、`Border` エッジ
- **Slider** — `TextSecondary` トラック、`Accent` つまみ
- **ComboBox** — `Surface` 背景、`Accent` アイテム選択
- **Docks** — `Base` 背景、`SurfaceVariant` ボーダー

---

## 選択範囲・マスク

### SelectionMask

- `std::vector<uint8_t>` — アルファマスク（document.width × document.height）
- 各ピクセル 0~255 で選択度を表現
- マーチングアンツ表示（キャンバスオーバーレイ）

### クリッピングマスク

- レイヤーの `clippedToBelow` フラグ
- Renderer がコンポジット時に下のレイヤーのアルファを参照

### レイヤーマスク

- `Layer::maskBuffer` — 別のピクセルバッファ
- `hasMask / maskEnabled` フラグ
- コンポジット時に適用

---

## ファイル I/O

### 形式対応

- **読み込み**
  - JPEG / PNG （QImage via Qt）
  - ネイティブ（未実装 — フォーマット検討中）
  
- **書き出し**
  - PNG / JPEG
  - flattened export

### Clipboard I/O

- QImage ↔ PixelBuffer 変換（`platform/qt`）
- 「新規プロジェクトとしてペースト」対応
- 「レイヤーとしてインポート」対応

---

## 履歴（Undo/Redo）

### 対象操作

- ✅ ブラシ / 消しゴム / 直線 / 塗りつぶし
- ✅ レイヤー可視性トグル
- ✅ レイヤー順序変更
- ✅ レイヤー opacity / blendMode
- ✅ 選択変更（作成 / clear / invert）
- ❌ キャンバスリサイズ（未実装）

### 実装方法

**スナップショットベース** (`before / after`)

- 安全性と単純性を優先
- 操作前後のバッファをコピー

---

## データモデル（コア）

### Document

```cpp
struct Document {
  size_t width;
  size_t height;
  float dpi;
  std::vector<std::unique_ptr<Layer>> layers;
  std::optional<SelectionMask> selection;
};
```

### Layer

```cpp
struct Layer {
  std::string name;
  LayerKind kind;  // Raster / Vector / Folder
  bool visible;
  float opacity;
  
  std::optional<PixelBuffer> rasterBuffer;  // Raster のみ
  std::vector<VectorPath> vectorPaths;      // Vector のみ
  
  bool clippedToBelow;
  std::optional<PixelBuffer> maskBuffer;
  bool maskEnabled;
};
```

### VectorPath

```cpp
struct VectorPath {
  std::vector<FPoint> points;  // float 精度
  float pressure;              // 筆圧
  BrushSettings brushSettings; // ブラシパラメータ
};
```

### PixelBuffer

```cpp
struct PixelBuffer {
  size_t width;
  size_t height;
  std::vector<uint8_t> data;  // RGBA (width * height * 4)
};
```

---

## コンパイル構成

### CMake フラグ

| フラグ | デフォルト | 効果 |
|---|---|---|
| `PAINT_USE_SKIA` | OFF | Skia バックエンド有効化 |
| `PAINT_USE_ONNX` | OFF | ONNX AI 推論有効化 |
| `PAINT_BUILD_TESTS` | OFF | 自動テスト有効化 |

### 依存ライブラリ

| ライブラリ | 用途 | バージョン |
|---|---|---|
| Qt | UI フレームワーク | 6.7.2 |
| Skia | 2D レンダリング（optional） | m126+ |
| ONNX Runtime | AI 推論（optional） | 1.18+ |
| GoogleTest | テストフレームワーク | 1.14+ |

---

## 拡張性方針

### 新規サブツール追加

1. `ToolCatalog` でプリセット定義
2. Deep UI ロジックを変更しない
3. 将来的に JSON 外部定義対応可

### 新規プロパティコントロール

1. descriptor キー追加
2. `ToolPropertyPanel` バインディング追加
3. core 動作契約を変更しない

### バックエンド置き換え

- Renderer インターフェース経由で実装交換可能
- Qt/Skia の境界は `src/platform/` に集約

---

## パフォーマンス目標

- **ブラシ描画** — 60 FPS @ 4K canvas
- **レイヤー合成** — 100ms 以下 @ 10 layers
- **ファイル I/O** — 2MB/s 以上
- **Undo スナップショット** — メモリ 500MB 上限（メモリ効率化は Phase 1+）

