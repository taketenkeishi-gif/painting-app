#include "core/tools/ToolManager.h"

#include <utility>

namespace core {

bool ToolManager::registerTool(std::unique_ptr<ITool> tool) {
  if (!tool) {
    return false;
  }

  const ToolKind kind = tool->kind();
  m_tools[kind] = std::move(tool);
  return true;
}

bool ToolManager::setActiveTool(ToolKind kind) noexcept {
  if (!hasTool(kind)) {
    return false;
  }
  m_activeToolKind = kind;
  return true;
}

bool ToolManager::hasTool(ToolKind kind) const noexcept {
  return m_tools.find(kind) != m_tools.end();
}

ITool* ToolManager::activeTool() noexcept {
  const auto it = m_tools.find(m_activeToolKind);
  return it == m_tools.end() ? nullptr : it->second.get();
}

const ITool* ToolManager::activeTool() const noexcept {
  const auto it = m_tools.find(m_activeToolKind);
  return it == m_tools.end() ? nullptr : it->second.get();
}

ToolResult ToolManager::pointerPress(ToolContext& context, const ToolPointerEvent& event) {
  ITool* tool = activeTool();
  return tool == nullptr ? ToolResult {} : tool->onPointerPress(context, event);
}

ToolResult ToolManager::pointerMove(ToolContext& context, const ToolPointerEvent& event) {
  ITool* tool = activeTool();
  return tool == nullptr ? ToolResult {} : tool->onPointerMove(context, event);
}

ToolResult ToolManager::pointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  ITool* tool = activeTool();
  return tool == nullptr ? ToolResult {} : tool->onPointerRelease(context, event);
}

ToolResult ToolManager::cancel(ToolContext& context) {
  ITool* tool = activeTool();
  return tool == nullptr ? ToolResult {} : tool->onCancel(context);
}

ToolResult ToolManager::wheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  ITool* tool = activeTool();
  return tool == nullptr ? ToolResult {} : tool->onWheel(context, deltaSteps, event);
}

ToolOverlayState ToolManager::overlay() const {
  const ITool* tool = activeTool();
  return tool == nullptr ? ToolOverlayState {} : tool->overlay();
}

} // namespace core
