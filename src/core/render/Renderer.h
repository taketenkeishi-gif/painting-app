#pragma once

#include "core/buffer/PixelBuffer.h"
#include "core/document/Document.h"

namespace core {

class Renderer {
public:
  PixelBuffer composite(const Document& document) const;
};

} // namespace core
