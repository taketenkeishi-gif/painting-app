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

---

## ADR-010: 一時編集モードの History 設計原則

**決定日:** 2026-06-13
**ステータス:** ACCEPTED

### 問題

QuickMask モードで発生した Undo 漏れのデバッグで判明した設計原則。

```
Q ON → QuickMaskStroke × N → Q OFF
↓
Ctrl+Z（QM commit を Undo）
↓
QuickMaskStroke の undo ハンドラが
  if (m_quickMaskMode && m_quickMaskLayer) で判定
→ QM モードが false のため何もしない（無効化）
```

根本原因：**編集バッファのライフサイクルと History エントリの寿命が一致していなかった**。

### 決定

**一時編集モードは、確定（commit）時に「モード状態ごと」History エントリに保存する。**

具体的に保存すべき内容：
- edit buffer（ピクセルデータ）
- mode state（フラグ: quickMaskMode, isTransforming など）
- UI-visible state（selection, overlay, layer list）

ピクセル diff だけを保存することを禁止する。

### 適用対象（将来含む）

| モード | commit エントリ | undo 時に復元すべきもの |
|---|---|---|
| QuickMask | `QuickMaskCommit` ✅ 実装済み | QMバッファ + QMフラグ + 選択前状態 |
| Transform preview | `TransformCommit` (未実装) | 変形メッシュ + 変形フラグ + 元ピクセル |
| Liquify | `LiquifyCommit` (未実装) | メッシュ + 元バッファ |
| AI preview mask | `AiMaskCommit` (未実装) | pending mask + AI Select フラグ |

### 実装済み例（QuickMask）

```cpp
// HistoryKind::QuickMaskCommit
entry.beforeLayer     = qmSnapshot;    // QM バッファ（undo 時に復元）
entry.beforeSelection = selBefore;     // QM 確定前の選択
entry.afterSelection  = newSel;        // QM 確定後の選択（redo 用）

// undo ハンドラ
m_quickMaskMode = true;
m_quickMaskLayer.emplace(*entry.beforeLayer);
m_quickMaskSnapshot = entry.beforeSelection;
```

### Why

この原則を守らないと、一時モード中のストロークすべてが「Undo できても反映されない」という Silent Failure になる。  
ユーザーには「Ctrl+Z が効かない」に見えるが、コード上は正常にエントリをポップしているため検出が困難。

### 関連

- `AppController.cpp` `toggleQuickMaskMode()` OFF パス（~line 1854）
- `HistoryKind::QuickMaskCommit` / `HistoryKind::QuickMaskStroke`

---

## AI アーキテクチャ

### ADR-011: AI 責務レイヤー分離（UI → AiService → Provider → Transport）

**決定日:** 2026-06-14
**Status:** ACCEPTED（ComfyProvider 導入で確立）

**決定:** AI 機能の呼び出し経路を 4 層に固定する。

```
UI / Panel
    ↓
AiService         ← 唯一の AI 呼び出し窓口（Facade）
    ↓
ComfyProvider     ← バックエンド選択・クライアント生成
    ↓
ComfyClient       ← HTTP Transport（ComfyUI 固有）
```

**禁止パターン（将来実装時も維持）:**

| 禁止 | 理由 |
|---|---|
| Panel / Dialog → ComfyClient 直接参照 | UI が Transport 詳細を知るべきでない |
| AppController → ComfyClient 直接参照 | AppController は AiService に委譲する |
| Transport 層（ComfyClient）内でのワークフロー生成 | ワークフロー組み立ては上位層の責務 |
| AiService 内での ComfyClient 直接生成 | Provider を経由することで backend 差し替えが可能になる |

**理由:**

- **backend 差し替え容易性:** Comfy 以外の AI backend（ローカル推論・別サービス）を追加する際、Provider 層を追加するだけで AiService／UI に変更が不要
- **責務分散防止:** 過去に Panel が ComfyUiClient を直接保持していた経緯があり、テストと変更が困難だった
- **巨大化防止:** AiService に全ロジックを集約しないよう Provider で分割

**実装（確認済み）:**

- `AiService` — generate / inpaint の Facade。`ComfyProvider` のみ参照
- `ComfyProvider` — `ComfyClient` の生成・URL 管理を担う薄いラッパー
- `ComfyClient` — ComfyUI HTTP API（upload / queue / poll / fetch）
- `ComfyUiClient` — WebSocket ベースの旧クライアント（Panel 直結用途に限定）

**新 AI 機能を追加するときの配置判断:**

```
ワークフロー組み立て・バインディング  → AiService / AiGenerationController
バックエンド切り替え・クライアント管理 → Provider 層（ComfyProvider 等）
HTTP / WebSocket 通信                → Transport 層（ComfyClient 等）
UI への結果反映・エラー表示           → Panel が AiService シグナルを受信
```

**参考:**

- `src/app/bridge/AiService.h`
- `src/app/bridge/ComfyProvider.h`
- `src/platform/comfy/ComfyClient.h`


---

## 2026-06-14: Task 9 — LoRA/Checkpoint プリセット E2E 検証完了

**決定:** `ai-generate-with-preset` debug action は `runGenerateWithWorkflow()` を呼ばず `m_aiService->generate()` を直接呼ぶ

**理由:**

- `ensureComfyRunning()` の TCP 300ms タイムアウトが Windows 環境でしばしば失敗し、非同期パスに落ちる
- 非同期パス（ready シグナル経由）は動作するが遅延が大きく検証ループが詰まる
- debug action は検証目的のみ。production の `ensureComfyRunning` ロジックは変更不要

**修正内容（AppController.cpp）:**

1. `runGenerateWithWorkflow` 内の `seed` 生成: `generate() & 0x7FFFFFFFu` — 負の値防止
2. `ai-generate-with-preset` ハンドラー: `m_aiService->generate(req, 1)` を直接呼ぶ（`runGenerateWithWorkflow` バイパス）

**E2E 検証結果（VERIFIED）:**

| 検証項目 | 値 |
|---------|---|
| CheckpointLoaderSimple.ckpt_name | illustriousXL20_v20.safetensors |
| LoraLoader.lora_name | style\UMI_style#3.safetensors |
| LoraLoader strength_model/clip | 0.55 / 0.66 |
| CLIPTextEncode (positive) text | test_prompt_marker lora_test_marker |
| CLIPTextEncode (negative) text | bad quality |
| KSampler.seed | 42（正値確認） |
| AI レイヤー追加 | "AI Generated" レイヤー生成 |

**参考:** `src/app/bridge/AppController.cpp` — `ai-generate-with-preset` handler
