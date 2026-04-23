#pragma once

#include <algorithm>
#include <string_view>
#include <vector>

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
  void setSpacing(float spacing) noexcept { m_settings.spacing = std::clamp(spacing, 0.01F, 3.0F); }
  void setAntiAlias(bool antiAlias) noexcept { m_settings.antiAlias = antiAlias; }
  void setStabilization(float stabilization) noexcept { m_settings.stabilization = std::clamp(stabilization, 0.0F, 1.0F); }
  void setPostCorrection(bool postCorrection) noexcept { m_settings.postCorrection = postCorrection; }
  void setVelocityBasedCorrection(bool enabled) noexcept { m_settings.velocityBasedCorrection = enabled; }
  void setShapeType(BrushShapeType shapeType) noexcept { m_settings.shapeType = shapeType; }
  void setBlendMode(BlendMode blendMode) noexcept { m_settings.blendMode = blendMode; }
  void setEraseMode(bool eraseMode) noexcept { m_settings.eraseMode = eraseMode; }
  void setLockAlphaRespect(bool lockAlpha) noexcept { m_settings.lockAlphaRespect = lockAlpha; }

  const BrushSettings& settings() const noexcept { return m_settings; }

  void stroke(Layer& layer, const Point& from, const Point& to) const;

  ToolKind kind() const noexcept override { return ToolKind::Brush; }
  std::string_view displayName() const noexcept override { return "Brush"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;

private:
  Point applyStabilization(const Point& from, const Point& to) const;
  void stampCircle(PixelBuffer& buffer, const Point& center, int radius, const Color& color, bool lockAlpha) const;
  void stampSquare(PixelBuffer& buffer, const Point& center, int radius, const Color& color, bool lockAlpha) const;
  void blendPixel(PixelBuffer& buffer, int x, int y, const Color& src, float strength, bool lockAlpha) const;

  BrushSettings m_settings;
  bool m_drawing {false};
  Point m_lastPoint {0, 0};
  std::vector<Point> m_vectorPoints;
};

} // namespace core
