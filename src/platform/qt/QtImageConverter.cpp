#include "platform/qt/QtImageConverter.h"

#include <cstdint>

namespace platform::qt {

QImage QtImageConverter::toQImage(const core::PixelBuffer& buffer) {
  QImage image(buffer.width(), buffer.height(), QImage::Format_RGBA8888);
  for (int y = 0; y < buffer.height(); ++y) {
    auto* scanLine = image.scanLine(y);
    for (int x = 0; x < buffer.width(); ++x) {
      const core::Color color = buffer.pixel(x, y);
      const int offset = x * 4;
      scanLine[offset + 0] = color.r;
      scanLine[offset + 1] = color.g;
      scanLine[offset + 2] = color.b;
      scanLine[offset + 3] = color.a;
    }
  }
  return image;
}

core::PixelBuffer QtImageConverter::fromQImage(const QImage& image) {
  const QImage rgba = image.convertToFormat(QImage::Format_RGBA8888);
  core::PixelBuffer buffer(rgba.width(), rgba.height(), core::Color::Transparent());
  for (int y = 0; y < rgba.height(); ++y) {
    const auto* scanLine = rgba.constScanLine(y);
    for (int x = 0; x < rgba.width(); ++x) {
      const int offset = x * 4;
      buffer.setPixel(
          x,
          y,
          core::Color {
              static_cast<std::uint8_t>(scanLine[offset + 0]),
              static_cast<std::uint8_t>(scanLine[offset + 1]),
              static_cast<std::uint8_t>(scanLine[offset + 2]),
              static_cast<std::uint8_t>(scanLine[offset + 3])});
    }
  }
  return buffer;
}

} // namespace platform::qt
