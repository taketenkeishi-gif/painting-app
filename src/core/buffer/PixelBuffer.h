#pragma once

#include <cstddef>
#include <vector>

#include "core/color/Color.h"

namespace core {

class PixelBuffer {
public:
  PixelBuffer() = default;
  PixelBuffer(int width, int height, Color fill = Color::Transparent());

  int width() const noexcept { return m_width; }
  int height() const noexcept { return m_height; }

  bool inBounds(int x, int y) const noexcept;
  Color pixel(int x, int y) const noexcept;
  bool setPixel(int x, int y, const Color& color) noexcept;

  void resize(int width, int height, Color fill = Color::Transparent());
  void fill(const Color& color) noexcept;

private:
  std::size_t indexOf(int x, int y) const noexcept;

  int m_width {0};
  int m_height {0};
  std::vector<Color> m_pixels;
};

} // namespace core
