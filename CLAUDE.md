# CLAUDE.md — LayeredPaintApp

> このファイルは Claude Code が会話開始時に自動読み込みする設定ファイルです。
> Stop フックにより会話終了時に自動更新されます。

---

## プロジェクト概要

CSP / Krita 同等以上の描画品質をベースに、将来的に静止画MV制作ソフトと連携する
「自分に最適化されたクリエイティブスイート」の中核アプリ。
Adobe でいえば Photoshop + After Effects の関係を、独自スタック・ローカルファーストで実現する。

- **リポジトリ**: https://github.com/taketenkeishi-gif/painting-app
- **スタック**: C++17 / Qt 6.7.2 / CMake 4.x / MSVC 2022
- **Qt パス**: `C:\Qt\6.7.2\msvc2019_64`

---

## クイックスタート

```powershell
cmake --build build --config Release   # ビルド
.\launch.bat                           # ビルド + 起動
```

---

## ディレクトリ構成

```
src/
  core/         Qt非依存の描画コア（Document / Layer / Renderer / Tools / Selection / AI）
  app/
    bridge/     AppController（UI ↔ core 橋渡し）
    canvasview/ CanvasWidget（描画・オーバーレイ）
    mainwindow/ MainWindow（メインウィンドウ・全メニュー・DockTitleBar）
    panels/     ToolPanel / SubToolPanel / LayerPanel / ColorWheelWidget
    ui/         ToolDescriptor / UiState / IconLoader / Theme
  platform/
    qt/         QImage ↔ PixelBuffer 変換
    skia/       Skia バックエンド（PAINT_USE_SKIA=ON 時のみ）
docs/           SPEC.md / HANDOFF.md / architecture.md / system_status.md
scripts/        build.ps1 / run.ps1 / export_icon.ps1 / sync_claude_md.ps1
tests/          core_tests / app_smoke_tests
```

---

## 重要ファイル

| ファイル | 役割 |
|---|---|
| `src/core/document/Document.h` | Document クラス（キャンバスサイズ・DPI・レイヤー管理） |
| `src/core/layer/Layer.h` | Layer / VectorPath 定義 |
| `src/core/tools/ToolContext.h` | ToolOverlayState（ベクターライブプレビュー含む） |
| `src/app/bridge/AppController.h` | UI ↔ コアの全操作インターフェース |
| `src/app/mainwindow/MainWindow.cpp` | メインウィンドウ・ダイアログ・DockTitleBar |
| `src/app/ui/ToolDescriptor.cpp` | サブツール定義（TargetLayerKind 含む） |
| `src/app/resources/app_icon.svg` | アプリアイコン SVG ソース |
| `docs/SPEC.md` | プロジェクトビジョン・フェーズ構成 |
| `.claude/HANDOFF.md` | 実装済み内容・次タスク詳細 |

---

## 現在の実装状態

### 完了済み
- ブラシエンジン: size / opacity / hardness / flow / spacing / velocity / texture / wet-mix / smear
- ベクターレイヤー: ストローク記録・ライブプレビュー（Both 対応）
- レイヤーシステム: Raster / Vector / Folder / Adjustment / マスク / クリッピング UI
- 選択ツール: 矩形・なげなわ・多角形・自動選択 + マーチングアンツ
- カラーシステム: HSV カラーホイール / スライダー / HiDPI スウォッチ
- ドックパネル: フロート / タブ / 全エリアドッキング
- キャンバスリサイズ: プリセット付き新規ダイアログ / アンカーポイント付きリサイズ
- Skia バックエンド: 基盤整備済み（デフォルト OFF）

### TODO（優先順）
1. ベクター編集: ストローク選択・移動・削除・ベジェフィッティング・テーパー
2. フォルダレイヤーのネスト構造
3. クリッピンググループ合成（Renderer）
4. Skia レンダリング本移植
5. ペンタブ筆圧（Wintab / Windows Ink）

---

## 開発方針：OSS 吸収ファースト

新機能を実装する前に、まず実績ある OSS ライブラリ・アルゴリズムの採用を検討する。

**基本姿勢:**
- スクラッチ実装より OSS 取り込みを優先し、開発速度と品質を同時に確保する
- ライセンス（MIT / BSD / Apache 2.0 を優先）を確認してから採用する
- OSS を採用した場合はラッパー層を薄く保ち、将来の差し替えを妨げない設計にする
- 採用候補は実装前にここか HANDOFF.md に記載して認識合わせする

**主な採用候補領域:**

| 領域 | 候補 OSS |
|---|---|
| 2D レンダリング | Skia（既採用・デフォルト OFF）|
| ベクターパス | nanosvg / Blend2D |
| 色管理 | Little CMS 2 (lcms2) |
| 画像 I/O | libpng / libjpeg-turbo / OpenEXR / libwebp |
| ファイル形式 | libzip（PSD 解析は検討中）|
| AI 推論 | ONNX Runtime（将来フェーズ）|
| テスト | GoogleTest（既採用）|

---

## コーディング規約

- C++17、`#pragma once`
- コメントは「なぜ」だけ（何をするかはコードで自明）
- `core::` 名前空間は Qt 非依存を維持
- `TargetLayerKind` は JSON から読まない（カタログ定義が正）
- OSS ラッパーは `src/platform/` 以下に配置し `core::` から直接 OSS API を呼ばない

---
<!-- AUTO-SYNC: Stop フックにより更新 -->
## 最終同期情報

- **ブランチ**: (初回同期前)
- **最終コミット**: (初回同期前)
- **system_status**: WIP - 件 / TODO - 件
<!-- /AUTO-SYNC -->