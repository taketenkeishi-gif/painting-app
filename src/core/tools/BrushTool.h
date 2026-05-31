#pragma once

#include <algorithm>
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
  void setPressureOpacityEnabled(bool enabled) noexcept { m_settings.dynamics.pressureOpacity = enabled; }

  const BrushSettings& settings() const noexcept { return m_settings; }

  ToolKind kind() const noexcept override { return ToolKind::Brush; }
  std::string_view displayName() const noexcept override { return "Brush"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;

protected:
  FPoint applyStabilization(const FPoint& from, const FPoint& to) const;
  float computeTaperStrength(float t, float taperStart, float taperEnd) const;
  float computePressureSize(float pressure) const;
  float computePressureOpacity(float pressure) const;

  void strokeSegment(Layer& layer, const FPoint& from, const FPoint& to,
                     float pressureFrom, float pressureTo,
                     float strokeT, float strokeLen) const;

  void stampAt(PixelBuffer& buffer, const FPoint& center, float radius,
               float strength, bool lockAlpha) const;

  void blendPixel(PixelBuffer& buffer, int x, int y,
                  const Color& src, float strength, bool lockAlpha) const;

  // ストローク内opacity制御用バッファ（buildup=falseのとき使用）
  mutable PixelBuffer m_strokeAccum;
  mutable bool m_strokeAccumDirty {false};

  BrushSettings m_settings;
  bool m_drawing {false};
  FPoint m_lastPoint {0.0f, 0.0f};
  float m_lastPressure {1.0f};
  mutable float m_distanceAccum {0.0f};
  float m_strokeLength {0.0f};
  std::vector<FPoint> m_vectorPoints;
};

} // namespace core
