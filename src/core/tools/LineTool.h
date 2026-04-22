#pragma once

#include <string_view>

#include "core/common/Point.h"
#include "core/tools/ITool.h"

namespace core {

class LineTool : public ITool {
public:
  void setSnapAngleDegrees(int snapAngleDegrees) noexcept { m_snapAngleDegrees = snapAngleDegrees < 0 ? 0 : snapAngleDegrees; }

  ToolKind kind() const noexcept override { return ToolKind::Line; }
  std::string_view displayName() const noexcept override { return "Line"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;
  ToolOverlayState overlay() const override;

private:
  Point snappedPoint(const Point& start, const Point& rawEnd) const;
  void drawLine(Layer& layer, const Point& from, const Point& to, const Color& color, int size) const;
  void addVectorLine(Layer& layer, const Point& from, const Point& to, const Color& color, int size) const;
  void stampCircle(PixelBuffer& buffer, const Point& center, int radius, const Color& color, bool lockAlpha) const;
  void blendPixel(PixelBuffer& buffer, int x, int y, const Color& src, bool lockAlpha) const;

  bool m_drawing {false};
  int m_snapAngleDegrees {0};
  Point m_start {0, 0};
  Point m_current {0, 0};
};

} // namespace core
