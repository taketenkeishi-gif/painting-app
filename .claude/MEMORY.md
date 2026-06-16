# Project Memory — Painting-app

プロジェクト固有のメモ・制約・パターン・注意点。

---

## プロジェクト特性

- **型式:** C++ / Qt 6.7.2 ベースの描画アプリ
- **フェーズ:** Phase 0（描画エンジン根本再構築）〜 Phase 1 途中
- **開発方針:** OSS 吸収ファースト（Skia / ONNX など）
- **品質基準:** CSP / Krita 同等以上の描画品質

---

## 重要な実装パターン

### 1. core は Qt フリー

**ルール:** `src/core/` に `#include <Qt...>` を持ち込まない

```cpp
// ✅ OK
#include "core/document/Document.h"
#include "core/layer/Layer.h"

// ❌ NG
#include <QImage>      // QImage を core に持ち込まない
#include <QPixmap>     // 同上
```

**理由:** core の再利用性・テスト容易性のため

**実装:**
- Qt 変換は `src/platform/qt/` に隔離
- core は raw `PixelBuffer` / `VectorPath` で扱う

### 2. Platform 層経由で OSS を統合

**ルール:** Skia / ONNX などの OSS ラッパーは `src/platform/` 以下に限定

```cpp
// ✅ OK
src/platform/skia/SkiaRenderer.h
src/platform/qt/PixelBufferQt.cpp

// ❌ NG
src/core/renderer/SkiaRenderer.h  // core に Skia を持ち込まない
```

**理由:** バックエンド交換可能性のため

### 3. History は Snapshot-based

**パターン:** Undo/Redo は `before` / `after` コピーで実装

```cpp
struct HistoryEntry {
  PixelBuffer before;
  PixelBuffer after;
};
```

**メリット:**
- 実装がシンプル（操作の複雑さと無関係）
- デバッグが簡単（before/after を表示すれば確認可)

**デメリット:**
- メモリ消費（Phase 1+ で差分記録に改善）

### 4. ToolDescriptor は Code-driven

**ルール:** ツール定義は JSON 外部定義ではなく、code で定義

```cpp
// ✅ OK: src/app/ui/ToolDescriptor.cpp
ToolCatalog::load() {
  auto brushTool = ToolDescriptor(
    "brush",
    ToolCategory::Paint,
    /* ... */
  );
}

// ❌ NG: JSON 外部定義（Phase 2+ まで不採用）
```

**理由:**
- IDE リファクタリング対応
- 実行時エラーなし
- development cycle が早い

---

## ビルド・環境

### Qt パス

```
C:\Qt\6.7.2\msvc2019_64
```

### CMake コマンド

```powershell
# 標準ビルド（Release）
cmake --build build --config Release

# テストビルド
.\scripts\build-tests.ps1

# Skia 有効
cmake -DPAINT_USE_SKIA=ON -B build

# ONNX 有効
cmake -DPAINT_USE_ONNX=ON -B build
```

### デバッグ構成（VS Code）

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug Paint App",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/Debug/LayeredPaintApp.exe",
      "args": [],
      "cwd": "${workspaceFolder}/build"
    }
  ]
}
```

---

## よくあるエラーと対処

### 1. CMake `Qt6 not found`

```
❌ CMake Error at CMakeLists.txt:50
   Could not find Qt 6
```

**対処:**
```powershell
cmake -DCMAKE_PREFIX_PATH="C:\Qt\6.7.2\msvc2019_64" -B build
```

### 2. Skia ビルド timeout

```
❌ vcpkg install skia:x64-windows が 30+ 分
```

**対処:**
- Skia は巨大（初回ビルド 200+ MB）
- ネットワークが遅い場合は事前に wifi で落としておく
- `vcpkg --binarycaching` でキャッシング活用

### 3. ONNX モデルファイル未検出

```
⚠️ [WARNING] SAM2 models not found in <exe>/models/
```

**対処:**
```powershell
# モデルダウンロード
.\scripts\download_sam2.ps1

# build\Release\models\ に配置される
```

---

## パフォーマンスチェック

### ブラシ描画が遅い場合

1. **Debug ビルド確認:** Debug/Release の別
2. **Skia 有効化:** `PAINT_USE_SKIA=ON` で GPU 加速
3. **Canvas サイズ確認:** 4K 以上の場合は GPU VRAM 確認

### レイヤー合成が遅い場合

1. **レイヤー数確認:** 10+ layer の場合、GPU compose 検討
2. **透明度ブレンド:** Alpha blending が多い場合、Skia GPU compose へ

---

## テストとデバッグ

### ユニットテスト

```powershell
# テストビルド & 実行
.\scripts\build-tests.ps1

# 個別テスト実行
ctest -R "BrushEngine*" --output-on-failure
```

### Smoke テスト（UI）

- ウィンドウが開く
- ドックが表示される
- レイヤー追加 / 削除
- ブラシ描画
- Undo/Redo

---

## 文書体系

### 新規セッション開始時の読順

1. **PROJECT_STATUS.md** — 現在の実装進捗
2. **SPEC.md** — ビジョン・フェーズ
3. **SPEC2.md** — 実装仕様・データモデル
4. **ADR/** — 技術判断記録
5. **DEV_LOG.md** — 作業履歴（参考）

### 更新責任

- **PROJECT_STATUS.md** — セッション終了時（進捗更新）
- **DEV_LOG.md** — セッション終了時（作業記録）
- **DECISIONS.md** — 重要判断直後
- **FAILED_ATTEMPTS.md** — 失敗時直後

---

## 注意事項

### Git コミット

```bash
# 型チェック & テスト実行を忘れずに
npm run type-check
npm run test

# コミット前に SPEC や .claude/* を更新
git add docs/SPEC2.md .claude/PROJECT_STATUS.md

# Clear commit message
git commit -m "feat: ベクターレイヤーの点編集機能を実装"
```

### ドキュメント変更

- SPEC.md / SPEC2.md の仕様内容は変更しない
- 新規判断は DECISIONS.md / docs/adr/ に記録
- 失敗記録は FAILED_ATTEMPTS.md に追記

---

## 関連リポジトリ

- **MV-studio-app** — 動画編集ソフト（静止画MV向け）
  - Painting-app の描画成果物を使用する予定
  - 連携は Phase 2 で設計

