#ifdef PAINT_USE_SKIA

#include "platform/skia/SkiaLayerCache.h"

#include "include/core/SkColor.h"

namespace platform::skia {

namespace {
inline SkColor pixelToSk(const core::Color& c) noexcept {
  return SkColorSetARGB(c.a, c.r, c.g, c.b);
}
} // namespace

void SkiaLayerCache::blitFromBuffer(SkBitmap& bm, const core::PixelBuffer& buf)
{
  const int w = buf.width(), h = buf.height();
  if (bm.width() != w || bm.height() != h)
    bm.allocN32Pixels(w, h);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      *bm.getAddr32(x, y) = pixelToSk(buf.pixel(x, y));
}

const SkBitmap& SkiaLayerCache::getBitmap(uint32_t layerId,
                                           const core::PixelBuffer& buf)
{
  auto& entry = m_cache[layerId];
  if (entry.dirty) {
    blitFromBuffer(entry.bitmap, buf);
    entry.dirty = false;
  }
  return entry.bitmap;
}

void SkiaLayerCache::markDirty(uint32_t layerId)
{
  m_cache[layerId].dirty = true;
}

void SkiaLayerCache::invalidateAll()
{
  for (auto& [id, entry] : m_cache)
    entry.dirty = true;
}

void SkiaLayerCache::evict(uint32_t layerId)
{
  m_cache.erase(layerId);
}

} // namespace platform::skia

#endif // PAINT_USE_SKIA
