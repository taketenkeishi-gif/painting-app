#pragma once

#include <vector>

#include "core/common/FPoint.h"
#include "core/common/Point.h"
#include "core/tools/ITool.h"
#include "core/tools/ToolType.h"

namespace core {

class CurveTool : public ITool {
public:
  void setSnapAngleDegrees(int snapAngleDegrees) noexcept { m_snapAngleDegrees = snapAngleDegrees < 0 ? 0 : snapAngleDegrees; }
  void setSimplifyLevel(int level) noexcept { m_simplifyLevel = level < 0 ? 0 : level; }

  ToolKind kind() const noexcept override { return ToolKind::Shape; }
  std::string_view displayName() const noexcept override { return "Curve"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;
  ToolOverlayState overlay() const override;

private:
  Point snappedPoint(const Point& start, const Point& rawEnd, bool shiftConstraint) const;
  void addVectorBezierCurve(Layer& layer, const std::vector<FPoint>& curvePoints, const Color& color, int size) const;
  std::vector<FPoint> generateBezierPoints(const Point& p0, const FPoint& p1, const FPoint& p2, const Point& p3) const;

  bool m_drawing {false};
  int m_snapAngleDegrees {0};
  int m_simplifyLevel {0};
  Point m_start {0, 0};
  FPoint m_handle1 {0.0f, 0.0f};
  FPoint m_handle2 {0.0f, 0.0f};
  Point m_current {0, 0};
};

} // namespace core
