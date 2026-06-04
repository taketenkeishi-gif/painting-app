#include "core/buffer/PixelBuffer.h"

#include <algorithm>

namespace core {

PixelBuffer::PixelBuffer(int width, int height, Color fill) {
  resize(width, height, fill);
}

bool PixelBuffer::inBounds(int x, int y) const noexcept {
  return x >= 0 && y >= 0 && x < m_width && y < m_height;
}

Color PixelBuffer::pixel(int x, int y) const noexcept {
  if (!inBounds(x, y)) {
    return Color::Transparent();
  }
  return m_pixels[indexOf(x, y)];
}

bool PixelBuffer::setPixel(int x, int y, const Color& color) noexcept {
  if (!inBounds(x, y)) {
    return false;
  }
  m_pixels[indexOf(x, y)] = color;
  return true;
}

void PixelBuffer::resize(int width, int height, Color fill) {
  m_width = std::max(0, width);
  m_height = std::max(0, height);
  m_pixels.assign(static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height), fill);
}

void PixelBuffer::fill(const Color& color) noexcept {
  std::fill(m_pixels.begin(), m_pixels.end(), color);
}

std::size_t PixelBuffer::indexOf(int x, int y) const noexcept {
  return static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(x);
}

} // namespace core
