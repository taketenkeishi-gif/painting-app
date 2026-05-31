#include "core/tools/EyedropperTool.h"

namespace core {

ToolResult EyedropperTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  if (!context.composited.inBounds(event.point.x, event.point.y)) {
    return {};
  }
  ToolResult result;
  result.sampledColor = context.composited.pixel(event.point.x, event.point.y);
  return result;
}

ToolResult EyedropperTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  return {};
}

ToolResult EyedropperTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  return {};
}

ToolResult EyedropperTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  return {};
}

ToolResult EyedropperTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

} // namespace core
