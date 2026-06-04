#pragma once

// SkiaRenderer — Skia-accelerated composite renderer.
//
// Replaces the software core::Renderer hot path when PAINT_USE_SKIA is
// defined.  The API mirrors core::Renderer so call sites only need to
// swap the type (or use the factory below).

#ifdef PAINT_USE_SKIA

#include "core/buffer/PixelBuffer.h"
#include "core/common/Rect.h"
#include "core/document/Document.h"

namespace platform::skia {

class SkiaRenderer {
public:
  SkiaRenderer() = default;
  ~SkiaRenderer() = default;

  // Full composite — mirrors core::Renderer::composite().
  core::PixelBuffer composite(const core::Document& document) const;

  // Incremental update into an existing buffer — mirrors compositeInto().
  void compositeInto(const core::Document& document,
                     core::PixelBuffer& target,
                     const core::Rect& dirtyRect) const;
};

} // namespace platform::skia

#endif // PAINT_USE_SKIA
