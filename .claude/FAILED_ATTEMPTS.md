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

## インデックス

| 日付 | 内容 | 原因 | 修正済 |
|---|---|---|---|
| 2026-06-08 | (例) Skia 色反転 | フォーマット誤解 | ✅ |

