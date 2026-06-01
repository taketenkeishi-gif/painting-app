#ifdef PAINT_USE_SKIA

#include "platform/skia/SkiaPixelBuffer.h"

// Skia public headers
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"

namespace platform::skia {

// ──────────────────────────────────────────────────────────
// Impl (pimpl to keep Skia headers out of the public header)
// ──────────────────────────────────────────────────────────
struct SkiaPixelBuffer::Impl {
  SkBitmap bitmap;
};

// ──────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────
static inline SkColor toSkColor(const core::Color& c) noexcept {
  return SkColorSetARGB(c.a, c.r, c.g, c.b);
}

static inline core::Color fromSkColor(SkColor c) noexcept {
  return core::Color{
    static_cast<std::uint8_t>(SkColorGetR(c)),
    static_cast<std::uint8_t>(SkColorGetG(c)),
    static_cast<std::uint8_t>(SkColorGetB(c)),
    static_cast<std::uint8_t>(SkColorGetA(c))
  };
}

// ──────────────────────────────────────────────────────────
// Constructors / destructor / moves
// ──────────────────────────────────────────────────────────
SkiaPixelBuffer::SkiaPixelBuffer(int width, int height)
  : m_impl(new Impl)
{
  m_impl->bitmap.allocN32Pixels(width, height);
  m_impl->bitmap.eraseColor(SK_ColorTRANSPARENT);
}

SkiaPixelBuffer::~SkiaPixelBuffer() {
  delete m_impl;
}

SkiaPixelBuffer::SkiaPixelBuffer(SkiaPixelBuffer&& other) noexcept
  : m_impl(other.m_impl)
{
  other.m_impl = nullptr;
}

SkiaPixelBuffer& SkiaPixelBuffer::operator=(SkiaPixelBuffer&& other) noexcept {
  if (this != &other) {
    delete m_impl;
    m_impl = other.m_impl;
    other.m_impl = nullptr;
  }
  return *this;
}

// ──────────────────────────────────────────────────────────
// Dimensions
// ──────────────────────────────────────────────────────────
int SkiaPixelBuffer::width()  const noexcept { return m_impl ? m_impl->bitmap.width()  : 0; }
int SkiaPixelBuffer::height() const noexcept { return m_impl ? m_impl->bitmap.height() : 0; }

// ──────────────────────────────────────────────────────────
// Pixel access
// ──────────────────────────────────────────────────────────
core::Color SkiaPixelBuffer::pixel(int x, int y) const noexcept {
  if (!m_impl || x < 0 || y < 0 || x >= width() || y >= height())
    return core::Color::Transparent();
  return fromSkColor(*m_impl->bitmap.getAddr32(x, y));
}

bool SkiaPixelBuffer::setPixel(int x, int y, const core::Color& color) noexcept {
  if (!m_impl || x < 0 || y < 0 || x >= width() || y >= height())
    return false;
  *m_impl->bitmap.getAddr32(x, y) = toSkColor(color);
  return true;
}

void SkiaPixelBuffer::fill(const core::Color& color) noexcept {
  if (m_impl)
    m_impl->bitmap.eraseColor(toSkColor(color));
}

void SkiaPixelBuffer::resize(int w, int h, const core::Color& fillColor) {
  if (!m_impl) m_impl = new Impl;
  m_impl->bitmap.reset();
  m_impl->bitmap.allocN32Pixels(w, h);
  m_impl->bitmap.eraseColor(toSkColor(fillColor));
}

// ──────────────────────────────────────────────────────────
// Skia-specific
// ──────────────────────────────────────────────────────────
SkBitmap* SkiaPixelBuffer::bitmap() noexcept       { return m_impl ? &m_impl->bitmap : nullptr; }
const SkBitmap* SkiaPixelBuffer::bitmap() const noexcept { return m_impl ? &m_impl->bitmap : nullptr; }

SkCanvas* SkiaPixelBuffer::makeCanvas() {
  if (!m_impl) return nullptr;
  return new SkCanvas(m_impl->bitmap);
}

// ──────────────────────────────────────────────────────────
// Round-trip conversions
// ──────────────────────────────────────────────────────────
core::PixelBuffer SkiaPixelBuffer::toCoreBuffer() const {
  const int w = width(), h = height();
  core::PixelBuffer buf(w, h);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      buf.setPixel(x, y, pixel(x, y));
  return buf;
}

void SkiaPixelBuffer::fromCoreBuffer(const core::PixelBuffer& src) {
  resize(src.width(), src.height());
  for (int y = 0; y < src.height(); ++y)
    for (int x = 0; x < src.width(); ++x)
      setPixel(x, y, src.pixel(x, y));
}

} // namespace platform::skia

#endif // PAINT_USE_SKIA
