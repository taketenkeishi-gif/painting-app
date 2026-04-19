#pragma once

#include "core/common/Point.h"
#include "core/layer/Layer.h"
#include "core/tools/ToolTypes.h"

namespace core {

class BrushTool {
public:
  void setColor(const Color& color) noexcept { m_settings.color = color; }
  void setSize(int size) noexcept { m_settings.size = size < 1 ? 1 : size; }

  const BrushSettings& settings() const noexcept { return m_settings; }

  void stroke(Layer& layer, const Point& from, const Point& to) const;

private:
  void stampCircle(PixelBuffer& buffer, const Point& center, int radius, const Color& color) const;
  void blendPixel(PixelBuffer& buffer, int x, int y, const Color& src) const;

  BrushSettings m_settings;
};

} // namespace core
