# 失敗記録 — Paint App

実装中に失敗した試行錯誤・デバッグの過程を記録。次セッションで同じ失敗を避けるため。

---

## 2026-06-08

### (記録例) Qt + Skia での SkBitmap ↔ QImage 変換で色成分反転

**試み:** Skia の SkBitmap を直接 QImage に変換しようとした

**理由:** 両方とも RGBA フォーマットだから変換は不要と思い込んだ

**失敗内容:**

```
QImage に表示されたが、色が反転していた（赤↔青）
```

**原因:**

- Skia: BGRA フォーマット (Intel エンディアン向け)
- Qt: RGBA フォーマット
- 単純コピーでは色が反転

**学んだこと:**

- フォーマット変換は byte-level で明示的に行う必要がある
- platform 層で `SkBitmap → QImage` conversion function を用意すべき
- `src/platform/qt/PixelBufferQt.cpp` で明示的にループして byte swap

**修正方法:**

```cpp
// SkBitmap (BGRA) → QImage (RGBA)
for (int i = 0; i < width * height; ++i) {
  uint8_t b = bgra[i*4 + 0];
  uint8_t g = bgra[i*4 + 1];
  uint8_t r = bgra[i*4 + 2];
  uint8_t a = bgra[i*4 + 3];
  
  rgba[i*4 + 0] = r;
  rgba[i*4 + 1] = g;
  rgba[i*4 + 2] = b;
  rgba[i*4 + 3] = a;
}
```

**参考:** `src/platform/qt/PixelBufferQt.h`

---

## 記録フォーマット

失敗が判明した直後に記録する。

```markdown
### YYYY-MM-DD: [タイトル]

**試み:** どんなことをしようとしたか

**理由:** なぜそれを試したのか

**失敗内容:** 何が起きたのか

**原因:** 根本的な原因は何か

**学んだこと:** 次に生かすポイント

**修正方法:** 実際にどう直したか（コード例含む）

**参考:** 関連するファイルやコミット
```

---

---

## 2026-06-10

### FreeTransform が Layer Offset モデルと非互換（アーキテクチャ不整合）

**試み:** FreeTransform の座標計算・描画座標・commit 先バッファを個別に修正

**理由:** ビルドが通る・画面表示がある状態で「あと1箇所直せば動く」と判断し続けた

**失敗内容:**

```
修正1: canvasW/H をバッファサイズから取得 → 改善せず
修正2: offX/Y をキャンバス座標に変換 → 改善せず
修正3: commit の tight bbox をキャンバス座標で計算 → 改善せず
→ 毎回別の症状が現れ、根本解決に至らなかった
```

**原因:**

FreeTransform は「レイヤー = キャンバスサイズのバッファ」を前提としたCanvas-bound設計だった。
Layer Offset 移行後は「レイヤー = 独立バッファ + offset座標」になっているが、
FreeTransform 側のデータモデル認識が更新されていなかった。

局所修正を続けた結果、4箇所以上の不整合を個別パッチで対処することになった。

**学んだこと:**

- 「ビルド成功」「画面に何か表示される」は完了条件ではない
- 下流（UI/Controller）から上流（DataModel）へ向かう修正連鎖はアーキテクチャ不整合のサイン
- 同じ問題で2回修正して改善しない場合は即停止して設計差分を確認する
- `like Photoshop / CSP` という言葉が出た時点でデータモデル比較から始めるべきだった

**参考:** `GPT_DEVELOPMENT_MANAGER.md` → Architecture Mismatch Prevention Rule

---

## 2026-06-10

### Build 成功なのに動作変化なし（実行バイナリ不一致）

**試み:** FreeTransform の pixel クリア条件修正、offset 計算修正、commit パス修正を複数回実施

**理由:** Build が通るため修正が反映されていると思い込んだ

**失敗内容:**

```
build\src\Release\LayeredPaintApp.exe  → 13:19:26（最新ビルド）
build_cv\src\Release\LayeredPaintApp.exe → 06-07 09:46（3日前）← 実際に起動中

Get-Process で確認するまで気づかなかった
```

**原因:**

- Qt Creator が `build_cv` をデフォルトの実行ディレクトリとして設定していた
- `launch.bat` は `build\` を参照するが、別の手段で古い `build_cv\` を起動していた
- Build と Run が別バイナリを指していた

**学んだこと:**

- 「Build 成功」と「修正済みバイナリが動作中」は別の確認ステップ
- UX 変化なし・ログが出ない場合、コード修正前に `Get-Process` でパス確認が必須
- 複数の `build*/` ディレクトリが存在するプロジェクトでは特に注意

**参考:** `GPT_DEVELOPMENT_MANAGER.md` → Build Artifact Verification Rule

## 2026-06-12

### SAM ONNX デコーダのマスク座標ずれ（パディング非考慮リサイズ）

**試み:** SAM デコーダに `orig_im_size=[H, W]` を渡し、返ってきた `maskH×maskW` マスクをキャンバスにそのまま貼り付けた

**理由:** `maskH==originalHeight && maskW==originalWidth` なら座標は一致しているはずと思い込んだ

**失敗内容:**

```
composited=800x600, scale=1.28 の場合:
  ストローク重心 y ≈ 394（キャンバス座標）
  マスク重心    y ≈ 236（マスクピクセル座標）
  → 青いマスクの形状は正しいが、Y方向に上にずれて表示される
  X方向は正確（ストローク重心 x≈406、マスク重心 x≈407 一致）
```

**原因:**

SAM の ONNX デコーダは `orig_im_size=[H, W]` を受け取っても、
**パディング済み 1024×1024 から [H, W] へ直接リサイズ**する実装の場合がある（パディング除去なし）。

エンコーダでは `scale = min(1024/W, 1024/H)` でアスペクト比維持リサイズし、
残りをゼロパディングする:

```
800×600 の場合:
  scale = 1024/800 = 1.28
  newW = 1024（幅フル）  → X パディングなし → X 誤差なし ✓
  newH = round(600*1.28) = 768  → Y に 256px パディング
  デコーダが 1024→600 直接リサイズ: y_mask = y_canvas × 0.75（0.75 = 600/1024）
```

X が正確で Y のみずれる非対称な症状はこのパターンの特徴的サイン。

**学んだこと:**

- `maskH == originalHeight` だからといって座標が一致する保証はない
- SAM ONNX デコーダの `orig_im_size` 実装はモデルによって異なる
- 「X が正確、Y のみずれる」→ **幅だけ 1024 にフィットしている landscape 画像**を疑う
- デバッグ十字（ストローク重心 vs マスク重心）の X 一致・Y 不一致で即判断できる

**修正方法:**

`OnnxSegEngine::decode` のマスク→キャンバス変換を「スケール考慮バイリニア補間」に統一:

```cpp
// 正しい式: canvas(x,y) → encoder(x*scale, y*scale) → mask
// scaleW = scale * maskW / kInputSize
// scaleH = scale * maskH / kInputSize
const float scaleW = scale * static_cast<float>(maskW) / static_cast<float>(kInputSize);
const float scaleH = scale * static_cast<float>(maskH) / static_cast<float>(kInputSize);
for (int y = 0; y < originalHeight; ++y) {
    for (int x = 0; x < originalWidth; ++x) {
        const float mxf = (x + 0.5f) * scaleW - 0.5f;
        const float myf = (y + 0.5f) * scaleH - 0.5f;
        // バイリニア補間してピクセル値を取得...
    }
}
```

検証:
- 800×600 X軸: `1.28 × 800/1024 = 1.0` → mx = x（変化なし、パディングなしで正しい）
- 800×600 Y軸: `1.28 × 600/1024 = 0.75` → my = y×0.75（圧縮されたマスクを正しく伸長）
- maskH=1024 の場合: `scale × 1024/1024 = scale` → encoder 座標へ直接マップ（従来と同等）

**参考:** `src/core/ai/OnnxSegEngine.cpp` → `decode()` の「マスクを元画像サイズへ展開」節

---

## インデックス

| 日付 | 内容 | 原因 | 修正済 |
|---|---|---|---|
| 2026-06-08 | (例) Skia 色反転 | フォーマット誤解 | ✅ |
| 2026-06-10 | FreeTransform / Layer Offset 不整合 | アーキテクチャ設計差分を後回しにした | ✅ |
| 2026-06-10 | Build 成功なのに動作変化なし | 別ディレクトリの古いバイナリを起動していた | ✅ |
| 2026-06-12 | SAM マスク Y座標ずれ（パディング非考慮リサイズ） | デコーダが 1024×1024 から直接リサイズ、パディング除去なし | ✅ |

