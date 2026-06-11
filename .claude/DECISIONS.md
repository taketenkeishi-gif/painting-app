# 設計判断記録 — Paint App

重要な技術判断・アーキテクチャ決定を時系列で記録。

---

## Phase 0 — 描画エンジン根本再構築（最優先）

### ADR-001: Google Skia を 2D レンダリングバックエンド として採用

**決定日:** 2026-05-xx  
**決定:** Google Skia（Chrome / Flutter / Android が使用）を GPU レンダリングバックエンド として採用

**理由:**

- **品質:** Chrome が使うレベルの品質（アンチエイリアス・サブピクセル精度・GPU アクセラレーション）
- **ライセンス:** MIT ライセンス互換（商用利用可）
- **実績:** 数十億デバイスで実証済み
- **API:** ローカルライブラリ（ネットワークボトルネックなし）
- **将来性:** GPU 時代に対応

**代替案:**

1. ❌ **Qt Raster バックエンド** — CPU 描画のみ、GPU サポートなし
2. ❌ **自作 GPU レンダラー** — 工数が膨大（数百時間）
3. ❌ **Vulkan / OpenGL 直接** — Skia の方が高レベル

**影響範囲:**

- `src/platform/skia/` — Skia ラッパー層を新規作成
- `src/platform/qt/` — Qt との境界（SkBitmap ↔ QImage 変換）
- CMake に Skia 依存追加（vcpkg）
- PAINT_USE_SKIA フラグで optional（デフォルト OFF）

**実装状況:**
- ✅ SkiaPixelBuffer / SkiaRenderer 実装完了
- ✅ Default OFF（既存 CPU レンダラー互換）
- ⏳ 本移植（Phase 1）

**参考:** `docs/SPEC.md` Phase 0-1

---

### ADR-002: ブラシエンジンは MyPaint / Skia ハイブリッド戦略

**決定日:** 2026-05-xx  
**決定:** ブラシエンジンは MyPaint ライクな物理ベース実装 + Skia GPU アクセラレーション

**理由:**

- **MyPaint:** GIMP / MyPaint が使用（10年以上の実績）
- **Skia:** GPU で高速化可能（ブラシスタンプを GPU で描画）
- **ハイブリッド:** CPU 物理計算 + GPU 描画で品質と速度の両立

**実装方針:**

1. CPU で筆圧曲線・速度感応・テクスチャグレイン を計算
2. Skia / GPU で actual stamp（ガウシアン / テクスチャ）を描画

**代替案:**

1. ❌ 完全自作（工数 200+ 時間）
2. ❌ libmypaint 直接統合（ライセンス複雑性）
3. ✅ Skia パスストローク活用（現在進行中）

**実装状況:**
- ✅ 基本ダイナミクス完了（velocity, hardness, opacity）
- ✅ テクスチャグレイン（セルノイズ）
- ✅ Wet-mix / Smear
- ⏳ AA 描画（Skia 統合）

**参考:** `docs/SPEC.md` Phase 0-2

---

### ADR-003: ベクターレイヤー は「記録 + ラスタライズ合成」戦略

**決定日:** 2026-06-xx  
**決定:** Vector データはストロークポイントリストで記録、コンポジット時にラスタライズ合成

**理由:**

- **シンプル:** Raster と Vector を統一的に composite できる
- **品質:** 合成時に AA を一括かけられる
- **実装効率:** Skia の SkPath + SkPaint で AA は自動

**代替案:**

1. ❌ Vector だけ SVG DOM で保持 — 複合描画が複雑
2. ❌ 完全 Vector 出力 — MV 連携時に精度問題の可能性

**実装:**
- VectorPath = `std::vector<FPoint>` + BrushSettings
- Renderer が描画時に SkPath にマッピング
- ライブプレビュー = CanvasWidget のオーバーレイ描画

**実装状況:**
- ✅ VectorPath float 化完了
- ✅ ライブプレビュー実装
- ✅ ラスタライズ基本実装
- ⏳ ベクター編集ツール（Point edit など）

**参考:** `docs/SPEC2.md` Vector System

---

## ツール・UI 設計

### ADR-004: ToolDescriptor は「Code-driven」（JSON は不採用）

**決定日:** 2026-05-xx  
**決定:** ツール定義は `src/app/ui/ToolDescriptor.cpp` の code-driven 形式で、JSON 外部定義は採用しない

**理由:**

- **開発速度:** Code 変更とビルドで確実に反映
- **型安全:** JSON parse の実行時エラーなし
- **デバッグ:** IDE のリファクタリングツールが使える

**代替案:**

1. ❌ JSON 外部定義 — 開発速度落ちる（JSON ↔ parse ↔ QVariant）
2. ❌ XML スキーマ — さらに複雑

**将来性:**
- JSON 外部定義は Phase 2+（ユーザープラグイン対応時）

**参考:** `src/app/ui/ToolDescriptor.h` / `ToolCatalog.cpp`

---

### ADR-005: LayerPanel の「行インデックス明示」設計

**決定日:** 2026-05-xx  
**決定:** LayerPanel は「row ↔ layer index」を明示的にマッピング、ドラッグ reorder は index 直接操作

**理由:**

- **UI 直感性:** ユーザーが見た行 = code が動かす index
- **バグ低減:** index 計算ずれが起きない
- **Undo:** Document::moveLayer で一元管理

**実装:**
```cpp
// LayerPanel::onLayerUp() 
// row 上移動 = layer array で index を上げる
if (m_activeLayerIndex > 0) {
  document->moveLayer(m_activeLayerIndex, m_activeLayerIndex - 1);
}
```

**参考:** `src/app/panels/LayerPanel.cpp`

---

## AI 統合

### ADR-006: SAM2 ONNX を AI 選択ツール として採用

**決定日:** 2026-06-xx  
**決定:** SAM2 (Segment Anything Model 2) を ONNX Runtime 経由で統合、選択ツールのバックエンド として採用

**理由:**

- **精度:** Meta が提供する最新の物体検出（COCO-2014 で最高精度）
- **ローカル:** API 不要、オフライン実行
- **軽量:** ONNX フォーマット（Web ブラウザでも動作）

**代替案:**

1. ❌ 外部 API（Stable Diffusion API など）— オフライン要件に不適合
2. ❌ YOLOv8 — リアルタイムですが精度が劣る
3. ✅ SAM2 ONNX — 精度・速度・ローカルの全要件満たす

**実装:**
- `src/core/ai/OnnxSegEngine.h/.cpp` — SAM2 ラッパー
- `PAINT_USE_ONNX=OFF` 時はスタブ（互換性維持）
- `AiSelectTool::Settings::granularity` で細部 ↔ 全体を制御

**セットアップ:**
```powershell
# 1. モデルダウンロード
powershell -File .\scripts\download_sam2.ps1

# 2. vcpkg インストール
vcpkg install onnxruntime:x64-windows

# 3. ビルド
cmake -DPAINT_USE_ONNX=ON -B build
```

**参考:** `.claude/HANDOFF.md` SAM2 ONNX 項目

---

## UI テーマ・アクセシビリティ

### ADR-007: Dark Theme を「Color Token」ベース で実装

**決定日:** 2026-06-xx  
**決定:** UI テーマを色トークン（`Base`, `Surface`, `Accent` など）の集中管理で実装

**理由:**

- **保守性:** 色定義が一箇所
- **拡張性:** Light theme 追加が簡単
- **アクセシビリティ:** コントラスト比を一括検証可能

**色定義:**

| Token | RGB | 用途 |
|---|---|---|
| `Base` | `#13151c` | 背景（最暗） |
| `Surface` | `#1a1d27` | パネル |
| `SurfaceVariant` | `#26293a` | ボーダー |
| `Accent` | `#4e8ef7` | ハイライト |

**参考:** `docs/SPEC2.md` Color System

---

---

## レイヤーシステム設計

### ADR-008: レイヤーオフセットモデル（Canvas-Bound → Independent PixelBuffer）

**決定日:** 2026-06-10  
**Status:** ACCEPTED（未実装）  
**決定:** `Layer` クラスに `offsetX/offsetY` フィールドを追加し、`PixelBuffer` をキャンバスサイズに依存しない独立サイズで持てるようにする。

**背景:**

本番準備度レビュー（2026-06-10）により、現在のアーキテクチャが CSP/Photoshop ユーザーの期待する動作を構造上実現できないことが判明。具体的な制約:
- ペースト時にキャンバス外ピクセルが即時破棄される
- レイヤー移動でキャンバス端のピクセルが消失（不可逆）
- `PsdExporter.cpp` で `layer.offsetX()` が呼ばれているがメソッドが存在しない（コンパイル不可）

**検討した3案:**

1. ❌ **Case A: Temporary Floating Transform のみ** — 現状維持。目標 UX を構造上実現不可能
2. ✅ **Case B: Layer Offset + Independent PixelBuffer**（採用）— 業界標準。PSD 互換。段階移行安全
3. ❌ **Case C: Full Transform Matrix per Layer** — 過剰。ブラシ描画と根本的に相性が悪い。工数 5〜10 倍

**採用理由:**

- CSP / Krita / Photoshop が採用する業界標準モデル
- `offset = 0` をデフォルトにすることで既存動作を一切壊さずに段階的移行が可能
- PSD フォーマットが同一モデルのため、インポート/エクスポートが構造的に正しくなる

**移行リスク:**

- Phase 0〜2 は低リスク（offset フィールド追加、Renderer 変換、MoveLayer 簡素化）
- Phase 3（ブラシの動的バッファ拡張）が最高リスク。全描画ツールに波及するため十分なテスト必須

**影響範囲:** `Layer.h`, `Renderer`, `BrushEngine` 全ツール, `MoveLayerTool`, `FreeTransformTool`, LPA フォーマット, PSD Exporter

**詳細:** [ADR-008](../docs/adr/ADR-008-layer-offset-model.md)

---

## 今後の重要判断ポイント

### 未決定項目

| 項目 | 議論状態 | 次の判断時期 |
|---|---|---|
| ネイティブファイルフォーマット | 設計検討中 | Phase 1 終盤 |
| ペンタブ API（Wintab vs Windows Ink） | 実装前 | Phase 1 中 |
| Layer Offset Model 実装開始タイミング | ADR-008 ACCEPTED、実装待ち | Phase 1 中盤〜後半 |
| クラウド連携 | オプション方針 | Phase 2 |
| Plugin API | 将来 (JSON definition) | Phase 2+ |

### 参照

- [SPEC.md - ビジョン・フェーズ](docs/SPEC.md)
- [SPEC2.md - 実装仕様](docs/SPEC2.md)
- [PROJECT_STATUS.md - 進捗](PROJECT_STATUS.md)

