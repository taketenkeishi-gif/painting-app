#pragma once

#include <cmath>

namespace core {

// Unified brush coverage for a single pixel.
// dist      : normalized distance from brush center (0 = center, 1 = edge)
// hardness  : 0 = full Gaussian, 1 = hard circle
// radiusPx  : brush radius in canvas pixels — used to compute the 1-px AA fringe
// antiAlias : enable sub-pixel edge smoothing
inline float brushCoverage(float dist, float hardness, float radiusPx, bool antiAlias) noexcept {
  if (hardness >= 0.999f) {
    if (antiAlias && radiusPx > 0.5f) {
      // Porter-Duff compatible 1-pixel AA fringe:
      //   coverage = 1 at (r - 0.5 px), falls to 0 at (r + 0.5 px)
      return std::clamp(radiusPx * (1.0f - dist) + 0.5f, 0.0f, 1.0f);
    }
    return dist < 1.0f ? 1.0f : 0.0f;
  }

  // Soft Gaussian — already smooth, no extra AA needed
  if (dist >= 1.0f) return 0.0f;
  if (dist <= hardness) return 1.0f;
  const float t = (dist - hardness) / (1.0f - hardness);
  return std::exp(-5.0f * t * t);
}

// BT.601 輝度係数（HSL ブレンドモード・マスク編集で共用）
// BrushTool::blendPixel の HSL モードと DabRenderer のマスク編集パスで使用。
inline float luminanceBT601(float r, float g, float b) noexcept {
  return 0.299f * r + 0.587f * g + 0.114f * b;
}

} // namespace core
