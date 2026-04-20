#include "core/tools/LineTool.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace core {

ToolResult LineTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  m_drawing = true;
  m_start = event.point;
  m_current = event.point;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult LineTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  if (!m_drawing) {
    return {};
  }
  m_current = snappedPoint(m_start, event.point);
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult LineTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_drawing) {
    return {};
  }
  m_drawing = false;
  m_current = snappedPoint(m_start, event.point);

  Layer* active = context.document.activeLayer();
  if (active == nullptr) {
    return {};
  }

  if (active->kind() == LayerKind::Vector) {
    addVectorLine(*active, m_start, m_current, context.currentColor, context.brushSize);
  } else {
    drawLine(*active, m_start, m_current, context.currentColor, context.brushSize);
  }
  ToolResult result;
  result.pixelsChanged = true;
  result.viewportChanged = true;
  return result;
}

Point LineTool::snappedPoint(const Point& start, const Point& rawEnd) const {
  if (m_snapAngleDegrees <= 0 || m_snapAngleDegrees >= 180) {
    return rawEnd;
  }
  const float dx = static_cast<float>(rawEnd.x - start.x);
  const float dy = static_cast<float>(rawEnd.y - start.y);
  if (std::abs(dx) < 0.001F && std::abs(dy) < 0.001F) {
    return rawEnd;
  }
  const float distance = std::hypot(dx, dy);
  const float angle = std::atan2(dy, dx);
  constexpr float kPi = 3.14159265358979323846F;
  const float step = static_cast<float>(m_snapAngleDegrees) * kPi / 180.0F;
  const float snapped = std::round(angle / step) * step;
  return Point {
      static_cast<int>(std::lround(static_cast<float>(start.x) + std::cos(snapped) * distance)),
      static_cast<int>(std::lround(static_cast<float>(start.y) + std::sin(snapped) * distance))};
}

ToolResult LineTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  if (!m_drawing) {
    return {};
  }
  m_drawing = false;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult LineTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

ToolOverlayState LineTool::overlay() const {
  ToolOverlayState state;
  if (!m_drawing) {
    return state;
  }
  state.hasLine = true;
  state.lineStart = m_start;
  state.lineEnd = m_current;
  return state;
}

void LineTool::drawLine(Layer& layer, const Point& from, const Point& to, const Color& color, int size) const {
  PixelBuffer& buffer = layer.buffer();
  const int radius = std::max(1, size) / 2;
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  const int steps = std::max(std::abs(dx), std::abs(dy));

  if (steps == 0) {
    stampCircle(buffer, from, radius, color);
    return;
  }

  for (int i = 0; i <= steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const Point p {
        static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(dx) * t)),
        static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(dy) * t))};
    stampCircle(buffer, p, radius, color);
  }
}

void LineTool::addVectorLine(Layer& layer, const Point& from, const Point& to, const Color& color, int size) const {
  VectorPath path;
  path.points.push_back(from);
  path.points.push_back(to);
  path.color = color;
  path.width = std::max(1, size);
  path.opacity = static_cast<float>(color.a) / 255.0F;
  layer.addVectorPath(std::move(path));
}

void LineTool::stampCircle(PixelBuffer& buffer, const Point& center, int radius, const Color& color) const {
  const int r2 = radius * radius;
  for (int y = center.y - radius; y <= center.y + radius; ++y) {
    for (int x = center.x - radius; x <= center.x + radius; ++x) {
      const int dx = x - center.x;
      const int dy = y - center.y;
      if ((dx * dx + dy * dy) <= r2) {
        blendPixel(buffer, x, y, color);
      }
    }
  }
}

void LineTool::blendPixel(PixelBuffer& buffer, int x, int y, const Color& src) const {
  if (!buffer.inBounds(x, y)) {
    return;
  }

  const Color dst = buffer.pixel(x, y);
  const float srcA = static_cast<float>(src.a) / 255.0F;
  const float dstA = static_cast<float>(dst.a) / 255.0F;
  const float outA = srcA + dstA * (1.0F - srcA);
  if (outA <= 0.0F) {
    buffer.setPixel(x, y, Color::Transparent());
    return;
  }

  const float srcR = static_cast<float>(src.r) / 255.0F;
  const float srcG = static_cast<float>(src.g) / 255.0F;
  const float srcB = static_cast<float>(src.b) / 255.0F;
  const float dstR = static_cast<float>(dst.r) / 255.0F;
  const float dstG = static_cast<float>(dst.g) / 255.0F;
  const float dstB = static_cast<float>(dst.b) / 255.0F;

  const float outR = (srcR * srcA + dstR * dstA * (1.0F - srcA)) / outA;
  const float outG = (srcG * srcA + dstG * dstA * (1.0F - srcA)) / outA;
  const float outB = (srcB * srcA + dstB * dstA * (1.0F - srcA)) / outA;

  buffer.setPixel(
      x,
      y,
      Color {
          static_cast<std::uint8_t>(std::lround(std::clamp(outR, 0.0F, 1.0F) * 255.0F)),
          static_cast<std::uint8_t>(std::lround(std::clamp(outG, 0.0F, 1.0F) * 255.0F)),
          static_cast<std::uint8_t>(std::lround(std::clamp(outB, 0.0F, 1.0F) * 255.0F)),
          static_cast<std::uint8_t>(std::lround(std::clamp(outA, 0.0F, 1.0F) * 255.0F))});
}

} // namespace core
