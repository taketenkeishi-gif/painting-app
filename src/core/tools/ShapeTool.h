#pragma once

#include <memory>
#include <vector>

#include "core/common/FPoint.h"
#include "core/common/Point.h"
#include "core/tools/ITool.h"
#include "core/tools/LineTool.h"
#include "core/tools/CurveTool.h"

namespace core {

class ShapeTool : public ITool {
public:
  enum class ShapeMode {
    Line,
    Curve,
  };

  ShapeTool();

  void setShapeMode(ShapeMode mode) noexcept { m_mode = mode; }
  void setSnapAngleDegrees(int snapAngleDegrees) noexcept;
  void setSimplifyLevel(int level) noexcept;

  ToolKind kind() const noexcept override { return ToolKind::Shape; }
  std::string_view displayName() const noexcept override { return "Shape"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;
  ToolOverlayState overlay() const override;

private:
  ShapeMode m_mode {ShapeMode::Line};
  std::unique_ptr<LineTool> m_lineTool;
  std::unique_ptr<CurveTool> m_curveTool;

  ITool* activeTool() const noexcept;
};

} // namespace core
