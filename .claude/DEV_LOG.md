# 開発ログ — Paint App

セッションごとの作業内容・完了タスク・次のアクションを記録。

---

## 2026-06-15 (最新) — 本流確定・UI Regression監査完了

### 作業内容

1. **全git履歴監査** — 全branch/tag/reflogからHEAD未包含commitを抽出・分類
2. **統合可能性検証** — integration-testブランチでorigin/master + mainの--allow-unrelated-historiesを試行 → 65件 add/add コンフリクト確認 → aborted
3. **機能差分比較（全カテゴリ）** — Canvas / Layer / Tool / AI の read-only 比較完了
4. **本流確定** — main を唯一の製品本流として確定（ADR-020）
5. **golden タグ作成** — `golden-core-main-2026-06-15`（AI/Brush/Layer/Canvas機能統合済み基盤）
6. **UI Regression監査** — LayerPanel/menu/shortcut/canvas/toolpanel 全項目静的解析 → 問題なし

### 完了項目

- ✅ git audit: 43件のorigin/master未包含commit確認
- ✅ integration test: unrelated histories確認 → merge戦略廃棄
- ✅ 機能比較: origin/masterがmainより優れる機能=ゼロ 確認
- ✅ tag: golden-core-main-2026-06-15 作成
- ✅ DECISIONS.md: ADR-020 (本流確定・merge禁止) 追記
- ✅ UI Regression監査: 全5項目問題なし

### 結論

origin/master はレガシー参照専用。今後の比較基準は golden-core-main-2026-06-15 タグ。

### 次のアクション

- [ ] UI regressionをランタイム目視確認（アプリ起動して確認）
- [ ] ベクター編集: ストローク選択・移動・削除（優先 TODO #1）
- [ ] フォルダレイヤーネスト構造

---

## 2026-06-15 (前回) — Dev Bridge First Architecture 統合完了

### 作業内容

UX Fix Pass 1（shortcut / ToolSlider / AdjustmentDock）、Dev Bridge Runtime Observation 追加、
Action Surface 拡張（5 アクション）、FirstDrawingSession シナリオ v2 の実行・検証。

### 変更ファイル

- `src/app/bridge/AppController.cpp` — 5 新規 debug アクション追加（`#ifdef PAINT_DEBUG_SERVER` 内）:
  - `new-document` {width, height} — `newDocument()` 経由でキャンバス再作成
  - `add-raster-layer` — `addRasterLayer()` 経由でレイヤー追加
  - `set-active-layer` {index} — `setActiveLayer()` 経由でアクティブレイヤー切替
  - `set-foreground-color` {r,g,b} — `setBrushColor()` 経由で前景色変更
  - `export-png` {path} — `rerender()` + `QtImageConverter::toQImage()` + `QImage::save()` で PNG 書き出し
- `C:\Users\Keishi\Portfolio\Dev_Bridge\src\projects\LayeredPaint\scenarios\FirstDrawingSession.mjs` — v2 に書き換え（5 新アクション利用）

### UX Fix Pass 1（コミット c0ae0a9）

| 修正項目 | Before | After |
|---|---|---|
| shortcut conflict | 3 件（Del キー競合等） | 0 件 |
| ToolSlider 幅 | 12px | 48px |
| AdjustmentDock minHeight | 22px | 422px |

### Dev Bridge Runtime Observation

DebugServer に 3 エンドポイント追加（`PAINT_DEBUG_SERVER` 有効時）:
- `GET /debug/components` — UI ウィジェット一覧（type / text / visible / enabled / bounds）
- `GET /debug/layout` — ドック配置情報（area / floating / geometry / tabbedWith）
- `GET /debug/input` — ショートカット一覧・conflict 検出（conflictCount）

### FirstDrawingSession v2 実行結果

- 12 ステップ中 10 PASS / 0 FAIL / 2 NOT_REACHABLE
- automationRate: **83%**（前回 42%）
- NOT_REACHABLE 内訳:
  - step 2: `set-tool` action 未実装（Brush ツール既定アクティブなので click コスト=0）
  - step 8c: AI Inpaint（ComfyUI 接続 + プリセット設定が必要。`ai-generate-with-preset` は実装済み）

### Architecture Audit 結果

新規 5 アクション全件 PASS（重複ロジックなし、core 漏れなし）。
WARNING 1 件: `export-png` は `markClean()` を呼ばない（debug 用途で意図的。`#ifdef` 保護済み）。

### 完了項目

- ✅ UX Fix Pass 1（shortcut conflict 解消・スライダー幅・ドック高さ）
- ✅ Runtime Observation 3 エンドポイント（/debug/components / /debug/layout / /debug/input）
- ✅ Action Surface 5 件（new-document / add-raster-layer / set-active-layer / set-foreground-color / export-png）
- ✅ FirstDrawingSession v2: 10/12 PASS, automationRate 83%
- ✅ Architecture Audit: delegate-only パターン確認、重複実装なし

### 次のアクション

- [ ] `set-tool` action 実装（automationRate 100% への残り 1 ステップ）
- [ ] 機能開発継続（Vector 編集 / 選択範囲切り出し等）

---

## 2026-06-15 — ComfyUI AI 生成統合 E2E 完了

### 作業内容

ComfyUI との AI 生成経路（inpaint / txt2img）を完全非同期化し、E2E を verified。

### 変更ファイル

- `src/platform/comfy/ComfyProcessManager.cpp` — `waitForConnected` 完全廃止。`ensureRunning` + `onHealthCheckTick` を `connectToHost + connected/errorOccurred` シグナルによる完全非同期設計に書き換え
- `src/app/bridge/AppController.h` — `ensureComfyProcessManager()` private メソッド追加
- `src/app/bridge/AppController.cpp` — M1/M2/M3 修正:
  - M1: `ensureComfyRunning` から `waitForConnected(300)` 高速パスを削除。常に async 経路に統一
  - M2: `m_comfyProcess` 初期化 + 永続シグナル接続を `ensureComfyProcessManager()` に一本化（`failed → aiGenerationError` の重複接続リスクを排除）
  - M3: `debugCaptureComfyPayload/Response` の出力先を `QStandardPaths::AppLocalDataLocation` に固定（CWD 依存解消）

### 完了項目

- ✅ `waitForConnected` 残存 0（grep 確認）
- ✅ inpaint E2E: `selection-rect` → `ai-generate-with-preset` → `controllerInstance=1`, `activeHasMask=True`, `layers.count=2`
- ✅ `debug_comfy_prompt.json`: `C:\Users\Keishi\AppData\Local\LayeredPaintApp\` に出力（固定パス）、ノード数 10、`node_errors` なし
- ✅ txt2img regression: `selection-clear` 後 generate → `activeHasMask=False` 確認
- ✅ `failed → aiGenerationError` emit: `ensureComfyProcessManager()` の永続接続1本のみ

### 根本原因（修正済み）

`QTcpSocket::waitForConnected` は Qt ドキュメントで「Windows メインスレッドで動作保証なし」と明記されている。
HTTP ハンドラコンテキスト（DebugServer）と QTimer スロット（healthCheckTick）の両方から呼ばれていたため、
ComfyUI が起動済みでも `ready` シグナルが発火せず AI 生成が開始されなかった。

### 次のアクション

- [ ] 機能開発へ移行（次は「選択範囲を新規レイヤーとして切り出し」または Vector 編集）
- [ ] Architecture Cleanup Backlog（D 系: WebSocket 経路統合等）は優先度を落として並行消化

---

## 2026-06-12 (前セッション) — FreeTransform UX 修正（画質・コーナーアンカー）

### 作業内容

- Enter確定時の画質劣化（ガビガビ）を修正
- コーナードラッグで対角コーナーを厳密固定する CSP 互換の挙動を実装

### 変更ファイル

- `src/app/bridge/AppController.h` — `m_transformInterpolation` を `Bicubic` → `Bilinear` に変更
- `src/core/tools/FreeTransformTool.cpp` — コーナーハンドルの中心補正をアンカー逆算式に変更

### 修正詳細

**画質問題:**  
コミット時の `Bicubic` カスタムカーネルとプレビュー（`QPainter::SmoothPixmapTransform`）の描画アルゴリズム差異が原因。  
`Bilinear`（Qt `QImage::transformed()` + `Qt::SmoothTransformation`）に統一してプレビューと一致させた。

**コーナー移動問題:**  
縦横比固定時に `m_sx/m_sy` が補正されると `(cx + anchor) / 2` の中点式が崩れ、対角アンカーがずれていた。  
アンカー逆算式 `center = anchor + R*(sxS*sx*halfW, syS*sy*halfH)` に変更し、sx/sy 変更後も対角コーナーを厳密固定。

### 完了項目

- ✅ Enter確定後の画質がプレビューと一致
- ✅ コーナードラッグで対角コーナー固定（CSP互換）
- ✅ Shiftフリースケールも同式で一貫動作
- ✅ ユーザー確認済み（CSP公式挙動に近い）

### 次のアクション

- [ ] ベクター編集: ストローク選択・移動・削除（SPEC TODO 優先度 1）
- [ ] フォルダレイヤーのネスト構造

---

## 2026-06-11 (前セッション) — Layer Offset Model 実装 (ADR-008 Phase 0-2)

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

