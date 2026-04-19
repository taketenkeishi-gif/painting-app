#pragma once

#include <string_view>

#include "core/common/Point.h"
#include "core/tools/ITool.h"

namespace core {

class RectSelectionTool : public ITool {
public:
  ToolKind kind() const noexcept override { return ToolKind::RectSelection; }
  std::string_view displayName() const noexcept override { return "Rect Selection"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;
  ToolOverlayState overlay() const override;

private:
  static Rect normalizeRect(const Point& a, const Point& b);

  bool m_selecting {false};
  Point m_start {0, 0};
  Point m_current {0, 0};
};

} // namespace core
