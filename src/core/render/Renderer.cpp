#include "core/render/Renderer.h"

#include <algorithm>
#include <cmath>

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

} // namespace

PixelBuffer Renderer::composite(const Document& document) const {
  const Size size = document.canvasSize();
  PixelBuffer output(size.width, size.height, Color::Transparent());

  for (std::size_t layerIndex = 0; layerIndex < document.layerCount(); ++layerIndex) {
    const Layer& layer = document.layerAt(layerIndex);
    if (!layer.visible() || layer.opacity() <= 0.0F) {
      continue;
    }

    for (int y = 0; y < size.height; ++y) {
      for (int x = 0; x < size.width; ++x) {
        const Color dst = output.pixel(x, y);
        const Color src = layer.buffer().pixel(x, y);
        output.setPixel(x, y, blendOver(dst, src, layer.opacity()));
      }
    }
  }

  return output;
}

} // namespace core
