#include "core/tools/EraserTool.h"

#include <algorithm>
#include <cmath>

namespace core {

ToolResult EraserTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  Layer* active = context.document.activeLayer();
  if (active == nullptr) {
    return {};
  }

  m_erasing = true;
  m_lastPoint = event.point;
  eraseStroke(*active, m_lastPoint, m_lastPoint);
  ToolResult result;
  result.pixelsChanged = true;
  return result;
}

ToolResult EraserTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_erasing) {
    return {};
  }

  Layer* active = context.document.activeLayer();
  if (active == nullptr) {
    m_erasing = false;
    return {};
  }

  eraseStroke(*active, m_lastPoint, event.point);
  m_lastPoint = event.point;
  ToolResult result;
  result.pixelsChanged = true;
  return result;
}

ToolResult EraserTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_erasing) {
    return {};
  }

  m_erasing = false;
  Layer* active = context.document.activeLayer();
  if (active == nullptr) {
    return {};
  }

  if (m_lastPoint.x == event.point.x && m_lastPoint.y == event.point.y) {
    return {};
  }

  eraseStroke(*active, m_lastPoint, event.point);
  ToolResult result;
  result.pixelsChanged = true;
  return result;
}

ToolResult EraserTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  m_erasing = false;
  return {};
}

ToolResult EraserTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

void EraserTool::eraseStroke(Layer& layer, const Point& from, const Point& to) const {
  PixelBuffer& buffer = layer.buffer();
  const int radius = std::max(1, m_size) / 2;
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  const int steps = std::max(std::abs(dx), std::abs(dy));

  if (steps == 0) {
    eraseCircle(buffer, from, radius);
    return;
  }

  for (int i = 0; i <= steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const Point p {
        static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(dx) * t)),
        static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(dy) * t))};
    eraseCircle(buffer, p, radius);
  }
}

void EraserTool::eraseCircle(PixelBuffer& buffer, const Point& center, int radius) const {
  const int r2 = radius * radius;
  for (int y = center.y - radius; y <= center.y + radius; ++y) {
    for (int x = center.x - radius; x <= center.x + radius; ++x) {
      const int dx = x - center.x;
      const int dy = y - center.y;
      if ((dx * dx + dy * dy) <= r2) {
        if (buffer.inBounds(x, y)) {
          buffer.setPixel(x, y, Color::Transparent());
        }
      }
    }
  }
}

} // namespace core
