#include "core/tools/ZoomTool.h"

namespace core {

ToolResult ZoomTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  return {};
}

ToolResult ZoomTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  return {};
}

ToolResult ZoomTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  return {};
}

ToolResult ZoomTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  return {};
}

ToolResult ZoomTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  if (deltaSteps == 0) {
    return {};
  }
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

} // namespace core
