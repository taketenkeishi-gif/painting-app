#include "core/render/Renderer.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace core {

namespace {

Color blendOver(const Color& dst, const Color& src, float layerOpacity) {
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

  const float outR = (srcR * srcA + dstR * dstA * (1.0F - srcA)) / outA;
  const float outG = (srcG * srcA + dstG * dstA * (1.0F - srcA)) / outA;
  const float outB = (srcB * srcA + dstB * dstA * (1.0F - srcA)) / outA;

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
  target.setPixel(x, y, blendOver(dst, src, 1.0F));
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
    if (path.points.empty()) {
      continue;
    }
    Color color = path.color;
    color.a = static_cast<std::uint8_t>(std::lround(std::clamp(path.opacity, 0.0F, 1.0F) * static_cast<float>(color.a)));
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

} // namespace

PixelBuffer Renderer::composite(const Document& document) const {
  const Size size = document.canvasSize();
  PixelBuffer output(size.width, size.height, Color::Transparent());

  for (std::size_t layerIndex = 0; layerIndex < document.layerCount(); ++layerIndex) {
    const Layer& layer = document.layerAt(layerIndex);
    if (!layer.visible() || layer.opacity() <= 0.0F || layer.kind() == LayerKind::Folder) {
      continue;
    }

    PixelBuffer vectorRaster;
    const PixelBuffer* sourceBuffer = &layer.buffer();
    if (layer.kind() == LayerKind::Vector) {
      vectorRaster.resize(size.width, size.height, Color::Transparent());
      rasterizeVectorLayer(layer, vectorRaster);
      sourceBuffer = &vectorRaster;
    }

    for (int y = 0; y < size.height; ++y) {
      for (int x = 0; x < size.width; ++x) {
        const Color dst = output.pixel(x, y);
        Color src = sourceBuffer->pixel(x, y);
        src = applyLayerMask(layer, x, y, src);
        if (layer.clippedToBelow() && dst.a == 0) {
          continue;
        }
        output.setPixel(x, y, blendOver(dst, src, layer.opacity()));
      }
    }
  }

  return output;
}

} // namespace core
