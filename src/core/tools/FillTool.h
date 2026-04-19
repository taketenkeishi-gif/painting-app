#pragma once

#include <string_view>

#include "core/tools/ITool.h"

namespace core {

class FillTool : public ITool {
public:
  ToolKind kind() const noexcept override { return ToolKind::Fill; }
  std::string_view displayName() const noexcept override { return "Fill"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;

private:
  static bool isSameColor(const Color& a, const Color& b) noexcept;
};

} // namespace core
