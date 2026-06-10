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

## インデックス

| 日付 | 内容 | 原因 | 修正済 |
|---|---|---|---|
| 2026-06-08 | (例) Skia 色反転 | フォーマット誤解 | ✅ |
| 2026-06-10 | FreeTransform / Layer Offset 不整合 | アーキテクチャ設計差分を後回しにした | ✅ |

