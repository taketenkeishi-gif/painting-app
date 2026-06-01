#include "core/tools/BrushTool.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

#include "core/render/RenderUtils.h"

namespace {
  constexpr float kPi = 3.14159265358979f;

  // ── HSL ヘルパー ─────────────────────────────────────────────────────────
  inline float hslLuminance(float r, float g, float b) noexcept {
    return 0.299f * r + 0.587f * g + 0.114f * b;
  }

  inline float hslSaturation(float r, float g, float b) noexcept {
    const float cmax = std::max({r, g, b});
    const float cmin = std::min({r, g, b});
    return cmax - cmin;
  }

  struct RGB3 { float r, g, b; };

  inline RGB3 clipColor(RGB3 c) noexcept {
    const float l = hslLuminance(c.r, c.g, c.b);
    const float n = std::min({c.r, c.g, c.b});
    const float x = std::max({c.r, c.g, c.b});
    if (n < 0.0f) {
      c.r = l + (c.r - l) * l / (l - n);
      c.g = l + (c.g - l) * l / (l - n);
      c.b = l + (c.b - l) * l / (l - n);
    }
    if (x > 1.0f) {
      c.r = l + (c.r - l) * (1.0f - l) / (x - l);
      c.g = l + (c.g - l) * (1.0f - l) / (x - l);
      c.b = l + (c.b - l) * (1.0f - l) / (x - l);
    }
    return c;
  }

  inline RGB3 setLuminance(RGB3 c, float lum) noexcept {
    const float d = lum - hslLuminance(c.r, c.g, c.b);
    return clipColor({c.r + d, c.g + d, c.b + d});
  }

  inline RGB3 setSaturation(RGB3 c, float sat) noexcept {
    float& cmin_ref = c.r < c.g ? (c.r < c.b ? c.r : c.b) : (c.g < c.b ? c.g : c.b);
    float& cmax_ref = c.r > c.g ? (c.r > c.b ? c.r : c.b) : (c.g > c.b ? c.g : c.b);
    // mid: the one that is neither min nor max
    // We'll do it manually without reference tricks
    float vmin = std::min({c.r, c.g, c.b});
    float vmax = std::max({c.r, c.g, c.b});
    float vmid;
    if ((c.r >= vmin && c.r <= vmax) && !(c.r == vmin || c.r == vmax)) vmid = c.r;
    else if ((c.g >= vmin && c.g <= vmax) && !(c.g == vmin || c.g == vmax)) vmid = c.g;
    else vmid = c.b;
    (void)cmin_ref; (void)cmax_ref;

    if (vmax > vmin) {
      vmid = (vmid - vmin) * sat / (vmax - vmin);
      vmax = sat;
    } else {
      vmid = 0.0f;
      vmax = 0.0f;
    }
    vmin = 0.0f;
    // assign back by position
    float newMin = vmin, newMid = vmid, newMax = vmax;
    // Sort original r,g,b into min/mid/max order and rebuild
    // Simpler: recompute from scratch
    const float or_ = c.r, og = c.g, ob = c.b;
    const float omin = std::min({or_, og, ob});
    const float omax = std::max({or_, og, ob});
    const float omid = or_ + og + ob - omin - omax;
    (void)omid;
    RGB3 out = c;
    if (or_ == omin) out.r = newMin;
    else if (or_ == omax) out.r = newMax;
    else out.r = newMid;
    if (og == omin) out.g = newMin;
    else if (og == omax) out.g = newMax;
    else out.g = newMid;
    if (ob == omin) out.b = newMin;
    else if (ob == omax) out.b = newMax;
    else out.b = newMid;
    return out;
  }

  // ── ハッシュベースのグレインノイズ (0.0–1.0) ─────────────────────────────
  // セルノイズ: 入力座標とseedから確定的ランダム値を返す
  float grainNoise(int px, int py, int seed) noexcept {
    std::uint32_t h = static_cast<std::uint32_t>(px * 1619 + py * 31337 + seed * 6271);
    h ^= h >> 16;
    h *= 0x45d9f3bU;
    h ^= h >> 16;
    return static_cast<float>(h & 0xFFFFU) / 65535.0f;
  }

  // テクスチャスケールに合わせて座標をダウンサンプリング
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
        static_cast<std::uint8_t>(std::lround(s * a.a + t * b.a))};
  }
} // namespace

namespace core {

namespace {

Rect strokeDirtyRect(const FPoint& from, const FPoint& to, float size) {
  const int r = static_cast<int>(std::ceil(size * 0.5f)) + 2;
  const int minX = static_cast<int>(std::floor(std::min(from.x, to.x))) - r;
  const int minY = static_cast<int>(std::floor(std::min(from.y, to.y))) - r;
  const int maxX = static_cast<int>(std::ceil(std::max(from.x, to.x))) + r;
  const int maxY = static_cast<int>(std::ceil(std::max(from.y, to.y))) + r;
  return Rect {minX, minY, maxX - minX + 1, maxY - minY + 1};
}

} // namespace

// ---------------------------------------------------------------
// 筆圧→サイズ変換（リニアカーブ）
// ---------------------------------------------------------------
float BrushTool::computePressureSize(float pressure) const {
  if (!m_settings.dynamics.pressureSize) {
    return 1.0f;
  }
  const float minRatio = m_settings.dynamics.pressureSizeMin;
  return minRatio + (1.0f - minRatio) * pressure;
}

float BrushTool::computePressureOpacity(float pressure) const {
  if (!m_settings.dynamics.pressureOpacity) {
    return 1.0f;
  }
  const float minRatio = m_settings.dynamics.pressureOpacityMin;
  return minRatio + (1.0f - minRatio) * pressure;
}

// ---------------------------------------------------------------
// 速度係数 (0=静止, 1=高速)  段階的指数スムージング済み
// ---------------------------------------------------------------
float BrushTool::computeVelocityFactor(float segLenPx) const noexcept {
  // 基準速度: ブラシ直径の 8 倍/frame を「高速」とみなす
  const float refSpeed = static_cast<float>(std::max(1, m_settings.size)) * 8.0f;
  return std::clamp(segLenPx / refSpeed, 0.0f, 1.0f);
}

// ---------------------------------------------------------------
// テーパー（入り抜き）強度
// ---------------------------------------------------------------
float BrushTool::computeTaperStrength(float t, float taperStart, float taperEnd) const {
  float s = 1.0f;
  if (taperStart > 0.001f && t < taperStart) {
    s *= t / taperStart;
  }
  if (taperEnd > 0.001f && t > (1.0f - taperEnd)) {
    s *= (1.0f - t) / taperEnd;
  }
  return s;
}

// ---------------------------------------------------------------
// スタビライザー
// ---------------------------------------------------------------
FPoint BrushTool::applyStabilization(const FPoint& from, const FPoint& to) const {
  const float stabilization = std::clamp(m_settings.stabilization, 0.0F, 1.0F);
  if (stabilization <= 0.001f) {
    return to;
  }

  float response = 1.0f - stabilization * 0.85f;
  if (m_settings.velocityBasedCorrection) {
    const float distance = from.lengthTo(to);
    const float velocityFactor = std::clamp(1.0f - distance / 80.0f, 0.25f, 1.0f);
    response *= velocityFactor;
  }
  response = std::clamp(response, 0.05f, 1.0f);

  return FPoint {
      from.x + (to.x - from.x) * response,
      from.y + (to.y - from.y) * response};
}

// ---------------------------------------------------------------
// ピクセルへの書き込み（ブレンドモード対応）
// ---------------------------------------------------------------
void BrushTool::blendPixel(
    PixelBuffer& buffer, int x, int y,
    const Color& src, float strength, bool lockAlpha) const {
  if (!buffer.inBounds(x, y)) {
    return;
  }

  const Color dst = buffer.pixel(x, y);
  if (lockAlpha && dst.a == 0) {
    return;
  }

  const float alphaScale = std::clamp(strength, 0.0F, 1.0F);

  if (m_settings.eraseMode) {
    const float eraseStr = std::clamp((static_cast<float>(src.a) / 255.0f) * alphaScale, 0.0f, 1.0f);
    const float keep = 1.0f - eraseStr;
    buffer.setPixel(x, y, Color {
        static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.r) * keep)),
        static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.g) * keep)),
        static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.b) * keep)),
        static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.a) * keep))});
    return;
  }

  const Color effectiveSrc {
      src.r, src.g, src.b,
      static_cast<std::uint8_t>(std::lround(static_cast<float>(src.a) * alphaScale))};
  if (effectiveSrc.a == 0) {
    return;
  }

  const float srcA = static_cast<float>(effectiveSrc.a) / 255.0f;
  const float dstA = static_cast<float>(dst.a) / 255.0f;
  const float outA = lockAlpha ? dstA : srcA + dstA * (1.0f - srcA);
  if (outA <= 0.0f) {
    buffer.setPixel(x, y, Color::Transparent());
    return;
  }

  const float sR = static_cast<float>(effectiveSrc.r) / 255.0f;
  const float sG = static_cast<float>(effectiveSrc.g) / 255.0f;
  const float sB = static_cast<float>(effectiveSrc.b) / 255.0f;
  const float dR = static_cast<float>(dst.r) / 255.0f;
  const float dG = static_cast<float>(dst.g) / 255.0f;
  const float dB = static_cast<float>(dst.b) / 255.0f;

  float outR = 0.0f;
  float outG = 0.0f;
  float outB = 0.0f;

  auto compositeResult = [&](float blendR, float blendG, float blendB) {
    if (lockAlpha) {
      outR = dR + (blendR - dR) * srcA;
      outG = dG + (blendG - dG) * srcA;
      outB = dB + (blendB - dB) * srcA;
    } else {
      outR = (blendR * srcA + dR * dstA * (1.0f - srcA)) / outA;
      outG = (blendG * srcA + dG * dstA * (1.0f - srcA)) / outA;
      outB = (blendB * srcA + dB * dstA * (1.0f - srcA)) / outA;
    }
  };

  switch (m_settings.blendMode) {
    case BlendMode::Multiply:
      compositeResult(dR * sR, dG * sG, dB * sB);
      break;

    case BlendMode::LinearDodge:  // Add
      compositeResult(
          std::clamp(dR + sR, 0.0f, 1.0f),
          std::clamp(dG + sG, 0.0f, 1.0f),
          std::clamp(dB + sB, 0.0f, 1.0f));
      break;

    case BlendMode::Screen:
      compositeResult(1.0f-(1.0f-dR)*(1.0f-sR), 1.0f-(1.0f-dG)*(1.0f-sG), 1.0f-(1.0f-dB)*(1.0f-sB));
      break;

    case BlendMode::Overlay:
      compositeResult(
          dR < 0.5f ? 2.0f*dR*sR : 1.0f-2.0f*(1.0f-dR)*(1.0f-sR),
          dG < 0.5f ? 2.0f*dG*sG : 1.0f-2.0f*(1.0f-dG)*(1.0f-sG),
          dB < 0.5f ? 2.0f*dB*sB : 1.0f-2.0f*(1.0f-dB)*(1.0f-sB));
      break;

    case BlendMode::SoftLight: {
      auto softL = [](float d, float s) {
        if (s <= 0.5f) {
          return d - (1.0f - 2.0f*s) * d * (1.0f - d);
        }
        float gd = d <= 0.25f ? ((16.0f*d - 12.0f)*d + 4.0f)*d : std::sqrt(d);
        return d + (2.0f*s - 1.0f) * (gd - d);
      };
      compositeResult(softL(dR,sR), softL(dG,sG), softL(dB,sB));
      break;
    }

    case BlendMode::HardLight:
      compositeResult(
          sR < 0.5f ? 2.0f*dR*sR : 1.0f-2.0f*(1.0f-dR)*(1.0f-sR),
          sG < 0.5f ? 2.0f*dG*sG : 1.0f-2.0f*(1.0f-dG)*(1.0f-sG),
          sB < 0.5f ? 2.0f*dB*sB : 1.0f-2.0f*(1.0f-dB)*(1.0f-sB));
      break;

    case BlendMode::ColorDodge:
      compositeResult(
          sR >= 1.0f ? 1.0f : std::clamp(dR / (1.0f - sR), 0.0f, 1.0f),
          sG >= 1.0f ? 1.0f : std::clamp(dG / (1.0f - sG), 0.0f, 1.0f),
          sB >= 1.0f ? 1.0f : std::clamp(dB / (1.0f - sB), 0.0f, 1.0f));
      break;

    case BlendMode::ColorBurn:
      compositeResult(
          sR <= 0.0f ? 0.0f : std::clamp(1.0f - (1.0f - dR) / sR, 0.0f, 1.0f),
          sG <= 0.0f ? 0.0f : std::clamp(1.0f - (1.0f - dG) / sG, 0.0f, 1.0f),
          sB <= 0.0f ? 0.0f : std::clamp(1.0f - (1.0f - dB) / sB, 0.0f, 1.0f));
      break;

    case BlendMode::LinearBurn:
      compositeResult(
          std::clamp(dR + sR - 1.0f, 0.0f, 1.0f),
          std::clamp(dG + sG - 1.0f, 0.0f, 1.0f),
          std::clamp(dB + sB - 1.0f, 0.0f, 1.0f));
      break;

    case BlendMode::Darken:
      compositeResult(std::min(dR,sR), std::min(dG,sG), std::min(dB,sB));
      break;

    case BlendMode::Lighten:
      compositeResult(std::max(dR,sR), std::max(dG,sG), std::max(dB,sB));
      break;

    case BlendMode::Difference:
      compositeResult(std::abs(dR-sR), std::abs(dG-sG), std::abs(dB-sB));
      break;

    case BlendMode::Exclusion:
      compositeResult(dR+sR-2.0f*dR*sR, dG+sG-2.0f*dG*sG, dB+sB-2.0f*dB*sB);
      break;

    case BlendMode::Subtract:
      compositeResult(std::clamp(dR-sR,0.0f,1.0f), std::clamp(dG-sG,0.0f,1.0f), std::clamp(dB-sB,0.0f,1.0f));
      break;

    case BlendMode::Divide:
      compositeResult(
          sR <= 0.0f ? 1.0f : std::clamp(dR/sR, 0.0f, 1.0f),
          sG <= 0.0f ? 1.0f : std::clamp(dG/sG, 0.0f, 1.0f),
          sB <= 0.0f ? 1.0f : std::clamp(dB/sB, 0.0f, 1.0f));
      break;

    case BlendMode::VividLight:
      compositeResult(
          sR < 0.5f ? (sR<=0.0f?0.0f:std::clamp(1.0f-(1.0f-dR)/(2.0f*sR),0.0f,1.0f)) : (sR>=1.0f?1.0f:std::clamp(dR/(2.0f*(1.0f-sR)),0.0f,1.0f)),
          sG < 0.5f ? (sG<=0.0f?0.0f:std::clamp(1.0f-(1.0f-dG)/(2.0f*sG),0.0f,1.0f)) : (sG>=1.0f?1.0f:std::clamp(dG/(2.0f*(1.0f-sG)),0.0f,1.0f)),
          sB < 0.5f ? (sB<=0.0f?0.0f:std::clamp(1.0f-(1.0f-dB)/(2.0f*sB),0.0f,1.0f)) : (sB>=1.0f?1.0f:std::clamp(dB/(2.0f*(1.0f-sB)),0.0f,1.0f)));
      break;

    case BlendMode::LinearLight:
      compositeResult(std::clamp(dR+2.0f*sR-1.0f,0.0f,1.0f), std::clamp(dG+2.0f*sG-1.0f,0.0f,1.0f), std::clamp(dB+2.0f*sB-1.0f,0.0f,1.0f));
      break;

    case BlendMode::PinLight:
      compositeResult(
          sR < 0.5f ? std::min(dR,2.0f*sR) : std::max(dR,2.0f*sR-1.0f),
          sG < 0.5f ? std::min(dG,2.0f*sG) : std::max(dG,2.0f*sG-1.0f),
          sB < 0.5f ? std::min(dB,2.0f*sB) : std::max(dB,2.0f*sB-1.0f));
      break;

    case BlendMode::HardMix:
      compositeResult(
          (dR+sR) >= 1.0f ? 1.0f : 0.0f,
          (dG+sG) >= 1.0f ? 1.0f : 0.0f,
          (dB+sB) >= 1.0f ? 1.0f : 0.0f);
      break;

    case BlendMode::Dissolve: {
      // ランダムなピクセルのみ描画（確率 = alphaScale）
      const std::uint32_t h = static_cast<std::uint32_t>(x * 1619 + y * 31337 + 6271);
      const float rnd = static_cast<float>(h ^ (h >> 16)) / static_cast<float>(0xFFFFFFFFU);
      if (rnd >= alphaScale) {
        return; // このピクセルは描かない
      }
      compositeResult(sR, sG, sB);
      break;
    }

    case BlendMode::DarkerColor: {
      const float lumD = hslLuminance(dR, dG, dB);
      const float lumS = hslLuminance(sR, sG, sB);
      if (lumS < lumD) compositeResult(sR, sG, sB);
      else compositeResult(dR, dG, dB);
      break;
    }

    case BlendMode::LighterColor: {
      const float lumD = hslLuminance(dR, dG, dB);
      const float lumS = hslLuminance(sR, sG, sB);
      if (lumS > lumD) compositeResult(sR, sG, sB);
      else compositeResult(dR, dG, dB);
      break;
    }

    case BlendMode::Hue: {
      const RGB3 res = setLuminance(setSaturation({sR, sG, sB}, hslSaturation(dR, dG, dB)),
                                    hslLuminance(dR, dG, dB));
      compositeResult(res.r, res.g, res.b);
      break;
    }

    case BlendMode::HslSat: {
      const RGB3 res = setLuminance(setSaturation({dR, dG, dB}, hslSaturation(sR, sG, sB)),
                                    hslLuminance(dR, dG, dB));
      compositeResult(res.r, res.g, res.b);
      break;
    }

    case BlendMode::HslColor: {
      const RGB3 res = setLuminance({sR, sG, sB}, hslLuminance(dR, dG, dB));
      compositeResult(res.r, res.g, res.b);
      break;
    }

    case BlendMode::Luminosity: {
      const RGB3 res = setLuminance({dR, dG, dB}, hslLuminance(sR, sG, sB));
      compositeResult(res.r, res.g, res.b);
      break;
    }

    case BlendMode::Normal:
    default:
      compositeResult(sR, sG, sB);
      break;
  }

  buffer.setPixel(x, y, Color {
      static_cast<std::uint8_t>(std::lround(std::clamp(outR, 0.0f, 1.0f) * 255.0f)),
      static_cast<std::uint8_t>(std::lround(std::clamp(outG, 0.0f, 1.0f) * 255.0f)),
      static_cast<std::uint8_t>(std::lround(std::clamp(outB, 0.0f, 1.0f) * 255.0f)),
      static_cast<std::uint8_t>(std::lround(std::clamp(outA, 0.0f, 1.0f) * 255.0f))});
}

// ---------------------------------------------------------------
// スタンプ：float座標で高精度描画
// radius はブラシ半径、strength は opacity * flow
// composited: ウェットミックス/スメアで参照する合成済みバッファ
// ---------------------------------------------------------------
void BrushTool::stampAt(
    PixelBuffer& buffer, const PixelBuffer& composited,
    const FPoint& center, float radius,
    float strength, bool lockAlpha) const {
  if (radius <= 0.0f || strength <= 0.0f) {
    return;
  }

  const auto& dyn = m_settings.dynamics;
  const float angleRad = m_settings.angle * (kPi / 180.0f);
  const float cosA = std::cos(angleRad);
  const float sinA = std::sin(angleRad);
  const float invRoundness = (m_settings.roundness > 0.001f) ? (1.0f / m_settings.roundness) : 1.0f;

  // AA=true かつ hardness≒1 のとき fringe が半径の外に出るので +1 px 余裕を持たせる
  const float margin = (m_settings.antiAlias && m_settings.hardness >= 0.999f) ? 1.5f : 1.0f;
  const int x0 = static_cast<int>(std::floor(center.x - radius - margin));
  const int y0 = static_cast<int>(std::floor(center.y - radius - margin));
  const int x1 = static_cast<int>(std::ceil(center.x + radius + margin));
  const int y1 = static_cast<int>(std::ceil(center.y + radius + margin));

  // スメア: stamp 中心でキャンバス色を採取して以降のピクセルに使用
  if (dyn.smear) {
    const int cx = static_cast<int>(center.x);
    const int cy = static_cast<int>(center.y);
    if (composited.inBounds(cx, cy)) {
      m_smearColor = lerpColor(composited.pixel(cx, cy), m_smearColor, dyn.smearRate);
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
      float rdx = dx * cosA + dy * sinA;
      float rdy = (-dx * sinA + dy * cosA) * invRoundness;

      // 正規化距離（0=中心, 1=縁）
      float dist = std::sqrt(rdx * rdx + rdy * rdy) / radius;

      // スクエアブラシ
      if (m_settings.shapeType == BrushShapeType::Square) {
        dist = std::max(std::abs(rdx) / radius, std::abs(rdy) / radius);
      }

      float pixelStrength = brushCoverage(dist, m_settings.hardness, radius, m_settings.antiAlias) * strength;
      if (pixelStrength <= 0.001f) continue;

      // ── テクスチャグレイン ──────────────────────────────────────────────
      if (dyn.textureGrain) {
        const float g = grainAt(px, py, dyn.textureScale, grainSeed);
        // グレインが低い部分ほど coverage を削る
        pixelStrength *= std::clamp(g + (1.0f - dyn.textureStrength), 0.0f, 1.0f);
      }

      if (pixelStrength <= 0.001f) continue;

      // ── ウェットミックス / スメア: 描画色を決定 ──────────────────────────
      Color drawColor = m_settings.color;
      if (dyn.smear) {
        // スメア: キャンバス色を押し広げる（ブラシ色を使わない）
        drawColor = m_smearColor;
      } else if (dyn.wetMix && composited.inBounds(px, py)) {
        // ウェットミックス: ブラシ色 と キャンバス色 を混ぜる
        const Color canvasCol = composited.pixel(px, py);
        drawColor = lerpColor(m_settings.color, canvasCol, dyn.wetMixRate);
      }

      // ── buildup=false のとき、ストロークバッファで積み重ねを制御 ───────────
      if (!m_settings.buildupMode && !m_settings.eraseMode &&
          m_strokeAccum.inBounds(px, py)) {
        const float prevAccum = static_cast<float>(m_strokeAccum.pixel(px, py).r) / 255.0f;
        if (pixelStrength <= prevAccum + 0.001f) continue;

        // Porter-Duff 正確計算:
        //   既存アルファ dstA から目標アルファ pixelStrength に達するために
        //   必要な srcA を逆算 → srcA = (tgt - dstA) / (1 - dstA)
        // こうすることで delta 近似ではなく正確に目標カバレッジに到達する
        m_strokeAccum.setPixel(px, py, Color {
            static_cast<std::uint8_t>(std::lround(pixelStrength * 255.0f)), 0, 0, 255});

        if (lockAlpha) {
          // アルファロック時はアルファを増やせないので単純に差分強度でブレンド
          const float delta = pixelStrength - prevAccum;
          blendPixel(buffer, px, py, drawColor, delta, lockAlpha);
        } else {
          const float dstA = static_cast<float>(buffer.pixel(px, py).a) / 255.0f;
          if (dstA >= 0.999f) continue; // already fully opaque
          const float srcANeeded = std::clamp(
              (pixelStrength - dstA) / (1.0f - dstA), 0.0f, 1.0f);
          if (srcANeeded <= 0.001f) continue;
          blendPixel(buffer, px, py, drawColor, srcANeeded, lockAlpha);
        }
      } else {
        blendPixel(buffer, px, py, drawColor, pixelStrength, lockAlpha);
      }
    }
  }
}

// ---------------------------------------------------------------
// ストロークセグメント描画
// from→to の間にスタンプを等間隔で配置
// ---------------------------------------------------------------
void BrushTool::strokeSegment(
    Layer& layer, const PixelBuffer& composited,
    const FPoint& from, const FPoint& to,
    float pressureFrom, float pressureTo,
    float strokeT, float strokeLen) {
  if (layer.locked()) return;

  PixelBuffer& buffer = layer.buffer();
  const bool lockAlpha = m_settings.lockAlphaRespect || layer.alphaLocked();
  const auto& dyn = m_settings.dynamics;

  const float baseRadius = static_cast<float>(std::max(1, m_settings.size)) * 0.5f;
  const float dx = to.x - from.x;
  const float dy = to.y - from.y;
  const float segLen = std::sqrt(dx * dx + dy * dy);

  // ── 速度係数を更新（指数スムージング） ──────────────────────────────────
  const auto now = Clock::now();
  const float dtMs = static_cast<float>(
      std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastMoveTime).count()) / 1000.0f;
  m_lastMoveTime = now;
  if (dtMs > 0.5f) {
    const float rawVel = segLen / dtMs; // px/ms
    m_currentVelocityPxMs = m_currentVelocityPxMs * 0.7f + rawVel * 0.3f;
  }
  // 速度係数: refSpeed = brushDiameter * 2 px/ms を「高速」とみなす
  const float refSpeed = static_cast<float>(std::max(1, m_settings.size)) * 2.0f; // px/ms
  const float velFactor = std::clamp(m_currentVelocityPxMs / refSpeed, 0.0f, 1.0f);

  // 速度→サイズ比率 (1=通常, velocitySizeMin=最高速時)
  const float velSizeScale = dyn.velocitySize
      ? (1.0f - velFactor * (1.0f - dyn.velocitySizeMin))
      : 1.0f;
  // 速度→opacity比率
  const float velOpacityScale = dyn.velocityOpacity
      ? (1.0f - velFactor * (1.0f - dyn.velocityOpacityMin))
      : 1.0f;

  // spacing はブラシ直径の比率
  const float spacingPx = std::max(0.5f, m_settings.spacing * baseRadius * 2.0f);

  if (segLen < 0.001f) {
    const float radius   = baseRadius * computePressureSize(pressureFrom) * velSizeScale;
    const float opScale  = computePressureOpacity(pressureFrom) * velOpacityScale;
    const float strength = std::clamp(m_settings.opacity * m_settings.flow * opScale, 0.0f, 1.0f);
    stampAt(buffer, composited, from, radius, strength, lockAlpha);
    return;
  }

  float traveled = spacingPx - m_distanceAccum;
  if (traveled < 0.0f) traveled = 0.0f;
  if (traveled > segLen) {
    m_distanceAccum += segLen;
    return;
  }

  while (traveled <= segLen + 0.001f) {
    const float t       = std::clamp(traveled / segLen, 0.0f, 1.0f);
    const FPoint pos    {from.x + dx * t, from.y + dy * t};
    const float pressure = pressureFrom + (pressureTo - pressureFrom) * t;
    const float radius   = baseRadius * computePressureSize(pressure) * velSizeScale;
    const float opScale  = computePressureOpacity(pressure) * velOpacityScale;

    float taperScale = 1.0f;
    if (strokeLen > 0.0f) {
      const float globalT = std::clamp((strokeT + traveled) / strokeLen, 0.0f, 1.0f);
      taperScale = computeTaperStrength(globalT, m_settings.taperStart, m_settings.taperEnd);
    }

    const float strength = std::clamp(m_settings.opacity * m_settings.flow * opScale * taperScale, 0.0f, 1.0f);
    stampAt(buffer, composited, pos, radius, strength, lockAlpha);

    traveled += spacingPx;
  }

  const float consumed = traveled - spacingPx;
  m_distanceAccum = segLen - (consumed - spacingPx);
  if (m_distanceAccum < 0.0f) m_distanceAccum = 0.0f;
}

// ---------------------------------------------------------------
// ポインターイベント
// ---------------------------------------------------------------
ToolResult BrushTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() == LayerKind::Folder || active->locked()) {
    return {};
  }

  m_drawing = true;
  m_lastPoint = event.fpoint;
  m_lastPressure = event.pressure;
  m_distanceAccum = 0.0f;
  m_strokeLength = 0.0f;
  m_currentVelocityPxMs = 0.0f;
  m_lastMoveTime = Clock::now();
  // スメア: ストローク開始点でキャンバス色を採取
  {
    const int cx = static_cast<int>(event.fpoint.x);
    const int cy = static_cast<int>(event.fpoint.y);
    if (context.composited.inBounds(cx, cy)) {
      m_smearColor = context.composited.pixel(cx, cy);
    }
  }

  if (active->kind() == LayerKind::Vector) {
    m_vectorPoints.clear();
    m_vectorPoints.push_back(event.fpoint);
    ToolResult result;
    result.viewportChanged = true;
    return result;
  }

  // ストロークバッファ初期化
  if (!m_settings.buildupMode && !m_settings.eraseMode) {
    m_strokeAccum.resize(active->buffer().width(), active->buffer().height(),
                         Color::Transparent());
    m_strokeAccumDirty = false;
  }

  const float baseRadius = static_cast<float>(std::max(1, m_settings.size)) * 0.5f;
  const float radius = baseRadius * computePressureSize(event.pressure);
  const float opacityScale = computePressureOpacity(event.pressure);
  const float strength = std::clamp(m_settings.opacity * m_settings.flow * opacityScale, 0.0f, 1.0f);
  const bool lockAlpha = m_settings.lockAlphaRespect || active->alphaLocked();
  stampAt(active->buffer(), context.composited, m_lastPoint, radius, strength, lockAlpha);

  ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = strokeDirtyRect(m_lastPoint, m_lastPoint, static_cast<float>(m_settings.size));
  return result;
}

ToolResult BrushTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_drawing) {
    return {};
  }

  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() == LayerKind::Folder || active->locked()) {
    m_drawing = false;
    m_vectorPoints.clear();
    return {};
  }

  const FPoint stabilized = applyStabilization(m_lastPoint, event.fpoint);

  if (active->kind() == LayerKind::Vector) {
    const FPoint fpt = event.fpoint;
    if (m_vectorPoints.empty()) {
      m_vectorPoints.push_back(fpt);
    }
    if (m_vectorPoints.back().x != fpt.x || m_vectorPoints.back().y != fpt.y) {
      m_vectorPoints.push_back(fpt);
    }
    m_lastPoint = stabilized;
    ToolResult result;
    result.viewportChanged = true;
    return result;
  }

  const float segLen = m_lastPoint.lengthTo(stabilized);
  m_strokeLength += segLen;

  strokeSegment(*active, context.composited, m_lastPoint, stabilized,
                m_lastPressure, event.pressure,
                m_strokeLength - segLen, m_strokeLength);

  const Rect dirty = strokeDirtyRect(m_lastPoint, stabilized, static_cast<float>(m_settings.size));
  m_lastPoint = stabilized;
  m_lastPressure = event.pressure;

  ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = dirty;
  return result;
}

ToolResult BrushTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_drawing) {
    return {};
  }

  m_drawing = false;
  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() == LayerKind::Folder || active->locked()) {
    m_vectorPoints.clear();
    return {};
  }

  const FPoint stabilized = applyStabilization(m_lastPoint, event.fpoint);

  if (active->kind() == LayerKind::Vector) {
    const FPoint fpt = event.fpoint;
    if (m_vectorPoints.empty()) {
      m_vectorPoints.push_back(fpt);
    }
    if (m_vectorPoints.back().x != fpt.x || m_vectorPoints.back().y != fpt.y) {
      m_vectorPoints.push_back(fpt);
    }
    if (m_settings.postCorrection &&
        (fpt.x != m_vectorPoints.back().x || fpt.y != m_vectorPoints.back().y)) {
      m_vectorPoints.push_back(fpt);
    }
    if (m_vectorPoints.size() >= 2) {
      VectorPath path;
      path.points = m_vectorPoints;
      path.color = m_settings.color;
      path.width = std::max(1, m_settings.size);
      const float alpha = (static_cast<float>(m_settings.color.a) / 255.0f) *
          std::clamp(m_settings.opacity * m_settings.flow, 0.0f, 1.0f);
      path.opacity = std::clamp(alpha, 0.0f, 1.0f);
      active->addVectorPath(std::move(path));
      m_vectorPoints.clear();
      ToolResult result;
      result.pixelsChanged = true;
      result.viewportChanged = true;
      return result;
    }
    m_vectorPoints.clear();
    return {};
  }

  const float segLen = m_lastPoint.lengthTo(stabilized);
  if (segLen > 0.001f) {
    m_strokeLength += segLen;
    strokeSegment(*active, context.composited, m_lastPoint, stabilized,
                  m_lastPressure, event.pressure,
                  m_strokeLength - segLen, m_strokeLength);
  }

  if (m_settings.postCorrection &&
      stabilized.lengthTo(event.fpoint) > 0.5f) {
    const float finalSegLen = stabilized.lengthTo(event.fpoint);
    m_strokeLength += finalSegLen;
    strokeSegment(*active, context.composited, stabilized, event.fpoint,
                  event.pressure, event.pressure,
                  m_strokeLength - finalSegLen, m_strokeLength);
  }

  ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = strokeDirtyRect(m_lastPoint, stabilized, static_cast<float>(m_settings.size));
  m_lastPoint = stabilized;
  return result;
}

ToolResult BrushTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  m_drawing = false;
  m_vectorPoints.clear();
  m_distanceAccum = 0.0f;
  m_strokeLength = 0.0f;
  return {};
}

ToolResult BrushTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

} // namespace core
