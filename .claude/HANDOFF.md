# 引き継ぎタスク

全体ビジョン・技術選定は `docs/SPEC.md` を参照。

---

## 環境セットアップ（新環境向け）

```powershell
# Qt 6.7.2 インストール（初回のみ）
pip install aqtinstall
python -m aqt install-qt windows desktop 6.7.2 win64_msvc2019_64 -O "C:\Qt"

# CMake configure（初回のみ）
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.7.2\msvc2019_64" -DPAINT_BUILD_TESTS=OFF

# ビルド＆起動
.\launch.bat          # ビルド → 起動
# または
cmake --build build --config Release
```

Qt: `C:\Qt\6.7.2\msvc2019_64`  
MSVC: Visual Studio 2022 Community (v14.44)

---

## 完了済み

### Phase 0-1: Skia バックエンド整備（commit ff2b231）
- `src/platform/skia/` — SkiaPixelBuffer / SkiaRenderer / SkiaIntegration
- `PAINT_USE_SKIA=OFF` デフォルト、ON 時は vcpkg 必要

### Phase 0-2: ブラシ品質向上（commit 98f89c0）
- 速度感応ダイナミクス（velocity → size/opacity）
- テクスチャグレイン（決定論的セルノイズ）
- ウェットミックス / スメア
- AA 描画（stampCircleAA, drawSegmentAA）

### UI テーマ全面刷新
- デザイントークンベースのモダンダークテーマ
- Base: `#13151c` / Surface: `#1a1d27` / Accent: `#4e8ef7`

### VectorPath float 化
- 全ツールの VectorPath::points を FPoint に移行

### SAM2 ONNX AI選択ツール基盤（2026-06）
- `src/core/ai/OnnxSegEngine.h/.cpp` — SAM2 ONNX ラッパー（Qt フリー PIMPL）
- `PAINT_USE_ONNX=OFF` 時はスタブ（既存ビルド互換）、ON 時に実推論
- `AiSelectTool::Settings::granularity` (0〜3) — 細部→被写体全体を制御
- `AppController::initOnnxEngine` — `<exe>/models/*.onnx` を自動検出
- `AppController::setupOnnxInferenceCallback` — ONNX > ComfyUI > スタブの優先順
- `ToolPropertyPanel` に「AI 選択粒度」コンボボックスを追加
- セットアップ手順: `scripts\download_sam2.ps1` → vcpkg onnxruntime → cmake -DPAINT_USE_ONNX=ON

### task-002: FillTool SelectionMask 対応（2026-06）
- SelectionMask がアクティブなとき塗りつぶしを選択範囲内のみに限定
- 選択なし時は従来どおり contiguous/non-contiguous fill が動作
- undo/redo 正常動作確認済み
- system_status: Fill System 全項目 DONE

### ベクターレイヤー対応（2026-06）
- BrushTool: ベクターレイヤーへのストローク記録 + `overlay()` でライブプレビュー
- ToolDescriptor: ブラシ全サブツールを `TargetLayerKind::Both`（ラスター/ベクター共通）
- CanvasWidget: ストローク中のベクタープレビュー描画（ポリライン + 影）
- 保存設定の `targetLayerKind` 上書き問題を修正

### アイコン・起動
- アプリアイコン: `src/app/resources/app_icon.svg`（Inkscape でエクスポート）
- 再生成: `powershell -File scripts/export_icon.ps1`
- コンソール非表示: `WIN32` サブシステム
- デスクトップショートカット: `%USERPROFILE%\Desktop\LayeredPaintApp.lnk`

### UI 改善（2026-06）
- ToolPropertyPanel: ヘッダーを 1 行に圧縮（20x20px アイコンボタン）
- ColorSwatchWidget: 48x48px HiDPI 対応、角丸 FG/BG スウォッチ
- ToolPanel: 使用不可ツールを `QGraphicsOpacityEffect 0.30` でグレーアウト
- DockTitleBar: フロートボタン、全エリアドッキング対応

---

## 次のタスク（優先順）

### SAM2 ONNX モデルのセットアップ（AI選択ツール実用化）
- [ ] `scripts\download_sam2.ps1` を実行してモデルをエクスポート
- [ ] vcpkg で `onnxruntime:x64-windows` をインストール
- [ ] `cmake -DPAINT_USE_ONNX=ON -DCMAKE_TOOLCHAIN_FILE=...` でリビルド
- モデル配置先: `build\Release\models\sam2_encoder.onnx` + `sam2_decoder.onnx`
- 自動検出: アプリ起動時に `<exe>/models/` にモデルがあれば自動ロード

### ベクター編集（CSP レベルへ）
- [ ] ストローク選択ツール（描いたストロークをクリックで選択）
- [ ] 選択ストロークの移動・削除・色変更・幅変更
- [ ] ベジェフィッティング（点列 → スプライン近似）
- [ ] テーパー（線の入り抜き）

### Phase 0-3: Skia レンダリング移植（中期）
- `CanvasWidget::paintEvent` で `platform::skia::available()` により切り替え

### 未実装（HANDOFF 引き継ぎ）
- フォルダレイヤー入れ子構造（`Layer` にチャイルドリストなし）
- クリッピンググループ合成（`Renderer` 未実装）
- ComfyUI 画像アップロード（`POST /upload/image` 未実装）

---

## 既知の問題

- マウス入力の pressure は常に 1.0f（仕様）。ペンタブは `tabletEvent` 経由で正常
- ベクターストロークは単純なポリライン（ベジェ未実装）
- `tests/CMakeLists.txt` の `app_smoke_tests` が `C:/CraftRoot_KF6` を参照（旧環境パス）
