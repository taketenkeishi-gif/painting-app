#include "features/line/Descriptor.h"

#include <memory>

#include "core/tools/LineTool.h"
#include "features/common/DescriptorUtils.h"

namespace features::line {

core::registry::ToolEntry makeLineToolEntry() {
  core::registry::ToolEntry entry;
  entry.id = kToolId;
  entry.kind = core::ToolKind::Line;
  entry.displayName = "Line";
  entry.factory = []() {
    return std::make_unique<core::LineTool>();
  };
  return entry;
}

app::ui::ToolDescriptor makeLineToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Line;
  descriptor.id = "line";
  descriptor.displayName = "Line";
  descriptor.subTools = {
      features::common::makeSubTool(
          "line_raster",
          "Raster Line",
          app::ui::BrushPreset {8, 100, 100, 100, 25, true, 10, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Raster, app::ui::CursorStyle::Cross, 0, 0, 8},
          {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::StrokeWidth, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::BlendMode, app::ui::ToolPropertyKey::Angle},
          "Drag to draw straight raster line."),
      features::common::makeSubTool(
          "line_vector",
          "Vector Basic",
          app::ui::BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Vector, app::ui::CursorStyle::Cross, 0, 0, 8},
          {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::StrokeWidth, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::SnapAngle, app::ui::ToolPropertyKey::SimplifyLevel},
          "Drag to create vector line path."),
      features::common::makeSubTool(
          "line_vector_snap",
          "Vector Snap",
          app::ui::BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Vector, app::ui::CursorStyle::Cross, 15, 0, 8},
          {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::StrokeWidth, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::SnapAngle, app::ui::ToolPropertyKey::SimplifyLevel},
          "Drag to create snapped vector line."),
      features::common::makeSubTool(
          "line_vector_thick",
          "Vector Thick",
          app::ui::BrushPreset {16, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Vector, app::ui::CursorStyle::Cross, 0, 0, 16},
          {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::StrokeWidth, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::SnapAngle, app::ui::ToolPropertyKey::SimplifyLevel},
          "Drag to create thick vector line."),
      features::common::makeSubTool(
          "line_vector_thin",
          "Vector Thin",
          app::ui::BrushPreset {3, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Vector, app::ui::CursorStyle::Cross, 0, 0, 3},
          {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::StrokeWidth, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::SnapAngle, app::ui::ToolPropertyKey::SimplifyLevel},
          "Drag to create thin vector line.")};
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::StrokeWidth, app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::BlendMode, app::ui::ToolPropertyKey::SnapAngle, app::ui::ToolPropertyKey::SimplifyLevel, app::ui::ToolPropertyKey::Angle};
  descriptor.guide = "Draw straight lines.";
  return descriptor;
}

} // namespace features::line
