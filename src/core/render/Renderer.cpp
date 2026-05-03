#include "core/render/Renderer.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace core {

namespace {

Color blendOver(const Color& dst, const Color& src, float layerOpacity, BlendMode blendMode) {
  const float srcA = (static_cast<float>(src.a) / 255.0F) * std::clamp(layerOpacity, 0.0F, 1.0F);
  const float dstA = static_cast<float>(dst.a) / 255.0F;
  const float outA = srcA + dstA * (1.0F - srcA);
  if (outA <= 0.0F) {
    return Color::Transparent();
  }

  const float srcR = static_cast<float>(src.r) / 255.0F;
  const float srcG = static_cast<float>(src.g) / 255.0F;
  const float srcB = static_cast<float>(src.b) / 255.0F;
  const float dstR = static_cast<float>(dst.r) / 255.0F;
  const float dstG = static_cast<float>(dst.g) / 255.0F;
  const float dstB = static_cast<float>(dst.b) / 255.0F;

  float outR = 0.0F;
  float outG = 0.0F;
  float outB = 0.0F;
  switch (blendMode) {
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

  return Color {
      static_cast<std::uint8_t>(std::round(std::clamp(outR, 0.0F, 1.0F) * 255.0F)),
      static_cast<std::uint8_t>(std::round(std::clamp(outG, 0.0F, 1.0F) * 255.0F)),
      static_cast<std::uint8_t>(std::round(std::clamp(outB, 0.0F, 1.0F) * 255.0F)),
      static_cast<std::uint8_t>(std::round(std::clamp(outA, 0.0F, 1.0F) * 255.0F))};
}

void blendPixel(PixelBuffer& target, int x, int y, const Color& src) {
  if (!target.inBounds(x, y)) {
    return;
  }
  const Color dst = target.pixel(x, y);
  target.setPixel(x, y, blendOver(dst, src, 1.0F, BlendMode::Normal));
}

void stampCircle(PixelBuffer& target, const Point& center, int radius, const Color& color) {
  const int r2 = radius * radius;
  for (int y = center.y - radius; y <= center.y + radius; ++y) {
    for (int x = center.x - radius; x <= center.x + radius; ++x) {
      const int dx = x - center.x;
      const int dy = y - center.y;
      if ((dx * dx + dy * dy) <= r2) {
        blendPixel(target, x, y, color);
      }
    }
  }
}

void drawSegment(PixelBuffer& target, const Point& from, const Point& to, const Color& color, int width) {
  const int radius = std::max(1, width) / 2;
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  const int steps = std::max(std::abs(dx), std::abs(dy));
  if (steps == 0) {
    stampCircle(target, from, radius, color);
    return;
  }
  for (int i = 0; i <= steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const Point p {
        static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(dx) * t)),
        static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(dy) * t))};
    stampCircle(target, p, radius, color);
  }
}

void rasterizeVectorLayer(const Layer& layer, PixelBuffer& out) {
  for (const VectorPath& path : layer.vectorPaths()) {
    if (path.kind == VectorPath::Kind::Ruler) {
      continue;
    }
    if (path.points.empty()) {
      continue;
    }
    Color color = path.color;
    color.a = static_cast<std::uint8_t>(std::lround(std::clamp(path.opacity, 0.0F, 1.0F) * static_cast<float>(color.a)));
    if (path.kind == VectorPath::Kind::Text) {
      const Point origin = path.points.front();
      const int charW = std::max(4, path.width);
      const int charH = std::max(8, path.width * 2);
      for (std::size_t i = 0; i < path.text.size(); ++i) {
        const int x = origin.x + static_cast<int>(i) * (charW + 2);
        const int y = origin.y;
        drawSegment(out, {x, y}, {x + charW, y}, color, std::max(1, path.width / 4));
        drawSegment(out, {x, y + charH}, {x + charW, y + charH}, color, std::max(1, path.width / 4));
        drawSegment(out, {x, y}, {x, y + charH}, color, std::max(1, path.width / 4));
        drawSegment(out, {x + charW, y}, {x + charW, y + charH}, color, std::max(1, path.width / 4));
      }
      continue;
    }
    if (path.points.size() == 1) {
      drawSegment(out, path.points.front(), path.points.front(), color, path.width);
      continue;
    }
    for (std::size_t i = 1; i < path.points.size(); ++i) {
      drawSegment(out, path.points[i - 1], path.points[i], color, path.width);
    }
  }
}

Color applyLayerMask(const Layer& layer, int x, int y, Color src) {
  if (!layer.hasMask() || !layer.maskEnabled()) {
    return src;
  }
  const Color mask = layer.maskBuffer().pixel(x, y);
  const float maskAlpha = static_cast<float>(mask.a) / 255.0F;
  src.a = static_cast<std::uint8_t>(std::lround(static_cast<float>(src.a) * std::clamp(maskAlpha, 0.0F, 1.0F)));
  return src;
}

Rect clampRectToCanvas(const Rect& rect, const Size& size) {
  const int x0 = std::clamp(rect.x, 0, size.width);
  const int y0 = std::clamp(rect.y, 0, size.height);
  const int x1 = std::clamp(rect.x + rect.width, 0, size.width);
  const int y1 = std::clamp(rect.y + rect.height, 0, size.height);
  return Rect {x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

} // namespace

PixelBuffer Renderer::composite(const Document& document) const {
  const Size size = document.canvasSize();
  PixelBuffer output(size.width, size.height, Color::Transparent());
  compositeInto(document, output, Rect {0, 0, size.width, size.height});
  return output;
}

void Renderer::compositeInto(const Document& document, PixelBuffer& target, const Rect& dirtyRect) const {
  const Size size = document.canvasSize();
  if (size.width <= 0 || size.height <= 0) {
    return;
  }
  if (target.width() != size.width || target.height() != size.height) {
    target.resize(size.width, size.height, Color::Transparent());
  }

  const Rect area = clampRectToCanvas(dirtyRect, size);
  if (area.width <= 0 || area.height <= 0) {
    return;
  }

  std::vector<PixelBuffer> vectorRasters;
  vectorRasters.resize(document.layerCount());
  for (std::size_t layerIndex = 0; layerIndex < document.layerCount(); ++layerIndex) {
    const Layer& layer = document.layerAt(layerIndex);
    const registry::LayerRenderRoute route = m_registry.routeFor(layer);
    if (route != registry::LayerRenderRoute::VectorPaths || !layer.visible() || layer.opacity() <= 0.0F) {
      continue;
    }
    vectorRasters[layerIndex].resize(size.width, size.height, Color::Transparent());
    rasterizeVectorLayer(layer, vectorRasters[layerIndex]);
  }

  for (int y = area.y; y < area.y + area.height; ++y) {
    for (int x = area.x; x < area.x + area.width; ++x) {
      Color composed = document.paperVisible() ? document.paperColor() : Color::Transparent();
      float belowAlpha = 0.0F;

      for (std::size_t layerIndex = 0; layerIndex < document.layerCount(); ++layerIndex) {
        const Layer& layer = document.layerAt(layerIndex);
        const registry::LayerRenderRoute route = m_registry.routeFor(layer);
        if (!layer.visible() || layer.opacity() <= 0.0F || route == registry::LayerRenderRoute::None || layer.isPaperLayer()) {
          continue;
        }

        const PixelBuffer* sourceBuffer = &layer.buffer();
        if (route == registry::LayerRenderRoute::VectorPaths) {
          sourceBuffer = &vectorRasters[layerIndex];
        }

        Color src = sourceBuffer->pixel(x, y);
        src = applyLayerMask(layer, x, y, src);
        if (layer.clippedToBelow() && belowAlpha <= 0.0001F) {
          continue;
        }
        composed = blendOver(composed, src, layer.opacity(), layer.blendMode());
        const float srcAlpha = (static_cast<float>(src.a) / 255.0F) * std::clamp(layer.opacity(), 0.0F, 1.0F);
        belowAlpha = srcAlpha + belowAlpha * (1.0F - srcAlpha);
      }

      target.setPixel(x, y, composed);
    }
  }
}

} // namespace core
