#include "core/tools/HandTool.h"

namespace core {

ToolResult HandTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  m_dragging = true;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult HandTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  if (!m_dragging) {
    return {};
  }
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult HandTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  if (!m_dragging) {
    return {};
  }
  m_dragging = false;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult HandTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  if (!m_dragging) {
    return {};
  }
  m_dragging = false;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult HandTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

} // namespace core
