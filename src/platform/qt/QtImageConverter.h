#pragma once

#include <QImage>

#include "core/buffer/PixelBuffer.h"

namespace platform::qt {

class QtImageConverter {
public:
  static QImage toQImage(const core::PixelBuffer& buffer);
  static core::PixelBuffer fromQImage(const QImage& image);
};

} // namespace platform::qt
