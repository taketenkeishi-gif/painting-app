#pragma once

// SkiaRenderer — Skia-accelerated composite renderer.
//
// Replaces the software core::Renderer hot path when PAINT_USE_SKIA is
// defined.  The API mirrors core::Renderer so call sites only need to
// swap the type (or use the factory below).

#ifdef PAINT_USE_SKIA

#include <QImage>

#include "core/buffer/PixelBuffer.h"
#include "core/common/Rect.h"
#include "core/document/Document.h"

namespace platform::skia {

class SkiaLayerCache;

class SkiaRenderer {
public:
  SkiaRenderer() = default;
  ~SkiaRenderer() = default;

  // Optional per-layer bitmap cache. Set once by AppController at startup.
  // If null, every composite call blits all layers (original behaviour).
  void setCache(SkiaLayerCache* cache) noexcept { m_cache = cache; }

  // Full composite — mirrors core::Renderer::composite().
  core::PixelBuffer composite(const core::Document& document) const;

  // Incremental update into an existing buffer — mirrors compositeInto().
  void compositeInto(const core::Document& document,
                     core::PixelBuffer& target,
                     const core::Rect& dirtyRect) const;

  // Fast display path: composite dirty rect directly into a QImage.
  // Skips the SkBitmap→PixelBuffer readback; saves one O(W×H) copy per frame.
  // target is resized/reallocated if it doesn't match the canvas dimensions.
  // Format is always QImage::Format_RGBA8888.
  void compositeIntoQImage(const core::Document& document,
                            QImage& target,
                            const core::Rect& dirtyRect) const;

private:
  SkiaLayerCache* m_cache {nullptr};
};

} // namespace platform::skia

#endif // PAINT_USE_SKIA
