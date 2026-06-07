#include "core/tools/ShapeTool.h"

namespace core {

ShapeTool::ShapeTool()
    : m_lineTool(std::make_unique<LineTool>()),
      m_curveTool(std::make_unique<CurveTool>()) {
}

ITool* ShapeTool::activeTool() const noexcept {
  if (m_mode == ShapeMode::Line) {
    return m_lineTool.get();
  } else {
    return m_curveTool.get();
  }
}

void ShapeTool::setSnapAngleDegrees(int snapAngleDegrees) noexcept {
  if (m_lineTool) {
    m_lineTool->setSnapAngleDegrees(snapAngleDegrees);
  }
  if (m_curveTool) {
    m_curveTool->setSnapAngleDegrees(snapAngleDegrees);
  }
}

void ShapeTool::setSimplifyLevel(int level) noexcept {
  if (m_curveTool) {
    m_curveTool->setSimplifyLevel(level);
  }
}

ToolResult ShapeTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  ITool* tool = activeTool();
  return tool != nullptr ? tool->onPointerPress(context, event) : ToolResult {};
}

ToolResult ShapeTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  ITool* tool = activeTool();
  return tool != nullptr ? tool->onPointerMove(context, event) : ToolResult {};
}

ToolResult ShapeTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  ITool* tool = activeTool();
  return tool != nullptr ? tool->onPointerRelease(context, event) : ToolResult {};
}

ToolResult ShapeTool::onCancel(ToolContext& context) {
  ITool* tool = activeTool();
  return tool != nullptr ? tool->onCancel(context) : ToolResult {};
}

ToolResult ShapeTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  ITool* tool = activeTool();
  return tool != nullptr ? tool->onWheel(context, deltaSteps, event) : ToolResult {};
}

ToolOverlayState ShapeTool::overlay() const {
  const ITool* tool = activeTool();
  return tool != nullptr ? tool->overlay() : ToolOverlayState {};
}

} // namespace core
