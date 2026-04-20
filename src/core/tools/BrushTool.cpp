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

  const Point stabilizedPoint = applyStabilization(m_lastPoint, event.point);
  stroke(*active, m_lastPoint, stabilizedPoint);
  m_lastPoint = stabilizedPoint;
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

  const Point stabilizedPoint = applyStabilization(m_lastPoint, event.point);
  if (m_lastPoint.x == stabilizedPoint.x && m_lastPoint.y == stabilizedPoint.y &&
      (!m_settings.postCorrection ||
       (stabilizedPoint.x == event.point.x && stabilizedPoint.y == event.point.y))) {
    return {};
  }

  stroke(*active, m_lastPoint, stabilizedPoint);
  if (m_settings.postCorrection &&
      (stabilizedPoint.x != event.point.x || stabilizedPoint.y != event.point.y)) {
    stroke(*active, stabilizedPoint, event.point);
    m_lastPoint = event.point;
  } else {
    m_lastPoint = stabilizedPoint;
  }
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

Point BrushTool::applyStabilization(const Point& from, const Point& to) const {
  const float stabilization = std::clamp(m_settings.stabilization, 0.0F, 1.0F);
  if (stabilization <= 0.001F) {
    return to;
  }

  float response = 1.0F - stabilization * 0.85F;
  if (m_settings.velocityBasedCorrection) {
    const float distance = std::hypot(static_cast<float>(to.x - from.x), static_cast<float>(to.y - from.y));
    const float velocityFactor = std::clamp(1.0F - distance / 80.0F, 0.25F, 1.0F);
    response *= velocityFactor;
  }
  response = std::clamp(response, 0.05F, 1.0F);

  return Point {
      static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(to.x - from.x) * response)),
      static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(to.y - from.y) * response))};
}

void BrushTool::stroke(Layer& layer, const Point& from, const Point& to) const {
  PixelBuffer& buffer = layer.buffer();
  const int radius = std::max(1, m_settings.size) / 2;
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  const float distance = std::hypot(static_cast<float>(dx), static_cast<float>(dy));
  const float spacingPixels = std::max(1.0F, m_settings.spacing * static_cast<float>(std::max(1, m_settings.size)));
  const int steps = std::max(1, static_cast<int>(std::ceil(distance / spacingPixels)));

  if (distance <= 0.001F) {
    if (m_settings.shapeType == BrushShapeType::Square) {
      stampSquare(buffer, from, radius, m_settings.color);
    } else {
      stampCircle(buffer, from, radius, m_settings.color);
    }
    return;
  }

  for (int i = 0; i <= steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const Point p {
        static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(dx) * t)),
        static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(dy) * t))};
    if (m_settings.shapeType == BrushShapeType::Square) {
      stampSquare(buffer, p, radius, m_settings.color);
    } else {
      stampCircle(buffer, p, radius, m_settings.color);
    }
  }
}

void BrushTool::stampCircle(PixelBuffer& buffer, const Point& center, int radius, const Color& color) const {
  const int r2 = radius * radius;
  const float radiusF = static_cast<float>(std::max(1, radius));
  const float hardEdge = std::clamp(m_settings.hardness, 0.0F, 1.0F);
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
        if (m_settings.antiAlias) {
          if (hardEdge < 0.999F && distance > hardEdge) {
            strength = 1.0F - (distance - hardEdge) / (1.0F - hardEdge);
          }
        } else {
          strength = distance <= hardEdge ? 1.0F : 0.0F;
        }
        strength *= std::clamp(m_settings.opacity * m_settings.flow, 0.0F, 1.0F);
        if (strength > 0.001F) {
          blendPixel(buffer, x, y, color, strength);
        }
      }
    }
  }
}

void BrushTool::stampSquare(PixelBuffer& buffer, const Point& center, int radius, const Color& color) const {
  const float radiusF = static_cast<float>(std::max(1, radius));
  const float hardEdge = std::clamp(m_settings.hardness, 0.0F, 1.0F);
  for (int y = center.y - radius; y <= center.y + radius; ++y) {
    for (int x = center.x - radius; x <= center.x + radius; ++x) {
      const int dx = std::abs(x - center.x);
      const int dy = std::abs(y - center.y);
      const float distance = static_cast<float>(std::max(dx, dy)) / radiusF;
      if (distance > 1.0F) {
        continue;
      }

      float strength = 1.0F;
      if (m_settings.antiAlias) {
        if (hardEdge < 0.999F && distance > hardEdge) {
          strength = 1.0F - (distance - hardEdge) / (1.0F - hardEdge);
        }
      } else {
        strength = distance <= hardEdge ? 1.0F : 0.0F;
      }

      strength *= std::clamp(m_settings.opacity * m_settings.flow, 0.0F, 1.0F);
      if (strength > 0.001F) {
        blendPixel(buffer, x, y, color, strength);
      }
    }
  }
}

void BrushTool::blendPixel(PixelBuffer& buffer, int x, int y, const Color& src, float strength) const {
  if (!buffer.inBounds(x, y)) {
    return;
  }

  const Color dst = buffer.pixel(x, y);
  if (m_settings.lockAlphaRespect && dst.a == 0) {
    return;
  }

  const float alphaScale = std::clamp(strength, 0.0F, 1.0F);
  if (m_settings.eraseMode) {
    const float eraseStrength = std::clamp((static_cast<float>(src.a) / 255.0F) * alphaScale, 0.0F, 1.0F);
    const float keep = 1.0F - eraseStrength;
    buffer.setPixel(
        x,
        y,
        Color {
            static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.r) * keep)),
            static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.g) * keep)),
            static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.b) * keep)),
            static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.a) * keep))});
    return;
  }

  const Color effectiveSrc {
      src.r,
      src.g,
      src.b,
      static_cast<std::uint8_t>(std::lround(static_cast<float>(src.a) * alphaScale))};
  if (effectiveSrc.a == 0) {
    return;
  }

  const float srcA = static_cast<float>(effectiveSrc.a) / 255.0F;
  const float dstA = static_cast<float>(dst.a) / 255.0F;
  const float outA = srcA + dstA * (1.0F - srcA);
  if (outA <= 0.0F) {
    buffer.setPixel(x, y, Color::Transparent());
    return;
  }

  const float srcR = static_cast<float>(effectiveSrc.r) / 255.0F;
  const float srcG = static_cast<float>(effectiveSrc.g) / 255.0F;
  const float srcB = static_cast<float>(effectiveSrc.b) / 255.0F;
  const float dstR = static_cast<float>(dst.r) / 255.0F;
  const float dstG = static_cast<float>(dst.g) / 255.0F;
  const float dstB = static_cast<float>(dst.b) / 255.0F;

  float outR = 0.0F;
  float outG = 0.0F;
  float outB = 0.0F;
  switch (m_settings.blendMode) {
    case BlendMode::Multiply: {
      const float mulR = dstR * srcR;
      const float mulG = dstG * srcG;
      const float mulB = dstB * srcB;
      outR = (mulR * srcA + dstR * dstA * (1.0F - srcA)) / outA;
      outG = (mulG * srcA + dstG * dstA * (1.0F - srcA)) / outA;
      outB = (mulB * srcA + dstB * dstA * (1.0F - srcA)) / outA;
      break;
    }
    case BlendMode::Add: {
      const float premulR = std::clamp(dstR * dstA + srcR * srcA, 0.0F, 1.0F);
      const float premulG = std::clamp(dstG * dstA + srcG * srcA, 0.0F, 1.0F);
      const float premulB = std::clamp(dstB * dstA + srcB * srcA, 0.0F, 1.0F);
      outR = premulR / outA;
      outG = premulG / outA;
      outB = premulB / outA;
      break;
    }
    case BlendMode::Normal:
    default:
      outR = (srcR * srcA + dstR * dstA * (1.0F - srcA)) / outA;
      outG = (srcG * srcA + dstG * dstA * (1.0F - srcA)) / outA;
      outB = (srcB * srcA + dstB * dstA * (1.0F - srcA)) / outA;
      break;
  }

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
