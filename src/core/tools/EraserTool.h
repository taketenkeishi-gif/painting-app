#pragma once

#include <string_view>

#include "core/common/Point.h"
#include "core/layer/Layer.h"
#include "core/tools/ITool.h"

namespace core {

class EraserTool : public ITool {
public:
  void setSize(int size) noexcept { m_size = size < 1 ? 1 : size; }
  int size() const noexcept { return m_size; }

  ToolKind kind() const noexcept override { return ToolKind::Eraser; }
  std::string_view displayName() const noexcept override { return "Eraser"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;

private:
  void eraseStroke(Layer& layer, const Point& from, const Point& to) const;
  void eraseCircle(PixelBuffer& buffer, const Point& center, int radius) const;

  int m_size {8};
  bool m_erasing {false};
  Point m_lastPoint {0, 0};
};

} // namespace core
