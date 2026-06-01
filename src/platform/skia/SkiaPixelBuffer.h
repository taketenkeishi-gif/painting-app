#pragma once

// SkiaPixelBuffer — wraps an SkBitmap (BGRA_8888 / N32) and exposes
// the same pixel-access API as core::PixelBuffer so rendering code can
// operate through a common interface without template overhead.
//
// Build guard: only compiled when PAINT_USE_SKIA is defined in CMake.
// If Skia is absent the project falls back to the plain PixelBuffer
// software path with no API changes in the rest of the codebase.

#ifdef PAINT_USE_SKIA

#include <cstdint>

// Forward-declare Skia types to keep this header lean.
class SkBitmap;
class SkCanvas;

#include "core/buffer/PixelBuffer.h" // for core::Color

namespace platform::skia {

class SkiaPixelBuffer {
public:
  SkiaPixelBuffer() = default;
  SkiaPixelBuffer(int width, int height);
  ~SkiaPixelBuffer();

  // Non-copyable; move is allowed.
  SkiaPixelBuffer(const SkiaPixelBuffer&) = delete;
  SkiaPixelBuffer& operator=(const SkiaPixelBuffer&) = delete;
  SkiaPixelBuffer(SkiaPixelBuffer&&) noexcept;
  SkiaPixelBuffer& operator=(SkiaPixelBuffer&&) noexcept;

  int width()  const noexcept;
  int height() const noexcept;

  // Direct ARGB pixel access (matches core::PixelBuffer semantics).
  core::Color pixel(int x, int y) const noexcept;
  bool setPixel(int x, int y, const core::Color& color) noexcept;

  void fill(const core::Color& color) noexcept;
  void resize(int width, int height, const core::Color& fill = core::Color::Transparent());

  // --- Skia-specific accessors ---

  // Returns a non-owning pointer to the underlying SkBitmap.
  // Callers must not delete or outlive this object.
  SkBitmap* bitmap() noexcept;
  const SkBitmap* bitmap() const noexcept;

  // Creates a temporary canvas that draws into this bitmap.
  // The caller takes ownership of the returned SkCanvas.
  SkCanvas* makeCanvas();

  // Copy pixel data into a core::PixelBuffer for tools/tests that need it.
  core::PixelBuffer toCoreBuffer() const;

  // Blit data from a core::PixelBuffer into this Skia bitmap.
  void fromCoreBuffer(const core::PixelBuffer& src);

private:
  struct Impl;
  Impl* m_impl {nullptr};
};

} // namespace platform::skia

#endif // PAINT_USE_SKIA
