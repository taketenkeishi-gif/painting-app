# 開発ログ — Paint App

セッションごとの作業内容・完了タスク・次のアクションを記録。

---

## 2026-06-11 (最新) — Layer Offset Model 実装 (ADR-008 Phase 0-2)

### 作業内容

- Ctrl ドラッグ移動でキャンバス外ピクセルが失われる問題を修正
- ADR-008 Phase 0-2 を実装（Layer offset fields / Renderer 変換 / MoveLayerTool 非破壊化）
- BrushTool / EraserTool / FillTool 選択マスク対応
- AdjustmentPropertyPanel 追加（レイヤーパネルにタブ追加）
- FreeTransformTool デバッグログ削除

### 変更ファイル

- `src/core/layer/Layer.h` — `m_offsetX/Y` フィールド + accessor 追加（Phase 0）
- `src/core/render/Renderer.cpp` — `sampleBuffer()` + オフセット座標変換（Phase 1）
- `src/core/tools/MoveLayerTool.h/cpp` — ピクセルコピー→オフセット更新に置換（Phase 2）
- `src/core/document/Document.h/cpp` — `insertLayerAt()` 追加（LayerAdd/Remove undo用）
- `src/app/bridge/LpaExporter/Importer.cpp` — `offsetX/Y` 保存・読み込み（既存）
- `src/core/tools/BrushTool.h/cpp` — `m_selectionMask` で選択範囲内のみ描画
- `src/core/tools/EraserTool.h/cpp` — 同上（消しゴム）
- `src/core/tools/FillTool.cpp` — 同上（塗りつぶし）
- `src/app/panels/AdjustmentPropertyPanel.h/cpp` — 新規パネル
- `src/app/bridge/AppController.h` — `setActiveLayerAdjustmentParams` / `pasteBufferAsNewRasterLayerAndTransform` 追加
- `src/app/mainwindow/MainWindow.h/cpp` — AdjustmentDock 配線
- `src/CMakeLists.txt` — AdjustmentPropertyPanel をビルドに追加
- `docs/adr/ADR-008-layer-offset-model.md` — 新規 ADR 文書
- `src/app/main.cpp` — 起動ログ（[STARTUP] Build/Exe 情報）

### 完了項目

- ✅ 実装: Layer Offset Model Phase 0, 1, 2
- ✅ 実装: 選択マスク対応（Brush/Eraser/Fill）
- ✅ 実装: AdjustmentPropertyPanel
- ✅ ビルド: Release ビルド成功（コンパイルエラーなし）
- ✅ デバッグログ削除: FreeTransformTool の `[FT_PRESS]` `[FT_MOVE]` `[OVERLAY]` ログ除去

### 既知の未対応事項

- ADR-008 Phase 3 (BrushTool buffer 動的拡張): オフセット != 0 レイヤーへの直接描画は未対応
  → 回避策: 描画後に移動する順序で利用可能

### ユーザー確認ポイント

1. ラスターレイヤーを Ctrl ドラッグで半分キャンバス外へ移動 → 逆方向に戻す → 消えたピクセルが復活することを確認
2. 選択範囲を作成してブラシで描画 → 選択外に描かれないことを確認
3. 調整レイヤーを選択して「調整レイヤー」ドックが表示・動作することを確認

---

## 2026-06-11 — FreeTransform コミット修正・完全検証

### 作業内容

- ラスターレイヤーの FreeTransform コミット不具合を調査・修正
- 自動化テストスイート（verify_final.py）で 11/11 PASS を確認

### 発見した不具合

`beginTransformSession()` のラスターレイヤーパスで `m_transformSession = std::move(session)` が
抜けていた。`commitTransformSession()` は `m_transformSession.has_value()` をガードしているため、
ラスターレイヤーの FT コミットは常に早期 return（サイレント no-op）になっていた。

### 修正内容

- `src/app/bridge/AppController.cpp`: `m_transformSession = std::move(session)` 追加、重複代入除去
- `src/app/canvasview/CanvasWidget.cpp`: `[KPE]` 診断ログ追加（keyPressEvent 先頭）
- コミット: `b22225a fix: assign m_transformSession in raster layer transform path`

### 検証結果（11/11 PASS）

| ID | テスト | 結果 |
|---|---|---|
| 1-A | Paste 1600x1200 → offset=-400,-300 | PASS |
| 1-B | MoveLayer drag (M) | PASS |
| 1-C | FreeTransform commit | **PASS** ← 修正で解決 |
| 1-D | Undo | **PASS** ← 修正で解決 |
| 2-A | Save .lpa | PASS |
| 2-B | Restart | PASS |
| 2-C | Load .lpa → layer exists | PASS |
| 3-A | Brush stroke | PASS |
| 3-B | Eraser stroke | PASS |
| 3-C | Fill click | PASS |
| 3-D | Brush after FT commit | **PASS** ← 修正で解決 |

### Runtime Verification

```
BUILD:   build\src\Release\LayeredPaintApp.exe  2026/06/11 09:17:06
RUNTIME: build\src\Release\LayeredPaintApp.exe  PID=47420
MATCH:   YES
```

### 次のアクション

- [ ] ベクター編集: ストローク選択・移動・削除（SPEC TODO 優先度 1）
- [ ] フォルダレイヤーのネスト構造

---

## 2026-06-08

### セッション概要

**型式:** ドキュメント管理の標準化  
**対象:** docs/ と .claude/ の構造整理

### 作業内容

- [ ] docs/SPEC.md を確認・SPEC2.md との役割分け整理
- [ ] architecture.md の内容を SPEC2.md に統合
- [x] PROJECT_STATUS.md 新規作成（実装進捗管理）
- [x] DEV_LOG.md 新規作成（本ファイル）
- [x] DECISIONS.md 新規作成（技術判断記録）
- [x] FAILED_ATTEMPTS.md 作成スタブ
- [ ] docs/adr/ フォルダ作成 + ADR テンプレート

### 完了項目

- ✅ PROJECT_STATUS.md を MV-studio-app 形式で作成
- ✅ 実装進捗を Phase 0 / Phase 1 に分類
- ✅ タスク優先度リストを整理

### 次のアクション

- [ ] docs/adr/ + ADR-001 を作成（技術判断の履歴化）
- [ ] 古い管理ファイル（SESSION_LOG.md など）をアーカイブ
- [ ] ドキュメント統合の完了確認

---

## 2026-05-XX (過去セッション例)

### セッション概要

**型式:** Feature / Phase 0-2 ブラシ品質向上  
**対象:** src/core/brush/ / src/core/tools/

### 作業内容

- Velocity dynamics 実装（速度感応で size/opacity 変更）
- Texture grain 追加（セルノイズで決定論的グレイン）
- Wet-mix / smear 実装
- AA 描画（gaussianFalloff の代替）

### 完了項目

- ✅ BrushTool に velocity パラメータ追加
- ✅ テクスチャグレイン実装（core/math/cellnoise.cpp）
- ✅ Smear アルゴリズム
- ✅ stampCircleAA / drawSegmentAA 実装
- ✅ テスト通過

### 次のアクション

- [ ] UI exposure（ToolPropertyPanel に velocity toggle 追加）
- [ ] Skia 統合（Phase 0-1）

---

## 記録フォーマット

### セッション開始時

```markdown
## YYYY-MM-DD HH:MM 〜 HH:MM (所要時間)

### セッション概要

**型式:** [Feature / Fix / Debug / Refactor / Doc]  
**対象:** [モジュール / ファイル]  
**コミット:** [短縮ハッシュ]

### 作業内容

- タスク 1
- タスク 2

### 完了項目

- ✅ 実装: ...
- ✅ テスト: ...
- ✅ ドキュメント: ...

### 次のアクション

- [ ] TODO 1
- [ ] TODO 2

### 参考

- [参照ドキュメント](docs/SPEC.md)
- 関連コミット: `abc1234`
```

---

## 2026-06-10 — アーキテクチャレビュー & ADR-008 策定

### セッション概要

**型式:** Architecture Review / Doc  
**対象:** レイヤーモデル設計 調査・意思決定  
**コミット:** — （ドキュメントのみ）

### 作業内容

1. **本番準備度レビュー（Production Readiness Audit）**
   - イラストレーター / CSP・Krita ユーザー視点でコードベースを網羅調査
   - ツール登録・UI接続・Undo/Redo・保存読込・ブラシエンジン・選択ツール・テキスト・調整レイヤーを検証
   - 主要UXブロッカーを特定（選択マスク未適用、レイヤー操作Undo破壊、調整レイヤーUIなし）

2. **Canvas-Bound Layer Model の制約調査**
   - `Layer.h`, `PixelBuffer.h`, `Document.cpp`, `Renderer.cpp`, `BrushTool.cpp`, `MoveLayerTool.cpp`, `CanvasWidget.cpp` を全読
   - 全コンポーネントにわたるキャンバスサイズ固定・原点整列前提を確認
   - `PsdExporter.cpp` のコンパイル不可バグ（`layer.offsetX()` 未定義呼び出し）を発見

3. **3案の比較評価**
   - Case A（Temporary Transform のみ）: 目標 UX 不可能 → 不採用
   - Case B（Layer Offset + Independent PixelBuffer）: 業界標準 → 採用
   - Case C（Full Transform Matrix）: 過剰・ブラシと相性悪い → 不採用

4. **ドキュメント作成**
   - `docs/adr/ADR-008-layer-offset-model.md` 新規作成
   - `docs/adr/README.md` に ADR-008 追加
   - `docs/architecture.md` にレイヤーモデルの現状制約と移行計画を追記
   - `.claude/DECISIONS.md` に ADR-008 の判断記録を追記

### 完了項目

- ✅ 本番準備度レビュー完了（主要UXブロッカー特定）
- ✅ レイヤーアーキテクチャ調査完了（全コンポーネントの canvas-bound 前提確認）
- ✅ ADR-008 策定（3案比較・Case B 採用決定）
- ✅ ドキュメント更新（ADR, architecture.md, DECISIONS.md, DEV_LOG.md）

### 判断サマリー

**ADR-008 ACCEPTED:** Layer Offset + Independent PixelBuffer モデルを採用。
`offsetX/offsetY = 0` デフォルトにより、既存動作を壊さず 4 フェーズで段階移行。
Phase 3（ブラシ動的バッファ拡張）が最高リスク。実装開始前に Phase 0〜1 のテストを十分実施すること。

### 次のアクション

- [ ] ADR-008 Phase 0 実装（`Layer` に `offsetX/offsetY` フィールド追加）
  - `PsdExporter.cpp` のコンパイルエラーが同時解消
  - 既存テスト全パスを確認
- [ ] 選択マスクをブラシに適用（S-tier UXブロッカー、ADR-008 と独立して実施可能）
- [ ] レイヤー操作の Undo 対応（S-tier UXブロッカー、ADR-008 と独立）

---

## インデックス

| 日付 | セッション | 状態 | コミット |
|---|---|---|---|
| 2026-06-10 | アーキテクチャレビュー & ADR-008 策定 | DONE | — |
| 2026-06-08 | ドキュメント管理標準化 | 進行中 | — |
| 2026-05-xx | Phase 0-2 ブラシ品質 | DONE | `98f89c0` |
| 2026-05-xx | Skia 統合基盤 | DONE | `ff2b231` |

