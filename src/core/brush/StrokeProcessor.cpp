#include "core/brush/StrokeProcessor.h"

#include <algorithm>
#include <cmath>

namespace {

// Catmull-Rom スプライン: p1→p2 間を t∈[0,1] で補間。
// BrushTool.cpp と完全に同一のアルゴリズム。
inline core::FPoint catmullRomPos(
    const core::FPoint& p0, const core::FPoint& p1,
    const core::FPoint& p2, const core::FPoint& p3, float t) noexcept
{
  const float t2 = t * t;
  const float t3 = t2 * t;
  return {
    0.5f * ((2.0f * p1.x)
            + (-p0.x + p2.x) * t
            + (2.0f*p0.x - 5.0f*p1.x + 4.0f*p2.x - p3.x) * t2
            + (-p0.x + 3.0f*p1.x - 3.0f*p2.x + p3.x) * t3),
    0.5f * ((2.0f * p1.y)
            + (-p0.y + p2.y) * t
            + (2.0f*p0.y - 5.0f*p1.y + 4.0f*p2.y - p3.y) * t2
            + (-p0.y + 3.0f*p1.y - 3.0f*p2.y + p3.y) * t3)
  };
}

} // namespace

namespace core {

void StrokeProcessor::beginStroke(const FPoint& /*startPt*/) noexcept {
  m_hasPrevPoint  = false;
  m_prevPoint     = {0.0f, 0.0f};
  m_distanceAccum = 0.0f;
}

FPoint StrokeProcessor::applyStabilization(
    const FPoint& from, const FPoint& to,
    float stabilization, bool velocityBasedCorrection) const noexcept
{
  const float s = std::clamp(stabilization, 0.0F, 1.0F);
  if (s <= 0.001f) return to;

  float response = 1.0f - s * 0.85f;
  if (velocityBasedCorrection) {
    // 移動距離が大きいほど追従を鈍らせる（速書き時のオーバーシュート抑制）
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float velocityFactor = std::clamp(1.0f - distance / 80.0f, 0.25f, 1.0f);
    response *= velocityFactor;
  }
  response = std::clamp(response, 0.05f, 1.0f);
  return FPoint {
      from.x + (to.x - from.x) * response,
      from.y + (to.y - from.y) * response};
}

float StrokeProcessor::computeTaper(
    float t, float taperStart, float taperEnd) const noexcept
{
  float result = 1.0f;
  if (taperStart > 0.001f && t < taperStart) {
    result *= t / taperStart;
  }
  if (taperEnd > 0.001f && t > (1.0f - taperEnd)) {
    result *= (1.0f - t) / taperEnd;
  }
  return result;
}

void StrokeProcessor::feedSegment(
    const FPoint& from, const FPoint& to,
    float spacing, float baseRadius,
    float pressureFrom, float pressureTo,
    float strokeT, float strokeLen,
    float taperStart, float taperEnd,
    const DabCallback& callback)
{
  const float spacingPx = std::max(0.5f, spacing * baseRadius * 2.0f);

  // ── Catmull-Rom 制御点を決定 ─────────────────────────────────────────────
  const FPoint p0 = m_hasPrevPoint
      ? m_prevPoint
      : FPoint{from.x * 2.0f - to.x, from.y * 2.0f - to.y};
  const FPoint p1 = from;
  const FPoint p2 = to;
  const FPoint p3 = FPoint{to.x * 2.0f - from.x, to.y * 2.0f - from.y};

  // ── CR 弧長を推定（8 サンプル） ──────────────────────────────────────────
  static constexpr int kArcSamples = 8;
  float crArcLen = 0.0f;
  {
    FPoint prev = p1;
    for (int i = 1; i <= kArcSamples; ++i) {
      const FPoint curr = catmullRomPos(p0, p1, p2, p3,
                                         static_cast<float>(i) / kArcSamples);
      const float dx = curr.x - prev.x, dy = curr.y - prev.y;
      crArcLen += std::sqrt(dx*dx + dy*dy);
      prev = curr;
    }
  }

  // 極短セグメント: 単発 dab
  if (crArcLen < 0.001f) {
    const float taperScale = strokeLen > 0.0f
        ? computeTaper(std::clamp(strokeT / strokeLen, 0.0f, 1.0f), taperStart, taperEnd)
        : 1.0f;
    callback(DabRequest{from, pressureFrom, taperScale});
    m_prevPoint    = from;
    m_hasPrevPoint = true;
    return;
  }

  float traveled = spacingPx - m_distanceAccum;
  if (traveled < 0.0f) traveled = 0.0f;
  if (traveled > crArcLen) {
    m_distanceAccum += crArcLen;
    m_prevPoint    = from;
    m_hasPrevPoint = true;
    return;
  }

  // ── CR 曲線を細かく刻んでスタンプ位置を決定 ──────────────────────────────
  const int numSub = std::clamp(static_cast<int>(crArcLen * 2.0f), 8, 400);
  FPoint walkPrev  = p1;
  float  walkedLen = 0.0f;

  for (int step = 1; step <= numSub; ++step) {
    const float t         = static_cast<float>(step) / static_cast<float>(numSub);
    const FPoint walkCurr = catmullRomPos(p0, p1, p2, p3, t);
    const float dxS = walkCurr.x - walkPrev.x;
    const float dyS = walkCurr.y - walkPrev.y;
    const float stepLen = std::sqrt(dxS*dxS + dyS*dyS);
    walkedLen += stepLen;

    while (traveled <= walkedLen + 0.001f && traveled <= crArcLen + 0.001f) {
      const float alpha = (stepLen > 0.001f)
          ? std::clamp((traveled - (walkedLen - stepLen)) / stepLen, 0.0f, 1.0f)
          : 1.0f;
      const FPoint pos {
          walkPrev.x + (walkCurr.x - walkPrev.x) * alpha,
          walkPrev.y + (walkCurr.y - walkPrev.y) * alpha
      };

      const float tNorm    = std::clamp(traveled / crArcLen, 0.0f, 1.0f);
      const float pressure = pressureFrom + (pressureTo - pressureFrom) * tNorm;
      const float taperScale = strokeLen > 0.0f
          ? computeTaper(
                std::clamp((strokeT + traveled) / strokeLen, 0.0f, 1.0f),
                taperStart, taperEnd)
          : 1.0f;

      callback(DabRequest{pos, pressure, taperScale});
      traveled += spacingPx;
    }

    walkPrev = walkCurr;
  }

  // 次セグメントへの繰り越し距離
  m_distanceAccum = crArcLen - (traveled - spacingPx);
  if (m_distanceAccum < 0.0f) m_distanceAccum = 0.0f;

  m_prevPoint    = from;
  m_hasPrevPoint = true;
}

} // namespace core
