#include "platform/qt/QtImageConverter.h"

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

} // namespace platform::qt
