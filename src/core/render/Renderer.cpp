#include "core/render/Renderer.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/render/RenderUtils.h"

namespace core {

namespace {

// ---------------------------------------------------------------
// HSL/HSB ヘルパー（Hue/Saturation/Color/Luminosity ブレンドモード用）
// ---------------------------------------------------------------
struct RGB { float r, g, b; };

float luminance(RGB c) {
  return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
}

float saturation(RGB c) {
  return std::max({c.r, c.g, c.b}) - std::min({c.r, c.g, c.b});
}

RGB clipColor(RGB c) {
  const float l = luminance(c);
  const float mn = std::min({c.r, c.g, c.b});
  const float mx = std::max({c.r, c.g, c.b});
  if (mn < 0.0f) {
    c.r = l + ((c.r - l) * l) / (l - mn);
    c.g = l + ((c.g - l) * l) / (l - mn);
    c.b = l + ((c.b - l) * l) / (l - mn);
  }
  if (mx > 1.0f) {
    c.r = l + ((c.r - l) * (1.0f - l)) / (mx - l);
    c.g = l + ((c.g - l) * (1.0f - l)) / (mx - l);
    c.b = l + ((c.b - l) * (1.0f - l)) / (mx - l);
  }
  return c;
}

RGB setLuminance(RGB c, float lum) {
  const float d = lum - luminance(c);
  return clipColor({c.r + d, c.g + d, c.b + d});
}

RGB setSaturation(RGB c, float sat) {
  const float cmin = std::min({c.r, c.g, c.b});
  const float cmax = std::max({c.r, c.g, c.b});
  const float range = cmax - cmin;
  if (range <= 0.0f) {
    return {0.0f, 0.0f, 0.0f};
  }
  c.r = (c.r - cmin) * sat / range;
  c.g = (c.g - cmin) * sat / range;
  c.b = (c.b - cmin) * sat / range;
  return c;
}

// ---------------------------------------------------------------
// Photoshop互換ブレンド（2色）
// すべて [0,1] 正規化済みの float で動作
// ---------------------------------------------------------------
RGB blendColors(RGB dst, RGB src, BlendMode mode) {
  switch (mode) {
    case BlendMode::Multiply:
      return {dst.r*src.r, dst.g*src.g, dst.b*src.b};

    case BlendMode::LinearDodge:  // Add
      return {std::clamp(dst.r+src.r,0.0f,1.0f),
              std::clamp(dst.g+src.g,0.0f,1.0f),
              std::clamp(dst.b+src.b,0.0f,1.0f)};

    case BlendMode::Screen:
      return {1.0f-(1.0f-dst.r)*(1.0f-src.r),
              1.0f-(1.0f-dst.g)*(1.0f-src.g),
              1.0f-(1.0f-dst.b)*(1.0f-src.b)};

    case BlendMode::Overlay:
      return {dst.r<0.5f ? 2.0f*dst.r*src.r : 1.0f-2.0f*(1.0f-dst.r)*(1.0f-src.r),
              dst.g<0.5f ? 2.0f*dst.g*src.g : 1.0f-2.0f*(1.0f-dst.g)*(1.0f-src.g),
              dst.b<0.5f ? 2.0f*dst.b*src.b : 1.0f-2.0f*(1.0f-dst.b)*(1.0f-src.b)};

    case BlendMode::SoftLight: {
      auto sl = [](float d, float s) {
        if (s <= 0.5f) {
          return d - (1.0f-2.0f*s)*d*(1.0f-d);
        }
        float g = d <= 0.25f ? ((16.0f*d-12.0f)*d+4.0f)*d : std::sqrt(d);
        return d + (2.0f*s-1.0f)*(g-d);
      };
      return {sl(dst.r,src.r), sl(dst.g,src.g), sl(dst.b,src.b)};
    }

    case BlendMode::HardLight:
      return {src.r<0.5f ? 2.0f*dst.r*src.r : 1.0f-2.0f*(1.0f-dst.r)*(1.0f-src.r),
              src.g<0.5f ? 2.0f*dst.g*src.g : 1.0f-2.0f*(1.0f-dst.g)*(1.0f-src.g),
              src.b<0.5f ? 2.0f*dst.b*src.b : 1.0f-2.0f*(1.0f-dst.b)*(1.0f-src.b)};

    case BlendMode::ColorDodge:
      return {src.r>=1.0f ? 1.0f : std::clamp(dst.r/(1.0f-src.r),0.0f,1.0f),
              src.g>=1.0f ? 1.0f : std::clamp(dst.g/(1.0f-src.g),0.0f,1.0f),
              src.b>=1.0f ? 1.0f : std::clamp(dst.b/(1.0f-src.b),0.0f,1.0f)};

    case BlendMode::ColorBurn:
      return {src.r<=0.0f ? 0.0f : std::clamp(1.0f-(1.0f-dst.r)/src.r,0.0f,1.0f),
              src.g<=0.0f ? 0.0f : std::clamp(1.0f-(1.0f-dst.g)/src.g,0.0f,1.0f),
              src.b<=0.0f ? 0.0f : std::clamp(1.0f-(1.0f-dst.b)/src.b,0.0f,1.0f)};

    case BlendMode::LinearBurn:
      return {std::clamp(dst.r+src.r-1.0f,0.0f,1.0f),
              std::clamp(dst.g+src.g-1.0f,0.0f,1.0f),
              std::clamp(dst.b+src.b-1.0f,0.0f,1.0f)};

    case BlendMode::Darken:
      return {std::min(dst.r,src.r), std::min(dst.g,src.g), std::min(dst.b,src.b)};

    case BlendMode::Lighten:
      return {std::max(dst.r,src.r), std::max(dst.g,src.g), std::max(dst.b,src.b)};

    case BlendMode::DarkerColor:
      return luminance(dst) <= luminance(src) ? dst : src;

    case BlendMode::LighterColor:
      return luminance(dst) >= luminance(src) ? dst : src;

    case BlendMode::Difference:
      return {std::abs(dst.r-src.r), std::abs(dst.g-src.g), std::abs(dst.b-src.b)};

    case BlendMode::Exclusion:
      return {dst.r+src.r-2.0f*dst.r*src.r,
              dst.g+src.g-2.0f*dst.g*src.g,
              dst.b+src.b-2.0f*dst.b*src.b};

    case BlendMode::Subtract:
      return {std::clamp(dst.r-src.r,0.0f,1.0f),
              std::clamp(dst.g-src.g,0.0f,1.0f),
              std::clamp(dst.b-src.b,0.0f,1.0f)};

    case BlendMode::Divide:
      return {src.r<=0.0f ? 1.0f : std::clamp(dst.r/src.r,0.0f,1.0f),
              src.g<=0.0f ? 1.0f : std::clamp(dst.g/src.g,0.0f,1.0f),
              src.b<=0.0f ? 1.0f : std::clamp(dst.b/src.b,0.0f,1.0f)};

    case BlendMode::VividLight:
      return {src.r<0.5f?(src.r<=0.0f?0.0f:std::clamp(1.0f-(1.0f-dst.r)/(2.0f*src.r),0.0f,1.0f)):(src.r>=1.0f?1.0f:std::clamp(dst.r/(2.0f*(1.0f-src.r)),0.0f,1.0f)),
              src.g<0.5f?(src.g<=0.0f?0.0f:std::clamp(1.0f-(1.0f-dst.g)/(2.0f*src.g),0.0f,1.0f)):(src.g>=1.0f?1.0f:std::clamp(dst.g/(2.0f*(1.0f-src.g)),0.0f,1.0f)),
              src.b<0.5f?(src.b<=0.0f?0.0f:std::clamp(1.0f-(1.0f-dst.b)/(2.0f*src.b),0.0f,1.0f)):(src.b>=1.0f?1.0f:std::clamp(dst.b/(2.0f*(1.0f-src.b)),0.0f,1.0f))};

    case BlendMode::LinearLight:
      return {std::clamp(dst.r+2.0f*src.r-1.0f,0.0f,1.0f),
              std::clamp(dst.g+2.0f*src.g-1.0f,0.0f,1.0f),
              std::clamp(dst.b+2.0f*src.b-1.0f,0.0f,1.0f)};

    case BlendMode::PinLight:
      return {src.r<0.5f?std::min(dst.r,2.0f*src.r):std::max(dst.r,2.0f*src.r-1.0f),
              src.g<0.5f?std::min(dst.g,2.0f*src.g):std::max(dst.g,2.0f*src.g-1.0f),
              src.b<0.5f?std::min(dst.b,2.0f*src.b):std::max(dst.b,2.0f*src.b-1.0f)};

    case BlendMode::HardMix:
      return {(dst.r+src.r)>=1.0f?1.0f:0.0f,
              (dst.g+src.g)>=1.0f?1.0f:0.0f,
              (dst.b+src.b)>=1.0f?1.0f:0.0f};

    // HSL系
    case BlendMode::Hue:
      return setLuminance(setSaturation({src.r,src.g,src.b}, saturation({dst.r,dst.g,dst.b})),
                          luminance({dst.r,dst.g,dst.b}));

    case BlendMode::HslSat:
      return setLuminance(setSaturation({dst.r,dst.g,dst.b}, saturation({src.r,src.g,src.b})),
                          luminance({dst.r,dst.g,dst.b}));

    case BlendMode::HslColor:
      return setLuminance({src.r,src.g,src.b}, luminance({dst.r,dst.g,dst.b}));

    case BlendMode::Luminosity:
      return setLuminance({dst.r,dst.g,dst.b}, luminance({src.r,src.g,src.b}));

    case BlendMode::Normal:
    case BlendMode::Dissolve:
    default:
      return src;
  }
}

// ---------------------------------------------------------------
// 2色のアルファ合成（Porter-Duff over）
// ---------------------------------------------------------------
Color blendOver(const Color& dst, const Color& src, float layerOpacity, BlendMode blendMode) {
  const float srcA = (static_cast<float>(src.a) / 255.0f) * std::clamp(layerOpacity, 0.0f, 1.0f);
  const float dstA = static_cast<float>(dst.a) / 255.0f;
  const float outA = srcA + dstA * (1.0f - srcA);
  if (outA <= 0.0f) {
    return Color::Transparent();
  }

  const RGB srcRGB {
      static_cast<float>(src.r) / 255.0f,
      static_cast<float>(src.g) / 255.0f,
      static_cast<float>(src.b) / 255.0f};
  const RGB dstRGB {
      static_cast<float>(dst.r) / 255.0f,
      static_cast<float>(dst.g) / 255.0f,
      static_cast<float>(dst.b) / 255.0f};

  const RGB blended = blendColors(dstRGB, srcRGB, blendMode);

  const float outR = (blended.r * srcA + dstRGB.r * dstA * (1.0f - srcA)) / outA;
  const float outG = (blended.g * srcA + dstRGB.g * dstA * (1.0f - srcA)) / outA;
  const float outB = (blended.b * srcA + dstRGB.b * dstA * (1.0f - srcA)) / outA;

  return Color {
      static_cast<std::uint8_t>(std::lround(std::clamp(outR, 0.0f, 1.0f) * 255.0f)),
      static_cast<std::uint8_t>(std::lround(std::clamp(outG, 0.0f, 1.0f) * 255.0f)),
      static_cast<std::uint8_t>(std::lround(std::clamp(outB, 0.0f, 1.0f) * 255.0f)),
      static_cast<std::uint8_t>(std::lround(std::clamp(outA, 0.0f, 1.0f) * 255.0f))};
}

void blendPixel(PixelBuffer& target, int x, int y, const Color& src) {
  if (!target.inBounds(x, y)) {
    return;
  }
  const Color dst = target.pixel(x, y);
  target.setPixel(x, y, blendOver(dst, src, 1.0f, BlendMode::Normal));
}

// AA-capable circle stamp: float center, 1-pixel soft fringe
void stampCircleAA(PixelBuffer& target, float cx, float cy, float radius, const Color& color) {
  if (radius <= 0.0f || color.a == 0) {
    return;
  }
  const int x0 = static_cast<int>(std::floor(cx - radius - 1.0f));
  const int y0 = static_cast<int>(std::floor(cy - radius - 1.0f));
  const int x1 = static_cast<int>(std::ceil(cx + radius + 1.0f));
  const int y1 = static_cast<int>(std::ceil(cy + radius + 1.0f));

  for (int py = y0; py <= y1; ++py) {
    for (int px = x0; px <= x1; ++px) {
      const float dx = (static_cast<float>(px) + 0.5f) - cx;
      const float dy = (static_cast<float>(py) + 0.5f) - cy;
      const float dist = std::sqrt(dx * dx + dy * dy) / radius;
      const float coverage = brushCoverage(dist, 1.0f, radius, true);
      if (coverage <= 0.001f) {
        continue;
      }
      blendPixel(target, px, py,
          Color {color.r, color.g, color.b,
              static_cast<std::uint8_t>(std::lround(static_cast<float>(color.a) * coverage))});
    }
  }
}

// Float-precision segment: stamps are spaced at half-radius intervals for smooth fill
void drawSegmentAA(PixelBuffer& target,
    float x0, float y0, float x1, float y1,
    const Color& color, float radius) {
  const float dx = x1 - x0;
  const float dy = y1 - y0;
  const float segLen = std::sqrt(dx * dx + dy * dy);

  if (segLen < 0.001f) {
    stampCircleAA(target, x0, y0, radius, color);
    return;
  }

  // Spacing = 20% of diameter to guarantee solid fill without gaps
  const float spacingPx = std::max(0.5f, radius * 0.4f);
  const int steps = static_cast<int>(std::ceil(segLen / spacingPx));
  const float invSteps = 1.0f / static_cast<float>(steps);

  for (int i = 0; i <= steps; ++i) {
    const float t = static_cast<float>(i) * invSteps;
    stampCircleAA(target, x0 + dx * t, y0 + dy * t, radius, color);
  }
}

void rasterizeVectorLayer(const Layer& layer, PixelBuffer& out) {
  for (const VectorPath& path : layer.vectorPaths()) {
    if (path.points.empty()) {
      continue;
    }
    Color color = path.color;
    color.a = static_cast<std::uint8_t>(
        std::lround(std::clamp(path.opacity, 0.0f, 1.0f) * static_cast<float>(color.a)));
    if (color.a == 0) {
      continue;
    }
    const float radius = static_cast<float>(std::max(1, path.width)) * 0.5f;

    if (path.points.size() == 1) {
      stampCircleAA(out, path.points.front().x, path.points.front().y, radius, color);
      continue;
    }
    for (std::size_t i = 1; i < path.points.size(); ++i) {
      drawSegmentAA(out,
          path.points[i - 1].x, path.points[i - 1].y,
          path.points[i].x,     path.points[i].y,
          color, radius);
    }
  }
}

Color applyLayerMask(const Layer& layer, int x, int y, Color src) {
  if (!layer.hasMask() || !layer.maskEnabled()) {
    return src;
  }
  const Color mask = layer.maskBuffer().pixel(x, y);
  const float maskAlpha = static_cast<float>(mask.a) / 255.0f;
  src.a = static_cast<std::uint8_t>(std::lround(static_cast<float>(src.a) * std::clamp(maskAlpha, 0.0f, 1.0f)));
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
    if (layer.kind() != LayerKind::Vector || !layer.visible() || layer.opacity() <= 0.0f) {
      continue;
    }
    vectorRasters[layerIndex].resize(size.width, size.height, Color::Transparent());
    rasterizeVectorLayer(layer, vectorRasters[layerIndex]);
  }

  for (int y = area.y; y < area.y + area.height; ++y) {
    for (int x = area.x; x < area.x + area.width; ++x) {
      Color composed = document.paperVisible() ? document.paperColor() : Color::Transparent();
      float belowAlpha = 0.0f;

      for (std::size_t layerIndex = 0; layerIndex < document.layerCount(); ++layerIndex) {
        const Layer& layer = document.layerAt(layerIndex);
        if (!layer.visible() || layer.opacity() <= 0.0f || layer.kind() == LayerKind::Folder || layer.isPaperLayer()) {
          continue;
        }

        const PixelBuffer* sourceBuffer = &layer.buffer();
        if (layer.kind() == LayerKind::Vector) {
          sourceBuffer = &vectorRasters[layerIndex];
        }

        Color src = sourceBuffer->pixel(x, y);
        src = applyLayerMask(layer, x, y, src);
        if (layer.clippedToBelow() && belowAlpha <= 0.0001f) {
          continue;
        }
        composed = blendOver(composed, src, layer.opacity(), layer.blendMode());
        const float srcAlpha = (static_cast<float>(src.a) / 255.0f) * std::clamp(layer.opacity(), 0.0f, 1.0f);
        belowAlpha = srcAlpha + belowAlpha * (1.0f - srcAlpha);
      }

      target.setPixel(x, y, composed);
    }
  }
}

} // namespace core
