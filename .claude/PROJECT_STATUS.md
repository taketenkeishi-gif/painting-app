# プロジェクト状態 — Paint App

**Last Updated:** 2026-06-15  
**Latest Commit:** `6f37557` (feat(dev-bridge): Layer UI coverage追加 — LP-02〜LP-06 DEV_BRIDGE_VERIFIED)  
**Current Phase:** UI Restore（過去実装済み機能の復元フェーズ）

> **UI Restore フェーズについて:**  
> `golden-core-main-2026-06-15` タグ時点での regression 調査完了。  
> 過去commitで実装済みだった機能がa937467等の「UI密度復元」セッションで削除されていることを確認。  
> 復元対象リスト: [`.claude/UI_RESTORE_TODO.md`](UI_RESTORE_TODO.md)  
> 禁止: 新規設計 / CSP参考再設計 / リファクタ
>
> **中間地点タグ:** `ui-restore-verified-layer-2026-06-15` (6f37557)  
> LayerPanel LP-02〜LP-06: DEV_BRIDGE_VERIFIED。CW-01: RESTORED。

---

## UI Restore 進捗 (2026-06-15)

| セクション | 状態 | 詳細 |
|---|---|---|
| **LP-01** フォルダ展開/折りたたみ | `ALREADY_PRESENT` | ソース確認済み |
| **LP-02** レイヤー名インライン編集 | `DEV_BRIDGE_VERIFIED` | rename-layer action で確認 |
| **LP-03** マスクサムネイル | `DEV_BRIDGE_VERIFIED` | add-layer-mask action で確認 |
| **LP-04** イメージ/マスク編集ターゲット | `DEV_BRIDGE_VERIFIED` | set-edit-target action で確認 |
| **LP-05** 複数レイヤー選択 | `DEV_BRIDGE_VERIFIED` | select-layers action で確認 |
| **LP-06** checkableロックボタン | `DEV_BRIDGE_VERIFIED` | set-layer-lock + components checked で確認 |
| **LP-07** ブレンドモード全種 | `ALREADY_PRESENT` | ソース確認済み |
| **MW-01〜MW-04** DockTitleBar | `ALREADY_PRESENT` | ソース確認済み |
| **CW-01** ズーム補間 | `RESTORED` | commit 971e66a。目視確認待ち |
| **CW-02〜CW-05** Canvas機能 | `ALREADY_PRESENT` | ソース確認済み |
| **TU-01〜TU-04** Tool UI | `ALREADY_PRESENT` | ソース確認済み |
| **SC-01** shortcut conflict | `VERIFIED` | Dev Bridge conflictCount=0 |
| **SC-02** QuickMask Alt+Q | `ALREADY_PRESENT` | ソース確認済み |

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
- ✅ **PSD エクスポート**（ファイルメニュー「PSDとして書き出し」接続済み・Ctrl+Shift+D）
- ❌ PSD インポート
- ❌ ネイティブフォーマット

#### AI 機能
- ✅ **ComfyUI AI 生成統合**（inpaint / txt2img E2E 完了。`ComfyProcessManager` async 設計）
- ✅ **AI アップスケール**（ComfyUI + ONNX + バイリニア 3段階）
- ✅ **AI モデルフォルダ管理 UI**（QSettings 永続保存）
- ✅ **LoRA / Checkpoint プリセット**（WorkflowPreset + UI 接続）
- 🔧 **Roto Brush / SAM2 基盤**（`PAINT_USE_ONNX=ON` 時有効、モデルセットアップ待ち）
- 🔧 **Mesh Deformation（Puppet Warp）**（ワイヤーフレーム表示・変形実装済み）
- ✅ FreeTransform ツール（高品質変換・コーナーアンカー修正済み）

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
| **FreeTransform 修正** | 高品質変換・コーナーアンカー修正 | `22d3fc2` | ✅ DONE |
| **Mesh Deformation** | Puppet Warp（ワイヤーフレーム・変形） | `8b42467`, `f3cf765` | ✅ DONE |
| **Roto Brush** | object_select → AiSelectTool 接続 | `0cc1a45` | 🔧 ONNX モデル待ち |
| **AI アップスケール** | ComfyUI + ONNX + バイリニア 3段階 | — | ✅ DONE |
| **AI モデルフォルダ管理** | AiModelFolderDialog（QSettings） | — | ✅ DONE |
| **LoRA/Checkpoint プリセット** | WorkflowPreset + UI 接続 | `0795b4b` | ✅ DONE |
| **ComfyUI AI 生成 E2E** | inpaint（選択あり）/ txt2img（選択なし）verified | — | ✅ DONE |
| **ComfyProcessManager async化** | waitForConnected 完全廃止、async socket 設計 | — | ✅ DONE |
| **AppController M1/M2/M3 修正** | ensureComfyProcessManager 統一・debug path 固定 | — | ✅ DONE |
| **UX Fix Pass 1** | shortcut conflict 3→0・ToolSlider 12px→48px・AdjustmentDock minHeight 22px→422px | `c0ae0a9` | ✅ DONE |
| **Dev Bridge Runtime Observation** | /debug/components・/debug/layout・/debug/input 追加 | — | ✅ DONE |
| **Action Surface Expansion** | new-document・add-raster-layer・set-active-layer・set-foreground-color・export-png | — | ✅ DONE |
| **FirstDrawingSession v2** | automationRate 42% → 83%（12ステップ中10 PASS / 2 NOT_REACHABLE） | — | ✅ DONE |
| **UI Restore: CW-01** | ズーム適応スムース補間復元 | `971e66a` | ✅ DONE |
| **UI Restore: LayerPanel Coverage** | rename-layer/add-layer-mask/set-layer-lock/set-edit-target/select-layers actions + per-layer state fields | `6f37557` | ✅ DONE |

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

### 🔴 即着手可能（依存なし）

1. ~~**PSD 出力 → ファイルメニュー接続**~~ ✅ 完了
2. ~~**ComfyUI AI 生成統合**~~ ✅ 完了（inpaint / txt2img E2E verified）

3. **選択範囲を新規レイヤーとして切り出し** （〜2時間）
   - `AppController::extractSelectionToNewLayer()` を追加
   - 切り出し後、元レイヤーの選択領域を透明化
   - MV ワークフロー W-041/042 に対応

### 🔴 高優先度（依存あり）

4. **SAM2 ONNX モデルセットアップ** （AI 選択ツール実用化）
   - `cmake -DPAINT_USE_ONNX=ON` でリビルド
   - sam2_hiera_t.onnx を `build\Release\models\` に配置
   - Roto Brush の動作確認

5. **ADR-008 Phase 3** — オフセットレイヤーへの描画（BrushTool buffer動的拡張）
   - 現在はオフセット 0 のレイヤーで描画→移動の順序が前提
   - Phase 3 実装で「移動済みレイヤーに直接描画」が可能になる
   - **HIGH RISK**: 全描画ツールに影響。ユニットテスト整備が前提条件

6. **ベクター編集ツール実装** （Vector 層を完全化）
   - Point edit （選択・移動・削除）
   - Path split / connect / simplify
   - テーパー形状適用
   - 推定工数: 20 時間

7. **フォルダレイヤー階層** （UI 標準機能化）
   - `Layer::parent` ポインタ追加
   - LayerPanel 再帰表示
   - ドラッグ drop-into-folder 対応
   - 推定工数: 10 時間

### 🟡 中優先度

8. **ペンタブ筆圧統合** （入力品質向上）
   - Wintab / Windows Ink API 統合
   - `QTabletEvent` 処理
   - ToolPropertyPanel に筆圧プレビュー表示
   - 推定工数: 15 時間

9. **Skia 本移植** （GPU レンダリング）
   - `PAINT_USE_SKIA=ON` を default
   - Vulkan / Metal バックエンド動作確認
   - CPU fallback 検証
   - 推定工数: 30 時間

10. **クリッピンググループ合成** （標準機能化）
    - Renderer での処理
    - UI：layer clipping toggle
    - 推定工数: 8 時間

### 🟢 低優先度（Phase 1+）

11. グラデーション・テキストツール
12. 調整レイヤー（トーンカーブ・色相彩度）
13. アンシャープマスクフィルター（MV ワークフロー W-030）
14. LaMa インペイントエンジン（MV ワークフロー W-050）
15. PSD インポート

---

## MV 連携ワークフロー進捗（別ドキュメント参照）

詳細: [docs/MV_EXPORT_WORKFLOW.md](../docs/MV_EXPORT_WORKFLOW.md)

| ステップ | 完了度 | 残作業 |
|----------|--------|--------|
| 1. 画像インポート | 80% | キャンバスリサイズオプション |
| 2. 背景/キャラ分離 | 30% | SAM2 モデルセットアップ・自動分割 |
| **3. AIアップスケール / AI生成** | **100%** | ✅ inpaint/txt2img E2E 完了 |
| 4. エッジ強調 | 0% | 全タスク未着手 |
| 5. パーツ分離 | 10% | SAM2 依存・切り出しコマンド未実装 |
| 6. カモフラージュ | 0% | LaMa モデル選定が先 |
| 7. 手動調整 | 90% | プレビューのみ残 |
| **8. PSD出力** | **100%** | ✅ 完了 |

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

## Architecture Cleanup Backlog

Architecture Audit (2026-06-14) で記録された技術的負債。
機能開発の合間に段階的に解消する。詳細は `docs/architecture.md` の Known Architecture Debt を参照。

| 項目 | 優先度 | 状態 | 内容 |
|---|---|---|---|
| Remove unused AiGenerateDialog | 🟡 Medium | ⬜ 未着手 | `AiGenerateDialog.h/cpp`（約464行）がデッドコード。インスタンス化なし |
| Remove legacy ComfyUiClient workflow responsibilities | 🔴 High | ⬜ 未着手 | `ComfyUiClient` + `MinimalWebSocket` を除去し ComfyUI 経路を `AiService → ComfyProvider → ComfyClient` に一本化 |
| Move Qt-dependent core files | 🔴 High | ⬜ 未着手 | `core/ai/GenerativeFillEngine` の `<QObject>/<QTimer>/<QThread>` 依存を `app/` 側に移動 |
| Fix platform/ direct includes in panels | 🔴 High | ⬜ 未着手 | `UpscaleDialog`・`GenerativeFillDialog`・`MainWindow` が `platform/qt/QtImageConverter.h` を直接 include。bridge 経由に変更 |
| Split AppController | 🟢 Low | ⬜ 未着手 | 約7,400行・311 public メソッドを LayerService / SelectionService 等に段階分割。最大リスク。スコープ明示後に着手 |

**注意:** このバックログは即着手しない。上位の機能タスクを優先し、並行可能な場合のみ消化する。

---

## 問い合わせ・引き継ぎ

新規セッション開始時は以下の順で確認：
1. このファイル（PROJECT_STATUS.md）で現状把握
2. `docs/SPEC.md` でビジョン確認
3. 次タスク一覧から優先度判断
4. `git log --oneline -10` で直近コミット確認
5. タスク開始

