#ifdef PAINT_USE_SKIA

#include "platform/skia/SkiaRenderer.h"
#include "platform/skia/SkiaLayerCache.h"
#include "platform/skia/SkiaPixelBuffer.h"

// Skia headers
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImage.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkImageInfo.h"

#include "core/document/Document.h"
#include "core/layer/Layer.h"
#include "core/render/RenderUtils.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace platform::skia {

namespace {

// ── Helper: convert core::Color → SkColor ──────────────────────────────────
inline SkColor toSk(const core::Color& c) noexcept {
  return SkColorSetARGB(c.a, c.r, c.g, c.b);
}

// ── Blend mode mapping: core → Skia ────────────────────────────────────────
SkBlendMode toSkBlend(core::BlendMode mode) noexcept {
  switch (mode) {
    case core::BlendMode::Multiply:     return SkBlendMode::kMultiply;
    case core::BlendMode::Screen:       return SkBlendMode::kScreen;
    case core::BlendMode::Overlay:      return SkBlendMode::kOverlay;
    case core::BlendMode::SoftLight:    return SkBlendMode::kSoftLight;
    case core::BlendMode::HardLight:    return SkBlendMode::kHardLight;
    case core::BlendMode::ColorDodge:   return SkBlendMode::kColorDodge;
    case core::BlendMode::ColorBurn:    return SkBlendMode::kColorBurn;
    case core::BlendMode::Darken:       return SkBlendMode::kDarken;
    case core::BlendMode::Lighten:      return SkBlendMode::kLighten;
    case core::BlendMode::Difference:   return SkBlendMode::kDifference;
    case core::BlendMode::Exclusion:    return SkBlendMode::kExclusion;
    case core::BlendMode::Hue:          return SkBlendMode::kHue;
    case core::BlendMode::HslSat:       return SkBlendMode::kSaturation;
    case core::BlendMode::HslColor:     return SkBlendMode::kColor;
    case core::BlendMode::Luminosity:   return SkBlendMode::kLuminosity;
    case core::BlendMode::LinearDodge:  return SkBlendMode::kPlus; // closest
    default:                            return SkBlendMode::kSrcOver;
  }
}

// ── Rasterise one layer's PixelBuffer into an SkBitmap (fallback: no cache)
SkBitmap layerToSkBitmap(const core::PixelBuffer& buf) {
  const int w = buf.width(), h = buf.height();
  SkBitmap bm;
  bm.allocN32Pixels(w, h);
  bm.eraseColor(SK_ColorTRANSPARENT);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      *bm.getAddr32(x, y) = toSk(buf.pixel(x, y));
  return bm;
}

} // anonymous namespace

// ──────────────────────────────────────────────────────────────────────────
// composite — full-document flatten using Skia draw operations
// ──────────────────────────────────────────────────────────────────────────
core::PixelBuffer SkiaRenderer::composite(const core::Document& document) const {
  const core::Size sz = document.canvasSize();
  core::PixelBuffer out(sz.width, sz.height, core::Color::Transparent());
  compositeInto(document, out, core::Rect{0, 0, sz.width, sz.height});
  return out;
}

// ──────────────────────────────────────────────────────────────────────────
// compositeInto — incremental dirty-rect update
// ──────────────────────────────────────────────────────────────────────────
void SkiaRenderer::compositeInto(const core::Document& document,
                                  core::PixelBuffer& target,
                                  const core::Rect& dirtyRect) const
{
  const core::Size sz = document.canvasSize();
  if (sz.width <= 0 || sz.height <= 0) return;
  if (target.width() != sz.width || target.height() != sz.height)
    target.resize(sz.width, sz.height, core::Color::Transparent());

  // Clamp dirty rect
  const int rx0 = std::clamp(dirtyRect.x, 0, sz.width);
  const int ry0 = std::clamp(dirtyRect.y, 0, sz.height);
  const int rx1 = std::clamp(dirtyRect.x + dirtyRect.width,  0, sz.width);
  const int ry1 = std::clamp(dirtyRect.y + dirtyRect.height, 0, sz.height);
  if (rx1 <= rx0 || ry1 <= ry0) return;

  const int rw = rx1 - rx0, rh = ry1 - ry0;

  // Create an offscreen Skia bitmap covering the dirty rect
  SkBitmap dst;
  dst.allocN32Pixels(rw, rh);
  // Fill with paper color (or transparent)
  if (document.paperVisible()) {
    const auto& p = document.paperColor();
    dst.eraseColor(SkColorSetARGB(p.a, p.r, p.g, p.b));
  } else {
    dst.eraseColor(SK_ColorTRANSPARENT);
  }

  SkCanvas canvas(dst);
  canvas.translate(static_cast<SkScalar>(-rx0), static_cast<SkScalar>(-ry0));

  // Composite each layer via Skia draw
  for (std::size_t i = 0; i < document.layerCount(); ++i) {
    const core::Layer& layer = document.layerAt(i);
    if (!layer.visible() || layer.opacity() <= 0.0f) continue;
    if (layer.kind() == core::LayerKind::Folder) continue;
    if (layer.isPaperLayer()) continue;

    // Use cached SkBitmap when available; otherwise blit from CPU buffer.
    SkBitmap tempBm;
    const SkBitmap* layerBm;
    if (m_cache) {
      layerBm = &m_cache->getBitmap(layer.id(), layer.buffer());
    } else {
      tempBm = layerToSkBitmap(layer.buffer());
      layerBm = &tempBm;
    }

    SkPaint paint;
    paint.setBlendMode(toSkBlend(layer.blendMode()));
    paint.setAlphaf(std::clamp(layer.opacity(), 0.0f, 1.0f));

    canvas.drawImage(layerBm->asImage(), 0.0f, 0.0f, {}, &paint);
  }

  // Read the dirty rect back into target
  for (int y = 0; y < rh; ++y)
    for (int x = 0; x < rw; ++x) {
      const SkColor c = *dst.getAddr32(x, y);
      target.setPixel(rx0 + x, ry0 + y, core::Color{
        static_cast<std::uint8_t>(SkColorGetR(c)),
        static_cast<std::uint8_t>(SkColorGetG(c)),
        static_cast<std::uint8_t>(SkColorGetB(c)),
        static_cast<std::uint8_t>(SkColorGetA(c))
      });
    }
}

} // namespace platform::skia

#endif // PAINT_USE_SKIA
