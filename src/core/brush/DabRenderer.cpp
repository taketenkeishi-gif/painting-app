#include "core/brush/DabRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "core/render/RenderUtils.h"

namespace {

constexpr float kPi = 3.14159265358979f;

// ── HSV ↔ RGB 変換（ウェットミックスの「泥色」防止） ─────────────────────
// RGB 空間での直線補間は中間色が暗くなる（gray mud）。
// HSV 空間で最短角補間することで鮮やかさを保つ。
struct HSV { float h, s, v; };

inline HSV rgbToHsv(float r, float g, float b) noexcept {
  const float cmax  = std::max({r, g, b});
  const float cmin  = std::min({r, g, b});
  const float delta = cmax - cmin;
  HSV out;
  out.v = cmax;
  out.s = cmax > 0.0001f ? delta / cmax : 0.0f;
  if (delta < 0.0001f) { out.h = 0.0f; return out; }
  if (cmax == r)       out.h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
  else if (cmax == g)  out.h = (b - r) / delta + 2.0f;
  else                 out.h = (r - g) / delta + 4.0f;
  out.h /= 6.0f;
  return out;
}

inline void hsvToRgb(float h, float s, float v, float& r, float& g, float& b) noexcept {
  if (s < 0.0001f) { r = g = b = v; return; }
  const float hh = h * 6.0f;
  const int   i  = static_cast<int>(hh) % 6;
  const float f  = hh - std::floor(hh);
  const float p  = v * (1.0f - s);
  const float q  = v * (1.0f - s * f);
  const float t2 = v * (1.0f - s * (1.0f - f));
  switch (i) {
    case 0: r=v;  g=t2; b=p;  break;
    case 1: r=q;  g=v;  b=p;  break;
    case 2: r=p;  g=v;  b=t2; break;
    case 3: r=p;  g=q;  b=v;  break;
    case 4: r=t2; g=p;  b=v;  break;
    default: r=v; g=p;  b=q;  break;
  }
}

// HSV 空間で 2 色を混合（色相は最短経路で補間）
inline core::Color mixColorHSV(const core::Color& a, const core::Color& b, float t) noexcept {
  const float aR = a.r / 255.0f, aG = a.g / 255.0f, aB = a.b / 255.0f;
  const float bR = b.r / 255.0f, bG = b.g / 255.0f, bB = b.b / 255.0f;
  const HSV ha = rgbToHsv(aR, aG, aB);
  const HSV hb = rgbToHsv(bR, bG, bB);
  float dh = hb.h - ha.h;
  if (dh >  0.5f) dh -= 1.0f;
  if (dh < -0.5f) dh += 1.0f;
  const float rh = ha.h + dh * t;
  const float rs = ha.s + (hb.s - ha.s) * t;
  const float rv = ha.v + (hb.v - ha.v) * t;
  float r, g, bl;
  hsvToRgb(rh < 0.0f ? rh + 1.0f : (rh >= 1.0f ? rh - 1.0f : rh), rs, rv, r, g, bl);
  const float ra = (a.a / 255.0f) + (b.a / 255.0f - a.a / 255.0f) * t;
  return core::Color {
    static_cast<uint8_t>(std::lround(std::clamp(r,  0.0f, 1.0f) * 255.0f)),
    static_cast<uint8_t>(std::lround(std::clamp(g,  0.0f, 1.0f) * 255.0f)),
    static_cast<uint8_t>(std::lround(std::clamp(bl, 0.0f, 1.0f) * 255.0f)),
    static_cast<uint8_t>(std::lround(std::clamp(ra, 0.0f, 1.0f) * 255.0f))
  };
}

// ── ハッシュベースのグレインノイズ (0.0–1.0) ─────────────────────────────
float grainNoise(int px, int py, int seed) noexcept {
  std::uint32_t h = static_cast<std::uint32_t>(px * 1619 + py * 31337 + seed * 6271);
  h ^= h >> 16;
  h *= 0x45d9f3bU;
  h ^= h >> 16;
  return static_cast<float>(h & 0xFFFFU) / 65535.0f;
}

float grainAt(int px, int py, float scale, int seed) noexcept {
  const int gx = static_cast<int>(std::floor(static_cast<float>(px) / scale));
  const int gy = static_cast<int>(std::floor(static_cast<float>(py) / scale));
  return grainNoise(gx, gy, seed);
}

// Color の線形補間
inline core::Color lerpColor(const core::Color& a, const core::Color& b, float t) noexcept {
  const float s = 1.0f - t;
  return core::Color {
    static_cast<std::uint8_t>(std::lround(s * a.r + t * b.r)),
    static_cast<std::uint8_t>(std::lround(s * a.g + t * b.g)),
    static_cast<std::uint8_t>(std::lround(s * a.b + t * b.b)),
    static_cast<std::uint8_t>(std::lround(s * a.a + t * b.a))
  };
}

} // namespace

namespace core {

void DabRenderer::render(
    PixelBuffer&         buffer,
    const PixelBuffer&   composited,
    PixelBuffer&         strokeAccum,
    const BrushSettings& settings,
    bool                 maskEditMode,
    Color&               smearColor,
    const FPoint&        center,
    float                radius,
    float                strength,
    bool                 lockAlpha,
    float                angleDegrees,
    const BlendFn&       blendFn)
{
  const auto& dyn = settings.dynamics;
  const float angleRad    = (settings.angle + angleDegrees) * (kPi / 180.0f);
  const float cosA        = std::cos(angleRad);
  const float sinA        = std::sin(angleRad);
  const float invRoundness = (settings.roundness > 0.001f) ? (1.0f / settings.roundness) : 1.0f;

  // AA=true かつ hardness≒1 のとき fringe が半径の外に出るので +1 px 余裕を持たせる
  const float margin = (settings.antiAlias && settings.hardness >= 0.999f) ? 1.5f : 1.0f;
  const int x0 = static_cast<int>(std::floor(center.x - radius - margin));
  const int y0 = static_cast<int>(std::floor(center.y - radius - margin));
  const int x1 = static_cast<int>(std::ceil(center.x + radius + margin));
  const int y1 = static_cast<int>(std::ceil(center.y + radius + margin));

  // スメア: stamp 中心でキャンバス色を採取して以降のピクセルに使用
  if (dyn.smear) {
    const int cx = static_cast<int>(center.x);
    const int cy = static_cast<int>(center.y);
    if (composited.inBounds(cx, cy)) {
      smearColor = lerpColor(composited.pixel(cx, cy), smearColor, dyn.smearRate);
    }
  }

  // グレインのフレームシード: stamp 位置から決定論的に決める
  const int grainSeed = static_cast<int>(center.x * 7 + center.y * 13);

  for (int py = y0; py <= y1; ++py) {
    if (py < 0 || py >= buffer.height()) continue;
    for (int px = x0; px <= x1; ++px) {
      if (px < 0 || px >= buffer.width()) continue;

      // ピクセル中心からブラシ中心への距離（float精度）
      float dx = (static_cast<float>(px) + 0.5f) - center.x;
      float dy = (static_cast<float>(py) + 0.5f) - center.y;

      // 回転と楕円変換
      float rdx =  dx * cosA + dy * sinA;
      float rdy = (-dx * sinA + dy * cosA) * invRoundness;

      // 正規化距離（0=中心, 1=縁）
      float dist = std::sqrt(rdx * rdx + rdy * rdy) / radius;

      // スクエアブラシ
      if (settings.shapeType == BrushShapeType::Square) {
        dist = std::max(std::abs(rdx) / radius, std::abs(rdy) / radius);
      }

      float pixelStrength = brushCoverage(dist, settings.hardness, radius, settings.antiAlias) * strength;
      if (pixelStrength <= 0.001f) continue;

      // ── テクスチャグレイン ──────────────────────────────────────────────
      if (dyn.textureGrain) {
        const float g = grainAt(px, py, dyn.textureScale, grainSeed);
        pixelStrength *= std::clamp(g + (1.0f - dyn.textureStrength), 0.0f, 1.0f);
      }

      if (pixelStrength <= 0.001f) continue;

      // ── ウェットミックス / スメア: 描画色を決定 ──────────────────────────
      Color drawColor = settings.color;
      if (maskEditMode) {
        // マスク編集: 輝度でグレースケール化して alpha=255 にする
        const std::uint8_t lum = static_cast<std::uint8_t>(std::lround(
            core::luminanceBT601(drawColor.r / 255.0f, drawColor.g / 255.0f, drawColor.b / 255.0f) * 255.0f));
        drawColor = Color {lum, lum, lum, 255};
      } else if (dyn.smear) {
        // スメア: キャンバス色を押し広げる（ブラシ色を使わない）
        drawColor = smearColor;
      } else if (dyn.wetMix && composited.inBounds(px, py)) {
        // ウェットミックス: HSV 空間で混合（RGB lerp の「泥色」を防ぐ）
        const Color canvasCol = composited.pixel(px, py);
        drawColor = mixColorHSV(settings.color, canvasCol, dyn.wetMixRate);
      }

      // ── buildup=false のとき、ストロークバッファで積み重ねを制御 ───────────
      if (!settings.buildupMode && !settings.eraseMode &&
          strokeAccum.inBounds(px, py)) {
        const float prevAccum = static_cast<float>(strokeAccum.pixel(px, py).r) / 255.0f;
        if (pixelStrength <= prevAccum + 0.001f) continue;

        strokeAccum.setPixel(px, py, Color {
            static_cast<std::uint8_t>(std::lround(pixelStrength * 255.0f)), 0, 0, 255});

        if (lockAlpha) {
          const float delta = pixelStrength - prevAccum;
          blendFn(px, py, drawColor, delta, lockAlpha);
        } else {
          const float dstA = static_cast<float>(buffer.pixel(px, py).a) / 255.0f;
          const bool isTransparentMode = (drawColor.a == 0);
          if (dstA >= 0.999f && !isTransparentMode) {
            const float delta = pixelStrength - prevAccum;
            if (delta > 0.001f) {
              blendFn(px, py, drawColor, delta, lockAlpha);
            }
          } else {
            const float srcANeeded = std::clamp(
                (pixelStrength - dstA) / (1.0f - dstA), 0.0f, 1.0f);
            if (srcANeeded <= 0.001f) continue;
            blendFn(px, py, drawColor, srcANeeded, lockAlpha);
          }
        }
      } else {
        blendFn(px, py, drawColor, pixelStrength, lockAlpha);
      }
    }
  }
}

} // namespace core
