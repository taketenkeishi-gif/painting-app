#pragma once

#include <string_view>

#include "core/tools/ITool.h"

namespace core {

class HandTool : public ITool {
public:
  ToolKind kind() const noexcept override { return ToolKind::Hand; }
  std::string_view displayName() const noexcept override { return "Hand"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;

private:
  bool m_dragging {false};
};

} // namespace core
