#include "core/tools/RectSelectionTool.h"

#include <algorithm>

namespace core {

ToolResult RectSelectionTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  m_selecting = true;
  m_start = event.point;
  m_current = event.point;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult RectSelectionTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  if (!m_selecting) {
    return {};
  }
  m_current = event.point;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult RectSelectionTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  if (!m_selecting) {
    return {};
  }
  m_selecting = false;
  m_current = event.point;
  ToolResult result;
  result.selectionChanged = true;
  result.viewportChanged = true;
  return result;
}

ToolResult RectSelectionTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  if (!m_selecting) {
    return {};
  }
  m_selecting = false;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult RectSelectionTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

ToolOverlayState RectSelectionTool::overlay() const {
  ToolOverlayState state;
  if (!m_selecting) {
    return state;
  }
  state.hasRect = true;
  state.rect = normalizeRect(m_start, m_current);
  return state;
}

Rect RectSelectionTool::normalizeRect(const Point& a, const Point& b) {
  const int left = std::min(a.x, b.x);
  const int top = std::min(a.y, b.y);
  const int right = std::max(a.x, b.x);
  const int bottom = std::max(a.y, b.y);
  return Rect {left, top, right - left + 1, bottom - top + 1};
}

} // namespace core
