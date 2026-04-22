#pragma once

#include "core/buffer/PixelBuffer.h"
#include "core/common/Rect.h"
#include "core/document/Document.h"

namespace core {

class Renderer {
public:
  PixelBuffer composite(const Document& document) const;
  void compositeInto(const Document& document, PixelBuffer& target, const Rect& dirtyRect) const;
};

} // namespace core
