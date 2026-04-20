#include "core/tools/FillTool.h"

#include <vector>

#include "core/selection/SelectionMask.h"

namespace core {

bool FillTool::isSameColor(const Color& a, const Color& b) noexcept {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

ToolResult FillTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() != LayerKind::Raster) {
    return {};
  }

  PixelBuffer& buffer = active->buffer();
  if (!buffer.inBounds(event.point.x, event.point.y)) {
    return {};
  }
  const SelectionMask& selection = context.document.selection();
  const bool hasSelection = selection.hasSelection();
  if (hasSelection && !selection.contains(event.point.x, event.point.y)) {
    return {};
  }

  const Color target = buffer.pixel(event.point.x, event.point.y);
  const Color replacement = context.currentColor;
  if (isSameColor(target, replacement)) {
    return {};
  }

  const int width = buffer.width();
  const int height = buffer.height();
  std::vector<Point> stack;
  stack.reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) / 8 + 1);
  stack.push_back(event.point);

  while (!stack.empty()) {
    const Point p = stack.back();
    stack.pop_back();

    if (!buffer.inBounds(p.x, p.y)) {
      continue;
    }
    if (!isSameColor(buffer.pixel(p.x, p.y), target)) {
      continue;
    }
    if (hasSelection && !selection.contains(p.x, p.y)) {
      continue;
    }

    buffer.setPixel(p.x, p.y, replacement);

    stack.push_back(Point {p.x + 1, p.y});
    stack.push_back(Point {p.x - 1, p.y});
    stack.push_back(Point {p.x, p.y + 1});
    stack.push_back(Point {p.x, p.y - 1});
  }

  ToolResult result;
  result.pixelsChanged = true;
  return result;
}

ToolResult FillTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  return {};
}

ToolResult FillTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  return {};
}

ToolResult FillTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  return {};
}

ToolResult FillTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

} // namespace core
