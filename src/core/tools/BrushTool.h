#pragma once

#include <algorithm>
#include <chrono>
#include <string_view>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/common/FPoint.h"
#include "core/common/Point.h"
#include "core/layer/Layer.h"
#include "core/tools/ITool.h"
#include "core/tools/ToolTypes.h"

namespace core {

class BrushTool : public ITool {
public:
  void setColor(const Color& color) noexcept { m_settings.color = color; }
  void setSize(int size) noexcept { m_settings.size = size < 1 ? 1 : size; }
  void setOpacity(float opacity) noexcept { m_settings.opacity = std::clamp(opacity, 0.0F, 1.0F); }
  void setHardness(float hardness) noexcept { m_settings.hardness = std::clamp(hardness, 0.0F, 1.0F); }
  void setFlow(float flow) noexcept { m_settings.flow = std::clamp(flow, 0.0F, 1.0F); }
  void setSpacing(float spacing) noexcept { m_settings.spacing = std::clamp(spacing, 0.001F, 3.0F); }
  void setAntiAlias(bool antiAlias) noexcept { m_settings.antiAlias = antiAlias; }
  void setStabilization(float stabilization) noexcept { m_settings.stabilization = std::clamp(stabilization, 0.0F, 1.0F); }
  void setPostCorrection(bool postCorrection) noexcept { m_settings.postCorrection = postCorrection; }
  void setVelocityBasedCorrection(bool enabled) noexcept { m_settings.velocityBasedCorrection = enabled; }
  void setShapeType(BrushShapeType shapeType) noexcept { m_settings.shapeType = shapeType; }
  void setAngle(float angle) noexcept { m_settings.angle = angle; }
  void setRoundness(float roundness) noexcept { m_settings.roundness = std::clamp(roundness, 0.01F, 1.0F); }
  void setTaperStart(float taper) noexcept { m_settings.taperStart = std::clamp(taper, 0.0F, 1.0F); }
  void setTaperEnd(float taper) noexcept { m_settings.taperEnd = std::clamp(taper, 0.0F, 1.0F); }
  void setBlendMode(BlendMode blendMode) noexcept { m_settings.blendMode = blendMode; }
  void setEraseMode(bool eraseMode) noexcept { m_settings.eraseMode = eraseMode; }
  void setLockAlphaRespect(bool lockAlpha) noexcept { m_settings.lockAlphaRespect = lockAlpha; }
  void setBuildupMode(bool buildup) noexcept { m_settings.buildupMode = buildup; }
  void setPressureSizeEnabled(bool enabled) noexcept { m_settings.dynamics.pressureSize = enabled; }
  void setPressureSizeMin(float min) noexcept { m_settings.dynamics.pressureSizeMin = std::clamp(min, 0.0F, 1.0F); }
  void setPressureOpacityEnabled(bool enabled) noexcept { m_settings.dynamics.pressureOpacity = enabled; }
  void setPressureOpacityMin(float min) noexcept { m_settings.dynamics.pressureOpacityMin = std::clamp(min, 0.0F, 1.0F); }

  // 速度感応
  void setVelocitySizeEnabled(bool v)    noexcept { m_settings.dynamics.velocitySize    = v; }
  void setVelocitySizeMin(float v)       noexcept { m_settings.dynamics.velocitySizeMin = std::clamp(v, 0.0F, 1.0F); }
  void setVelocityOpacityEnabled(bool v) noexcept { m_settings.dynamics.velocityOpacity    = v; }
  void setVelocityOpacityMin(float v)    noexcept { m_settings.dynamics.velocityOpacityMin = std::clamp(v, 0.0F, 1.0F); }

  // テクスチャグレイン
  void setTextureGrainEnabled(bool v)  noexcept { m_settings.dynamics.textureGrain    = v; }
  void setTextureStrength(float v)     noexcept { m_settings.dynamics.textureStrength = std::clamp(v, 0.0F, 1.0F); }
  void setTextureScale(float v)        noexcept { m_settings.dynamics.textureScale    = std::clamp(v, 0.1F, 4.0F); }

  // ウェットミックス / スメア
  void setWetMixEnabled(bool v)  noexcept { m_settings.dynamics.wetMix    = v; }
  void setWetMixRate(float v)    noexcept { m_settings.dynamics.wetMixRate = std::clamp(v, 0.0F, 1.0F); }
  void setSmearEnabled(bool v)   noexcept { m_settings.dynamics.smear     = v; }
  void setSmearRate(float v)     noexcept { m_settings.dynamics.smearRate  = std::clamp(v, 0.0F, 1.0F); }

  // 筆圧カーブ (libmypaint BrushCurve)
  // 例: brush.setPressureSizeCurve(BrushCurve::soft())
  void setPressureSizeCurve(const BrushCurve& c)    noexcept { m_settings.dynamics.pressureSizeCurve    = c; }
  void setPressureOpacityCurve(const BrushCurve& c) noexcept { m_settings.dynamics.pressureOpacityCurve = c; }
  // 後方互換: γ 値で設定するショートカット
  void setPressureSizeGamma(float gamma)    noexcept { m_settings.dynamics.pressureSizeCurve    = BrushCurve::fromGamma(gamma); }
  void setPressureOpacityGamma(float gamma) noexcept { m_settings.dynamics.pressureOpacityCurve = BrushCurve::fromGamma(gamma); }

  // Dab 散布
  void setScatterEnabled(bool v)   noexcept { m_settings.dynamics.scatter       = v; }
  void setScatterAmount(float v)   noexcept { m_settings.dynamics.scatterAmount = std::clamp(v, 0.0F, 4.0F); }

  // 角度ジッター
  void setAngleJitterEnabled(bool v)  noexcept { m_settings.dynamics.angleJitter       = v; }
  void setAngleJitterAmount(float v)  noexcept { m_settings.dynamics.angleJitterAmount = std::clamp(v, 0.0F, 180.0F); }

  // 1 スタンプあたりの Dab 数（スプレー）
  void setDabCount(int v) noexcept { m_settings.dynamics.dabCount = std::clamp(v, 1, 64); }

  const BrushSettings& settings() const noexcept { return m_settings; }

  ToolKind kind() const noexcept override { return ToolKind::Brush; }
  std::string_view displayName() const noexcept override { return "Brush"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;
  ToolOverlayState overlay() const override;

protected:
  FPoint applyStabilization(const FPoint& from, const FPoint& to) const;
  float computeTaperStrength(float t, float taperStart, float taperEnd) const;
  float computePressureSize(float pressure) const;
  float computePressureOpacity(float pressure) const;
  float computeVelocityFactor(float segLenPx) const noexcept;

  void strokeSegment(Layer& layer, const PixelBuffer& composited,
                     const FPoint& from, const FPoint& to,
                     float pressureFrom, float pressureTo,
                     float strokeT, float strokeLen);

  // angleDegrees: m_settings.angle に加算するジッター角度（通常は 0）
  void stampAt(PixelBuffer& buffer, const PixelBuffer& composited,
               const FPoint& center, float radius,
               float strength, bool lockAlpha,
               float angleDegrees) const;

  // scatter / dabCount を考慮して stampAt を 1〜N 回呼ぶ
  void stampDabsAt(PixelBuffer& buffer, const PixelBuffer& composited,
                   const FPoint& center, float radius,
                   float strength, bool lockAlpha) const;

  void blendPixel(PixelBuffer& buffer, int x, int y,
                  const Color& src, float strength, bool lockAlpha) const;

  // ストローク内opacity制御用バッファ（buildup=falseのとき使用）
  mutable PixelBuffer m_strokeAccum;
  mutable bool m_strokeAccumDirty {false};

  BrushSettings m_settings;
  bool m_drawing {false};
  bool m_maskEditMode {false};
  FPoint m_lastPoint {0.0f, 0.0f};
  float m_lastPressure {1.0f};
  mutable float m_distanceAccum {0.0f};
  float m_strokeLength {0.0f};
  std::vector<FPoint> m_vectorPoints;

  // 速度計算用
  using Clock = std::chrono::steady_clock;
  Clock::time_point m_lastMoveTime {};
  float m_currentVelocityPxMs {0.0f}; ///< px/ms (exponential smoothed)

  // スメア用: 前回の stamp 中心で採取した色
  mutable Color m_smearColor {0, 0, 0, 255};

  // Catmull-Rom スプライン: 前セグメントの始点を保持
  FPoint m_prevPoint    {0.0f, 0.0f};
  bool   m_hasPrevPoint {false};

  // scatter / angleJitter 用 LCG シード（ストロークごとにリセット）
  mutable uint32_t m_dabRandSeed {0};
};

} // namespace core
