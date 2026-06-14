#pragma once

#include <chrono>
#include <functional>

#include "core/common/FPoint.h"

namespace core {

// ── DabRequest ───────────────────────────────────────────────────────────────
// StrokeProcessor が stampDabsAt に渡すパラメーター。
// radius / strength は StrokeProcessor が算出した値を含む。
// BrushTool 側で computePressureSize() 等を使って最終 radius を決定する。
struct DabRequest {
  FPoint   pos;
  float    pressure;   ///< 補間済み筆圧 (0-1)
  float    taperScale; ///< テーパー係数 (0-1)
};

// ── StrokeProcessor ──────────────────────────────────────────────────────────
// Stroke → Dab 配置責務を BrushTool から分離したクラス。
//
// 担当:
//   - applyStabilization (指数スムージング)
//   - Catmull-Rom 弧長推定 + サブステッピング
//   - spacing に基づく dab 配置タイミング
//   - m_distanceAccum (セグメント間繰り越し距離)
//   - m_prevPoint / m_hasPrevPoint (CR 制御点保持)
//   - stroke 全体長/位置によるテーパー計算補助
//
// 非担当 (BrushTool が保持):
//   - stampAt / stampDabsAt / blendPixel
//   - computePressureSize / computePressureOpacity / computeVelocityFactor
//   - m_strokeAccum / m_smearColor / m_dabRandSeed
//   - 描画アルゴリズム全般

class StrokeProcessor {
public:
  using DabCallback = std::function<void(const DabRequest&)>;

  // ── ストローク開始時にリセット ────────────────────────────────────────────
  void beginStroke(const FPoint& startPt) noexcept;

  // ── ポインター移動ごとに呼び出す ─────────────────────────────────────────
  // stabilization を適用した点を返す（BrushTool / EraserTool が m_lastPoint 更新に使う）
  // velocityBasedCorrection: true のとき移動距離が大きいほど追従を鈍らせる
  FPoint applyStabilization(const FPoint& from, const FPoint& to,
                             float stabilization,
                             bool  velocityBasedCorrection = false) const noexcept;

  // ── セグメントを処理して dab 位置を決定する ───────────────────────────────
  // spacing: BrushSettings.spacing
  // baseRadius: BrushSettings.size * 0.5
  // pressureFrom/To: セグメント両端の筆圧
  // strokeT: このセグメント開始時点でのストローク累積長
  // strokeLen: ストローク全体長 (テーパー計算用; 0 = 無効)
  // taperStart/End: BrushSettings.taperStart/End
  // callback: dab 1 個ごとに呼ばれる
  void feedSegment(
      const FPoint& from, const FPoint& to,
      float spacing, float baseRadius,
      float pressureFrom, float pressureTo,
      float strokeT, float strokeLen,
      float taperStart, float taperEnd,
      const DabCallback& callback);

private:
  // CR 用
  FPoint m_prevPoint    {0.0f, 0.0f};
  bool   m_hasPrevPoint {false};

  // spacing 繰り越し
  mutable float m_distanceAccum {0.0f};

  float computeTaper(float t, float taperStart, float taperEnd) const noexcept;
};

} // namespace core
