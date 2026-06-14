#pragma once

#ifdef PAINT_USE_SKIA

#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "include/core/SkBitmap.h"
#include "core/buffer/PixelBuffer.h"

namespace platform::skia {

// Per-layer SkBitmap cache.
//
// Owned by AppController. SkiaRenderer receives a pointer and uses it
// to skip the O(W×H) PixelBuffer→SkBitmap blit on clean layers.
//
// Invariant: if layer id is NOT in m_dirty the cached SkBitmap is up-to-date.
class SkiaLayerCache {
public:
  // Returns a valid SkBitmap for the layer. Blits from buf only if dirty/absent.
  const SkBitmap& getBitmap(uint32_t layerId, const core::PixelBuffer& buf);

  void markDirty(uint32_t layerId);
  void invalidateAll();
  void evict(uint32_t layerId);  // call on layer deletion

  // Debug counters — read from DebugServer / tests.
  uint64_t blitCount()  const noexcept { return m_blitCount.load(); }
  uint64_t hitCount()   const noexcept { return m_hitCount.load(); }
  void     resetStats() noexcept { m_blitCount = 0; m_hitCount = 0; }

private:
  struct Entry {
    SkBitmap bitmap;
    bool dirty {true};
  };
  std::unordered_map<uint32_t, Entry> m_cache;

  std::atomic<uint64_t> m_blitCount {0};
  std::atomic<uint64_t> m_hitCount  {0};

  static void blitFromBuffer(SkBitmap& bm, const core::PixelBuffer& buf);
};

} // namespace platform::skia

#endif // PAINT_USE_SKIA
