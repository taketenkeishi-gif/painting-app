# プロジェクト状態 — Paint App

**Last Updated:** 2026-06-11  
**Latest Commit:** `b97c990` (fix: preserve off-canvas pixels during high quality transform)  
**Current Phase:** Phase 1 実装中（Layer Offset Model Phase 0-2 完了）

---

## 実装進捗サマリー

### Phase 0 — 描画エンジン根本再構築

**進捗度:** 70%

| 項目 | 状態 | コミット | 詳細 |
|---|---|---|---|
| **0-1. Skia 統合基盤** | `DONE` | `ff2b231` | SkiaPixelBuffer / SkiaRenderer 実装、デフォルト OFF |
| **0-2. ブラシ品質向上** | `DONE` | `98f89c0` | velocity dynamics, texture grain, wet-mix, AA 描画 |
| **0-3. ベクターレイヤー AA** | `PARTIAL` | `4a4beba` | VectorPath float 化完了、レイブプレビュー実装、ラスタライズ実装 |
| **0-4. AA 制御実装** | `PARTIAL` | — | フラグは定義済み、UI 公開未実装 |

### Phase 1 — 標準機能の完全実装

**進捗度:** 30%

#### レイヤーシステム
- ✅ Raster / Vector / Folder 型定義・基本操作
- ✅ Visibility toggle / duplicate / merge down / rasterize
- ✅ Opacity スライダー + undo/redo
- ✅ Layer UI パネル（行表示、drag/drop reorder）
- ❌ フォルダ階層（ネスト）
- ❌ クリッピンググループ合成
- ❌ 複数レイヤー選択
- ❌ ロックモード（位置固定 / 透明保護）

#### 選択範囲
- ✅ 矩形選択 + clear / invert / all / deselect
- ✅ SelectionMask 実装・undo/redo
- ✅ マーチングアンツ表示
- ❌ なげなわ / 多角形 / 自動選択
- ❌ feather / grow / shrink

#### ツール
- ✅ ブラシツール（Raster + Vector）
- ✅ 消しゴム
- ✅ 直線ツール
- ✅ 塗りつぶし（SelectionMask 対応）
- ✅ 移動ツール（Layer Move）
- ✅ AI 選択ツール（SAM2 ONNX 基盤）
- ❌ グラデーション
- ❌ テキストツール
- ❌ 変形ハンドル（rotate / scale / pivot）

#### ペンタブ
- ❌ Wintab / Windows Ink 筆圧取得
- ✅ Mouse-first smoothing フック

#### ファイル I/O
- ✅ Open / Save / Save As
- ✅ Export PNG / JPEG
- ✅ Clipboard copy/paste
- ✅ Import as layer
- ❌ PSD/PSB 対応
- ❌ ネイティブフォーマット

#### カラーシステム
- ✅ HSV カラーホイール
- ✅ HiDPI スウォッチ
- ✅ FG/BG swap / Reset to B/W
- ✅ Transparent color command

#### ドック・ワークスペース
- ✅ Float / Tab / Full-area docking
- ✅ Reset Workspace
- ✅ Workspace save/load/delete
- ✅ Restore last workspace
- ✅ Shortcut settings 実行時設定・保存

#### UI テーマ
- ✅ Dark theme hierarchy（color tokens）
- `WIP` Panel / button / list spacing consistency
- ❌ Icon pack normalization

---

## 完了タスク一覧

### 2026-06 セッション

| Task | 説明 | コミット | 状態 |
|---|---|---|---|
| task-32 | MainWindow Ctrl+0/Ctrl+1 ショートカット確認 | `c056203` | ✅ DONE |
| task-23 | LayerPanel opacity スライダー同期修正 | `4a4beba` | ✅ DONE |
| task-002 | FillTool SelectionMask 対応 | — | ✅ DONE |
| ADR-008 P0-2 | Layer Offset Model (Move/Renderer/Layer) | — | ✅ DONE |
| selection-paint | BrushTool/EraserTool 選択マスク対応 | — | ✅ DONE |
| adj-panel | AdjustmentPropertyPanel 追加 | — | ✅ DONE |
| SAM2 ONNX | AI 選択ツール基盤（PAINT_USE_ONNX） | — | ✅ DONE |
| Vector Live Preview | ベクターレイヤー描画中プレビュー | — | ✅ DONE |
| UI Theme Refresh | Dark theme redesign | — | ✅ DONE |

### 過去セッション（2026-05）

- Phase 0-1: Skia 統合基盤
- Phase 0-2: ブラシ品質向上（velocity, texture, wet-mix）
- VectorPath float 化（全ツール対応）
- アイコン・起動周り整備
- UI 改善（ToolPropertyPanel, ColorSwatchWidget, DockTitleBar）

---

## アーキテクチャ決定済み（未実装）

### ADR-008: Layer Offset Model — ACCEPTED (2026-06-10)

CSP/Photoshop 互換の「レイヤーオフセット + 独立 PixelBuffer」モデルへの移行が決定。
詳細: [ADR-008](../docs/adr/ADR-008-layer-offset-model.md)

移行フェーズ（実装順）:

| Phase | 内容 | リスク | 状態 |
|---|---|---|---|
| **Phase 0** | `Layer` に `offsetX/offsetY` 追加（デフォルト 0） | 低 | ✅ 完了 |
| **Phase 1** | `Renderer` 全ピクセルアクセスに座標変換追加 | 中 | ✅ 完了 |
| **Phase 2** | `MoveLayerTool` をオフセット変更に置換。ペーストを元サイズ保持に | 中 | ✅ 完了 |
| **Phase 3** | ブラシのバッファ動的拡張（全描画ツール） | **高** | ⬜ 未着手 |
| **Phase 4** | LPA v2 フォーマット更新 / PSD 完全対応 | 低 | ⬜ 未着手 |

**Phase 3 はオフセット ≠ 0 のレイヤーに描画する場合に必要。現在はオフセット 0 で描画→移動の順序で利用可能。**

---

## 次のタスク（優先順）

### 🔴 高優先度

1. **ADR-008 Phase 3** — オフセットレイヤーへの描画（BrushTool buffer動的拡張）
   - 現在はオフセット 0 のレイヤーで描画→移動の順序が前提
   - Phase 3 実装で「移動済みレイヤーに直接描画」が可能になる
   - **HIGH RISK**: 全描画ツールに影響。ユニットテスト整備が前提条件

2. **SAM2 ONNX モデルセットアップ** （AI 選択ツール実用化）
   - `scripts\download_sam2.ps1` で モデルをエクスポート
   - `vcpkg install onnxruntime:x64-windows`
   - `cmake -DPAINT_USE_ONNX=ON` でリビルド
   - `build\Release\models\` に自動配置

2. **ベクター編集ツール実装** （Vector 層を完全化）
   - Point edit （選択・移動・削除）
   - Path split / connect / simplify
   - テーパー形状適用
   - 推定工数: 20 時間

3. **フォルダレイヤー階層** （UI 標準機能化）
   - `Layer::parent` ポインタ追加
   - LayerPanel 再帰表示
   - ドラッグ drop-into-folder 対応
   - 推定工数: 10 時間

### 🟡 中優先度

4. **ペンタブ筆圧統合** （入力品質向上）
   - Wintab / Windows Ink API 統合
   - `QTabletEvent` 処理
   - ToolPropertyPanel に筆圧プレビュー表示
   - 推定工数: 15 時間

5. **Skia 本移植** （GPU レンダリング）
   - `PAINT_USE_SKIA=ON` を default
   - Vulkan / Metal バックエンド動作確認
   - CPU fallback 検証
   - 推定工数: 30 時間

6. **クリッピンググループ合成** （標準機能化）
   - Renderer での処理
   - UI：layer clipping toggle
   - 推定工数: 8 時間

### 🟢 低優先度（Phase 1+）

7. グラデーション・テキストツール
8. 調整レイヤー（トーンカーブ・色相彩度）
9. PSD/PSB インポート
10. MV 制作ソフト連携（Phase 2）

---

## ビルド・実行

### クイックコマンド

```powershell
# ビルド + 起動
.\launch.bat

# ビルドのみ
cmake --build build --config Release

# テスト
.\scripts\build-tests.ps1
ctest --test-dir build-tests --output-on-failure
```

### 環境

- **Qt:** `C:\Qt\6.7.2\msvc2019_64`
- **CMake:** 4.3.1+
- **MSVC:** Visual Studio 2022 Community
- **Skia:** vcpkg (optional, `PAINT_USE_SKIA=ON`)
- **ONNX:** vcpkg (optional, `PAINT_USE_ONNX=ON`)

---

## ドキュメント構成

| ファイル | 用途 |
|---|---|
| `docs/SPEC.md` | 製品ビジョン・実装方針・フェーズ構成 |
| `docs/SPEC2.md` | 実装仕様・アーキテクチャ・データモデル |
| `docs/build.md` | ビルド手順・環境セットアップ |
| `docs/roadmap.md` | フェーズ別ロードマップ |
| `.claude/PROJECT_STATUS.md` | 本ファイル（状態スナップショット） |
| `.claude/DEV_LOG.md` | 作業履歴 |
| `.claude/DECISIONS.md` | 技術判断記録 |
| `.claude/FAILED_ATTEMPTS.md` | 失敗した試行錯誤 |
| `docs/adr/` | 技術決定録（ADR-xxx） |

---

## メモ・制約

### 重要な実装パターン

1. **ToolDescriptor は JSON で読まない** — UI 定義は code-driven（`ToolCatalog::load()`）
2. **core は Qt フリー** — `src/core/` に `#include <Qt...>` を持ち込まない
3. **Platform 層経由** — OSS ラッパー（Skia/ONNX）は `src/platform/` 以下に限定
4. **history は snapshot-based** — before/after コピーで安全性確保

### コンパイル安定性

- **PAINT_BUILD_TESTS=ON** → GoogleTest include（CTest は CMake 4.3.1 で stable）
- **PAINT_USE_SKIA=ON** → vcpkg SkiaB uild_TYPE= Release（Debug は巨大）
- **PAINT_USE_ONNX=ON** → `<exe>/models/` に `.onnx` ファイル必須

---

## 問い合わせ・引き継ぎ

新規セッション開始時は以下の順で確認：
1. このファイル（PROJECT_STATUS.md）で現状把握
2. `docs/SPEC.md` でビジョン確認
3. 次タスク一覧から優先度判断
4. `git log --oneline -10` で直近コミット確認
5. タスク開始

