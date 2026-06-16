# ADR-008: レイヤーオフセットモデル（Canvas-Bound から Independent PixelBuffer へ）

**Date:** 2026-06-10  
**Status:** `ACCEPTED`  
**Relates:** Phase 1 後半, SPEC.md, architecture.md

---

## Context

### 現在の制約（Canvas-Bound Layer Model）

現在の実装はすべてのラスターレイヤーがキャンバスサイズに固定され、原点(0,0)に整列している。

```
Document (1920×1080)
  ├─ Layer A: PixelBuffer(1920×1080)  ← 常にキャンバスサイズ
  ├─ Layer B: PixelBuffer(1920×1080)  ← 同上
  └─ Layer C: PixelBuffer(1920×1080)  ← 同上
```

コードで確認された制約:

| 制約 | コード根拠 |
|---|---|
| Layer に offset フィールドなし | `src/core/layer/Layer.h` — x/y 位置プロパティ不在 |
| 新規レイヤーは常にキャンバスサイズ | `Document::addLayer()` → `Layer(name, canvasSize.width, canvasSize.height)` |
| ペースト時にキャンバス外ピクセルを破棄 | `pasteBufferAsNewRasterLayer()` → `if (!layer.buffer().inBounds(x,y)) continue` |
| レイヤー移動はピクセルの物理コピー | `MoveLayerTool` — ピクセルを新座標にコピー、端が消失（不可逆）|
| Renderer はオフセットなしで座標直接参照 | `sourceBuffer->pixel(x, y)` — オフセット計算なし |
| マスクも同一座標で直接アクセス | `layer.maskBuffer().pixel(x, y)` — サイズ固定前提 |
| SelectionMask はキャンバスサイズ固定 | `SelectionMask(width, height)` → キャンバスサイズで生成 |

### 既存バグ（コンパイル不可箇所）

`src/app/bridge/PsdExporter.cpp` に `layer.offsetX()`, `layer.offsetY()` の呼び出しがあるが、
`Layer` クラスにこれらのメソッドが存在しない。本 ADR の実装後に自然解消される。

### ユーザーが期待する CSP/Photoshop 互換動作

- 2000×2000 の画像を 1920×1080 のキャンバスにペーストしたとき、元解像度を維持する
- キャンバス外のピクセルが隠れるが保持される（編集可能）
- Ctrl+T で元画像サイズ全体を変形できる
- 移動でキャンバス外に出た部分を後から取り戻せる
- 保存/読込で off-canvas ピクセルが失われない

---

## Decision

**Layer に `offsetX`, `offsetY` フィールドを追加し、PixelBuffer をキャンバスサイズに依存しない
独立したサイズで持てるようにする（Case B: Layer Offset + Independent PixelBuffer Size）。**

```
変更後のモデル:
  Layer {
    PixelBuffer buffer;  // 任意サイズ（元画像サイズ等）
    int offsetX = 0;     // キャンバス座標での X 位置（デフォルト 0）
    int offsetY = 0;     // キャンバス座標での Y 位置（デフォルト 0）
    PixelBuffer maskBuffer; // buffer と同サイズ（レイヤーローカル座標）
  }

  キャンバス座標 (cx, cy) → バッファ座標 (bx, by):
    bx = cx - layer.offsetX()
    by = cy - layer.offsetY()
    if (!buffer.inBounds(bx, by)) → 透明として扱う
```

### なぜこの決定か

| 項目 | 評価 | 理由 |
|---|---|---|
| **業界標準** | ⭐⭐⭐⭐⭐ | CSP / Krita / Photoshop が同一モデルを採用 |
| **PSD 互換性** | ⭐⭐⭐⭐⭐ | PSD はまさに「レイヤーオフセット + 独立バッファ」形式。インポート/エクスポートが構造的に正しくなる |
| **メモリ効率** | ⭐⭐⭐⭐☆ | 小さい描画がキャンバス全体を確保しない |
| **非破壊移動** | ⭐⭐⭐⭐⭐ | MoveLayer がオフセット値の変更のみになり、ピクセル消失がなくなる |
| **移行安全性** | ⭐⭐⭐⭐⭐ | offset = 0 をデフォルトにすることで、既存動作を一切壊さずに段階的に導入可能 |

---

## Alternatives

### Case A: ❌ Temporary Floating Transform Only（現状維持）

```
構造: Layer.buffer = 常にキャンバスサイズ
      FreeTransformTool のみが一時的に任意サイズバッファを保持
      変形確定時にキャンバスにベイク（現状そのもの）
```

- **メリット:** 変更ゼロ
- **デメリット:** ペースト時にキャンバス外ピクセルが即時破棄される。移動で端ピクセルが消失（不可逆）。CSP/Photoshop の期待動作が**構造上実現不可能**

**不採用理由:** 目標 UX を満たせない。

---

### Case B: ✅ Layer Offset + Independent PixelBuffer Size（採用）

```
構造: Layer に offsetX/Y を追加。PixelBuffer は元画像サイズで独立
      Renderer が canvas座標 → buffer座標 に変換
```

- **メリット:** 業界標準モデル。PSD 互換。非破壊移動。メモリ効率。段階移行安全
- **デメリット:** Renderer・ブラシ等の座標変換追加（中規模工数）。ブラシのバッファ動的拡張ロジックが必要

**採用。**

---

### Case C: ❌ Full Transform Matrix Per Layer

```
構造: Layer に AffineTransform matrix を持たせる
      キャンバス座標 → inverse(transform) * (cx, cy) でバッファ座標を得る
```

- **メリット:** 非破壊回転・スケールが可能。After Effects 的なワークフロー
- **デメリット:**
  - ブラシ描画が根本的に困難（回転済みレイヤーへの描画はピクセルグリッドが斜めになりリアルタイム逆変換が必要）
  - Renderer が全ピクセルにバイリニア補間必須
  - 選択マスクのマッピングが非直交座標になり破綻
  - CSP/Krita の通常レイヤーはこのモデルではない（Photoshop の Smart Object のみ限定採用）
  - 工数が Case B の 5〜10 倍

**不採用理由:** ペイントアプリの通常レイヤーモデルとしては過剰。将来の「Smart Object レイヤー」として**別種レイヤー**で検討する領域。

---

## Implementation

### Phase 0 — 準備（リスクゼロ・既存動作変化なし）

```cpp
// Layer.h に追加
class Layer {
  int m_offsetX = 0;  // デフォルト 0 → 既存動作は全て維持
  int m_offsetY = 0;
public:
  int offsetX() const { return m_offsetX; }
  int offsetY() const { return m_offsetY; }
  void setOffset(int x, int y) { m_offsetX = x; m_offsetY = y; }
};
```

この変更のみで PsdExporter のコンパイルエラーが解消される。

---

### Phase 1 — Renderer 対応（コア変更）

Renderer のピクセルアクセスに座標変換を追加。変換パターンは全箇所で統一。

```cpp
// Renderer.cpp — compositeInto() 内のレイヤー読み出し
// 変更前:
Color src = layer.buffer().pixel(x, y);

// 変更後:
const int bx = x - layer.offsetX();
const int by = y - layer.offsetY();
if (!layer.buffer().inBounds(bx, by)) continue; // off-buffer = 透明
Color src = layer.buffer().pixel(bx, by);

// マスクも同一変換
const Color mask = layer.maskBuffer().pixel(bx, by);
```

対象箇所: `compositeInto()` 内のレイヤー読み出し（5〜6箇所）、`applyLayerMask()`、ベクターラスタライズ

---

### Phase 2 — MoveLayer + FreeTransform + Paste

```cpp
// MoveLayerTool — ピクセルコピー → オフセット変更に置換（Phase 2）
// 変更前: ピクセルを新座標にコピー（端が消失）
// 変更後:
layer.setOffset(layer.offsetX() + dx, layer.offsetY() + dy);

// Paste — 元画像サイズでバッファ作成 + 中央配置オフセット
const int offX = (canvasW - buffer.width()) / 2;
const int offY = (canvasH - buffer.height()) / 2;
newLayer.setOffset(offX, offY);
newLayer.buffer() = std::move(buffer); // 元サイズのまま保持
```

---

### Phase 3 — Brush バッファ動的拡張（最高リスク）

ブラシがレイヤーバッファ外に描画した場合のバッファ拡張ロジック。

```
ストローク開始時: 必要範囲を事前計算
バッファ外に到達: バッファを拡張 + offset を再計算 + ピクセルを再配置
拡張後: 既存ピクセルは新バッファの正しい位置に移動済み
```

これは全描画ツール（BrushTool / EraserTool / FillTool / LineTool / CurveTool / GradientTool）に影響する。
**十分なユニットテストが必要。**

---

### Phase 4 — LPA v2 フォーマット更新

```json
// project.json に offsetX/Y を追加
{
  "version": 2,
  "layers": [
    {
      "name": "pasted image",
      "kind": "raster",
      "offsetX": -40,
      "offsetY": -60,
      "opacity": 1.0
    }
  ]
}
```

後方互換: version 1 の .lpa を読込時は `offsetX/Y = 0` をデフォルトとして完全互換を維持。

---

### 影響コンポーネント一覧

| コンポーネント | 変更規模 | リスク | 詳細 |
|---|---|---|---|
| `Layer.h` | 小 | **低** | フィールド追加のみ |
| `Renderer` | 中 | 中 | 5〜6箇所の座標変換。パターン統一 |
| `BrushEngine` 全ツール | 大 | **高** | バッファ動的拡張。全描画ツールに波及 |
| `SelectionMask` | なし | **低** | キャンバスサイズ固定を維持（Photoshop 方式）|
| `Masks` | 中 | 中 | Renderer と同一変換パターン |
| `MoveLayerTool` | 小 | **低** | ピクセルコピー → offset 変更に簡素化 |
| `FreeTransformTool` | 中 | 中 | 変形確定時のベイクロジック変更 |
| `VectorLayer` | 小 | **低** | 座標系維持、ラスタライズのみ変更 |
| `TextLayer` | 小 | **低** | originX/Y が自然に offset に対応 |
| LPA フォーマット | 小 | **低** | version bump + フィールド追加。後方互換維持 |
| PSD Exporter | 正の影響 | **低** | 既存コンパイルエラーが解消 |
| Undo System | 中 | 中 | Layer スナップショットに offset が自然に含まれる |

---

## Consequences

### ✅ 期待される効果

- CSP/Photoshop ユーザーが期待する「off-canvas ピクセル保持」「非破壊移動」「元解像度ペースト」が実現する
- PSD インポート/エクスポートが構造的に正しくなる（PSD は同一モデル）
- MoveLayerTool がオフセット変更のみになり、実装が簡素化される
- メモリ効率が改善する（小さいレイヤーがキャンバス全体を確保しない）
- PsdExporter のコンパイルエラーが Phase 0 で即解消

### ⚠️ 制約・リスク

- **Phase 3（ブラシ動的拡張）が最高リスク** — 十分なテストなしに実施しない
- Phase 0〜1 は offset=0 デフォルトのため既存テストを全てパスするはず
- Undo スナップショットのメモリ使用パターンがレイヤーサイズに依存するようになる
- CanvasWidget の「ビューポートに表示されているレイヤー bounds」計算が複雑化

---

## Related

- **調査元:** 本番準備度レビュー (2026-06-10) — 「選択範囲が描画をマスクしない」「移動で端ピクセル消失」の根本原因として特定
- **ADR-001:** [Skia バックエンド](ADR-001-skia-backend.md) — Phase 3 の動的拡張はバッファ再確保と Skia の相互作用を要確認
- **実装状況:** [PROJECT_STATUS.md](../../.claude/PROJECT_STATUS.md)
- **判断記録:** [DECISIONS.md](../../.claude/DECISIONS.md)

---

## References

- [Krita Layer Architecture](https://docs.krita.org/en/reference_manual/layers_and_masks/layers.html)
- [PSD File Format: Layer Records](https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/#50577409_72092)
- CSP 内部実装参考: レイヤーオフセットは PSD 互換の `top/left/bottom/right` bounds 形式
