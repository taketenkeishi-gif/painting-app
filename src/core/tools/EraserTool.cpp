#include "core/tools/EraserTool.h"

#include <algorithm>
#include <cmath>

namespace core {

ToolResult EraserTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() != LayerKind::Raster) {
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
  if (active == nullptr || active->kind() != LayerKind::Raster) {
    m_erasing = false;
    return {};
  }

  const Point stabilizedPoint = applyStabilization(m_lastPoint, event.point);
  eraseStroke(*active, m_lastPoint, stabilizedPoint);
  m_lastPoint = stabilizedPoint;
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
  if (active == nullptr || active->kind() != LayerKind::Raster) {
    return {};
  }

  const Point stabilizedPoint = applyStabilization(m_lastPoint, event.point);
  if (m_lastPoint.x == stabilizedPoint.x && m_lastPoint.y == stabilizedPoint.y &&
      (!m_postCorrection ||
       (stabilizedPoint.x == event.point.x && stabilizedPoint.y == event.point.y))) {
    return {};
  }

  eraseStroke(*active, m_lastPoint, stabilizedPoint);
  if (m_postCorrection &&
      (stabilizedPoint.x != event.point.x || stabilizedPoint.y != event.point.y)) {
    eraseStroke(*active, stabilizedPoint, event.point);
    m_lastPoint = event.point;
  } else {
    m_lastPoint = stabilizedPoint;
  }
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

Point EraserTool::applyStabilization(const Point& from, const Point& to) const {
  const float stabilization = std::clamp(m_stabilization, 0.0F, 1.0F);
  if (stabilization <= 0.001F) {
    return to;
  }

  float response = 1.0F - stabilization * 0.85F;
  if (m_velocityBasedCorrection) {
    const float distance = std::hypot(static_cast<float>(to.x - from.x), static_cast<float>(to.y - from.y));
    const float velocityFactor = std::clamp(1.0F - distance / 80.0F, 0.25F, 1.0F);
    response *= velocityFactor;
  }
  response = std::clamp(response, 0.05F, 1.0F);
  return Point {
      static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(to.x - from.x) * response)),
      static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(to.y - from.y) * response))};
}

void EraserTool::eraseStroke(Layer& layer, const Point& from, const Point& to) const {
  PixelBuffer& buffer = layer.buffer();
  const int radius = std::max(1, m_size) / 2;
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  const float distance = std::hypot(static_cast<float>(dx), static_cast<float>(dy));
  const float spacingPixels = std::max(1.0F, m_spacing * static_cast<float>(std::max(1, m_size)));
  const int steps = std::max(1, static_cast<int>(std::ceil(distance / spacingPixels)));

  if (distance <= 0.001F) {
    if (m_shapeType == BrushShapeType::Square) {
      eraseSquare(buffer, from, radius);
    } else {
      eraseCircle(buffer, from, radius);
    }
    return;
  }

  for (int i = 0; i <= steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const Point p {
        static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(dx) * t)),
        static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(dy) * t))};
    if (m_shapeType == BrushShapeType::Square) {
      eraseSquare(buffer, p, radius);
    } else {
      eraseCircle(buffer, p, radius);
    }
  }
}

void EraserTool::eraseCircle(PixelBuffer& buffer, const Point& center, int radius) const {
  const int r2 = radius * radius;
  const float radiusF = static_cast<float>(std::max(1, radius));
  const float hardEdge = std::clamp(m_hardness, 0.0F, 1.0F);
  for (int y = center.y - radius; y <= center.y + radius; ++y) {
    for (int x = center.x - radius; x <= center.x + radius; ++x) {
      const int dx = x - center.x;
      const int dy = y - center.y;
      if ((dx * dx + dy * dy) <= r2) {
        const float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy)) / radiusF;
        if (distance > 1.0F) {
          continue;
        }
        float strength = 1.0F;
        if (m_antiAlias) {
          if (hardEdge < 0.999F && distance > hardEdge) {
            strength = 1.0F - (distance - hardEdge) / (1.0F - hardEdge);
          }
        } else {
          strength = distance <= hardEdge ? 1.0F : 0.0F;
        }
        strength *= std::clamp(m_opacity * m_flow, 0.0F, 1.0F);
        if (strength > 0.001F) {
          erasePixel(buffer, x, y, strength);
        }
      }
    }
  }
}

void EraserTool::eraseSquare(PixelBuffer& buffer, const Point& center, int radius) const {
  const float radiusF = static_cast<float>(std::max(1, radius));
  const float hardEdge = std::clamp(m_hardness, 0.0F, 1.0F);
  for (int y = center.y - radius; y <= center.y + radius; ++y) {
    for (int x = center.x - radius; x <= center.x + radius; ++x) {
      const int dx = std::abs(x - center.x);
      const int dy = std::abs(y - center.y);
      const float distance = static_cast<float>(std::max(dx, dy)) / radiusF;
      if (distance > 1.0F) {
        continue;
      }
      float strength = 1.0F;
      if (m_antiAlias) {
        if (hardEdge < 0.999F && distance > hardEdge) {
          strength = 1.0F - (distance - hardEdge) / (1.0F - hardEdge);
        }
      } else {
        strength = distance <= hardEdge ? 1.0F : 0.0F;
      }
      strength *= std::clamp(m_opacity * m_flow, 0.0F, 1.0F);
      if (strength > 0.001F) {
        erasePixel(buffer, x, y, strength);
      }
    }
  }
}

void EraserTool::erasePixel(PixelBuffer& buffer, int x, int y, float strength) const {
  if (!buffer.inBounds(x, y)) {
    return;
  }

  const float s = std::clamp(strength, 0.0F, 1.0F);
  const Color dst = buffer.pixel(x, y);
  const float keep = 1.0F - s;
  buffer.setPixel(
      x,
      y,
      Color {
          static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.r) * keep)),
          static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.g) * keep)),
          static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.b) * keep)),
          static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.a) * keep))});
}

} // namespace core
