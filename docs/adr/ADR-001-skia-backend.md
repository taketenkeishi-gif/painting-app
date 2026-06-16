# ADR-001: Google Skia を 2D レンダリングバックエンド として採用

**Date:** 2026-05-xx  
**Status:** ✅ IMPLEMENTED  
**Relates:** Phase 0-1, SPEC.md Phase 0-1

---

## Context

現在のピクセル描画エンジンは CPU ベースの手書きループで実装されており、以下の問題がある：

- **性能:** CPU のみ利用、GPU が死んでいる
- **品質:** アンチエイリアスが手動実装（完全でない）
- **スケーラビリティ:** 高解像度での性能低下が激しい

## Decision

**Google Skia** を 2D レンダリングバックエンド として採用し、GPU アクセラレーション + 産業用品質の描画を実現する。

### なぜ Skia か

| 項目 | 評価 | 理由 |
|---|---|---|
| **品質** | ⭐⭐⭐⭐⭐ | Chrome / Flutter が使用。AA・サブピクセル精度は産業水準 |
| **ライセンス** | ⭐⭐⭐⭐⭐ | MIT ライセンス（商用利用可） |
| **実績** | ⭐⭐⭐⭐⭐ | 数十億デバイスで実証済み（10年以上） |
| **API** | ⭐⭐⭐⭐⭐ | ローカルライブラリ、ネットワークボトルネックなし |
| **GPU** | ⭐⭐⭐⭐⭐ | Vulkan / Metal / OpenGL 自動選択 |
| **ドキュメント** | ⭐⭐⭐☆☆ | 充実（Chrome のメイン実装が参考） |

## Alternatives

### 1. ❌ Qt Raster バックエンド のみ

- 既存 QImage / QPainter を使用
- **メリット:** Qt と統合されている
- **デメリット:** CPU 描画のみ、GPU サポートなし

### 2. ❌ 自作 GPU レンダラー（Vulkan / OpenGL 直接）

- Vulkan / OpenGL の raw API を使用
- **メリット:** 完全なカスタマイズ可
- **デメリット:** 工数が膨大（200+ 時間）、クロスプラットフォーム対応が難

### 3. ❌ Canvas API（Skia-lite）

- WebGL 互換の Canvas API
- **デメリット:** Web 向けで desktop には向かない

## Implementation

### ファイル構成

```
src/
  platform/
    skia/
      SkiaPixelBuffer.h/.cpp     // Skia ピクセルバッファ
      SkiaRenderer.h/.cpp        // Skia 描画エンジン
      SkiaIntegration.h/.cpp     // Qt ↔ Skia 境界
      CMakeLists.txt             // Skia 依存設定
```

### コンパイル構成

```cmake
# CMakeLists.txt
option(PAINT_USE_SKIA "Use Skia backend" OFF)

if(PAINT_USE_SKIA)
  find_package(Skia REQUIRED)
  target_link_libraries(LayeredPaintApp PRIVATE Skia::Skia)
  target_compile_definitions(LayeredPaintApp PRIVATE PAINT_USE_SKIA=1)
endif()
```

### vcpkg セットアップ

```powershell
# vcpkg でインストール
vcpkg install skia:x64-windows

# CMake configure
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DPAINT_USE_SKIA=ON
```

### API インターフェース

```cpp
// src/core/renderer/Renderer.h
// Platform agnostic interface

class PixelBuffer {
  virtual void drawCircle(float x, float y, float r, uint32_t color) = 0;
  // ...
};

// Skia 実装
class SkiaPixelBuffer : public PixelBuffer {
  SkBitmap m_bitmap;
  SkCanvas* m_canvas;
  
  void drawCircle(float x, float y, float r, uint32_t color) override {
    SkPaint paint;
    paint.setColor(color);
    m_canvas->drawCircle(x, y, r, paint);
  }
};
```

## Consequences

### ✅ メリット

- **GPU アクセラレーション** — RTX 3060 をフル活用
- **品質向上** — Chrome レベルの AA・品質
- **パフォーマンス** — 4K canvas でも 60 FPS を目指せる
- **メンテナンス** — Skia チームが feature / security update を継続

### ⚠️ デメリット・制約

- **ビルド時間:** Skia は巨大（初回ビルド 10+ 分）
- **バイナリサイズ:** Skia 非統合時の 2-3 倍
- **デフォルト OFF:** 互換性のため当面は CPU レンダラーがデフォルト
- **VRAM 要件:** 最小 2GB（RTX 3060 = 12GB で十分）

## Related

- **Phase 0-1:** `docs/SPEC.md` Phase 0-1
- **Decision:** [DECISIONS.md - ADR-001](../.claude/DECISIONS.md)
- **Status:** [PROJECT_STATUS.md](../.claude/PROJECT_STATUS.md)
- **Implementation:** `src/platform/skia/`

## References

- [Skia Official](https://skia.org/)
- [Skia GitHub](https://github.com/google/skia)
- [Chrome Rendering Architecture](https://www.chromium.org/developers/design-documents/gpu-accelerated-compositing-in-chrome/)

