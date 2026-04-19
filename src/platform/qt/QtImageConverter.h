#pragma once

#include <QImage>

#include "core/buffer/PixelBuffer.h"

namespace platform::qt {

class QtImageConverter {
public:
  static QImage toQImage(const core::PixelBuffer& buffer);
};

} // namespace platform::qt
