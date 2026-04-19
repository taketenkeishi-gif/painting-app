#pragma once

#include <string_view>

#include "core/common/Point.h"
#include "core/layer/Layer.h"
#include "core/tools/ITool.h"
#include "core/tools/ToolTypes.h"

namespace core {

class BrushTool : public ITool {
public:
  void setColor(const Color& color) noexcept { m_settings.color = color; }
  void setSize(int size) noexcept { m_settings.size = size < 1 ? 1 : size; }

  const BrushSettings& settings() const noexcept { return m_settings; }

  void stroke(Layer& layer, const Point& from, const Point& to) const;

  ToolKind kind() const noexcept override { return ToolKind::Brush; }
  std::string_view displayName() const noexcept override { return "Brush"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;

private:
  void stampCircle(PixelBuffer& buffer, const Point& center, int radius, const Color& color) const;
  void blendPixel(PixelBuffer& buffer, int x, int y, const Color& src) const;

  BrushSettings m_settings;
  bool m_drawing {false};
  Point m_lastPoint {0, 0};
};

} // namespace core
