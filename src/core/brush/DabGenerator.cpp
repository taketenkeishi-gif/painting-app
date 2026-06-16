#include "core/brush/DabGenerator.h"

#include <cmath>

namespace {
  constexpr float kPi = 3.14159265358979f;
} // namespace

namespace core {

// ── LCG 疑似乱数 ─────────────────────────────────────────────────────────────
// BrushTool.cpp 旧 anonymous namespace と完全に同一のパラメーター。
// 決定論的シーケンスを保証するためパラメーター変更禁止。
uint32_t DabGenerator::lcgNext(uint32_t& seed) noexcept {
  seed = seed * 1664525u + 1013904223u;
  return seed;
}

float DabGenerator::lcgFloat(uint32_t& seed) noexcept {
  return static_cast<float>(lcgNext(seed) >> 8) / static_cast<float>(0x00FFFFFFu);
}

// ── ストローク開始 ────────────────────────────────────────────────────────────
void DabGenerator::beginStroke(uint32_t seed) noexcept {
  m_seed = seed;
}

// ── Dab 配置生成 ──────────────────────────────────────────────────────────────
// BrushTool::stampDabsAt の scatter / angleJitter / dabCount ループと
// 完全に同一の LCG 消費順序を保つ（アルゴリズム変更禁止）。
void DabGenerator::generate(
    int   count,
    float radius,
    bool  scatterEnabled, float scatterAmount,
    bool  angleJitter,    float angleJitterAmount,
    const PlacementCallback& callback)
{
  for (int d = 0; d < count; ++d) {
    DabPlacement placement{{0.0f, 0.0f}, 0.0f};

    if (scatterEnabled) {
      // 円内一様分布: rejection-free disk sampling
      // LCG 消費順: 1 個目 → r 計算, 2 個目 → 角度計算
      const float r = radius * scatterAmount * std::sqrt(lcgFloat(m_seed));
      const float a = lcgFloat(m_seed) * (2.0f * kPi);
      placement.offset.x = r * std::cos(a);
      placement.offset.y = r * std::sin(a);
    }

    if (angleJitter) {
      // ±angleJitterAmount 度のランダム回転
      placement.angle = (lcgFloat(m_seed) * 2.0f - 1.0f) * angleJitterAmount;
    }

    callback(placement);
  }
}

} // namespace core
