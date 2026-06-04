#pragma once

// SkiaIntegration — thin factory / utility layer so app-level code can
// optionally select the Skia rendering path without #ifdef clutter at
// every call site.
//
// Usage pattern:
//
//   #include "platform/skia/SkiaIntegration.h"
//
//   // In AppController or CanvasWidget::paintEvent:
//   if (platform::skia::available()) {
//     platform::skia::SkiaRenderer renderer;
//     m_composited = renderer.composite(m_document);
//   } else {
//     core::Renderer renderer;
//     m_composited = renderer.composite(m_document);
//   }

#ifdef PAINT_USE_SKIA
#  include "platform/skia/SkiaRenderer.h"
#  include "platform/skia/SkiaPixelBuffer.h"
#endif

namespace platform::skia {

/// Returns true when the Skia backend was compiled in.
inline constexpr bool available() noexcept {
#ifdef PAINT_USE_SKIA
  return true;
#else
  return false;
#endif
}

} // namespace platform::skia
