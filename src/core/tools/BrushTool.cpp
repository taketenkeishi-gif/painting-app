#include "core/tools/BrushTool.h"

#include <algorithm>
#include <cmath>

namespace core {

ToolResult BrushTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  Layer* active = context.document.activeLayer();
  if (active == nullptr) {
    return {};
  }

  m_drawing = true;
  m_lastPoint = event.point;
  stroke(*active, m_lastPoint, m_lastPoint);
  ToolResult result;
  result.pixelsChanged = true;
  return result;
}

ToolResult BrushTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_drawing) {
    return {};
  }

  Layer* active = context.document.activeLayer();
  if (active == nullptr) {
    m_drawing = false;
    return {};
  }

  stroke(*active, m_lastPoint, event.point);
  m_lastPoint = event.point;
  ToolResult result;
  result.pixelsChanged = true;
  return result;
}

ToolResult BrushTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_drawing) {
    return {};
  }

  m_drawing = false;
  Layer* active = context.document.activeLayer();
  if (active == nullptr) {
    return {};
  }

  if (m_lastPoint.x == event.point.x && m_lastPoint.y == event.point.y) {
    return {};
  }

  stroke(*active, m_lastPoint, event.point);
  ToolResult result;
  result.pixelsChanged = true;
  return result;
}

ToolResult BrushTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  m_drawing = false;
  return {};
}

ToolResult BrushTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

void BrushTool::stroke(Layer& layer, const Point& from, const Point& to) const {
  PixelBuffer& buffer = layer.buffer();
  const int radius = std::max(1, m_settings.size) / 2;
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  const int steps = std::max(std::abs(dx), std::abs(dy));

  if (steps == 0) {
    stampCircle(buffer, from, radius, m_settings.color);
    return;
  }

  for (int i = 0; i <= steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const Point p {
        static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(dx) * t)),
        static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(dy) * t))};
    stampCircle(buffer, p, radius, m_settings.color);
  }
}

void BrushTool::stampCircle(PixelBuffer& buffer, const Point& center, int radius, const Color& color) const {
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

void BrushTool::blendPixel(PixelBuffer& buffer, int x, int y, const Color& src) const {
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
