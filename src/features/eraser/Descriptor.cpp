#include "features/eraser/Descriptor.h"

#include <memory>

#include "core/tools/EraserTool.h"
#include "features/common/DescriptorUtils.h"

namespace features::eraser {

core::registry::ToolEntry makeEraserToolEntry() {
  core::registry::ToolEntry entry;
  entry.id = kToolId;
  entry.kind = core::ToolKind::Eraser;
  entry.displayName = "Eraser";
  entry.factory = []() {
    return std::make_unique<core::EraserTool>();
  };
  return entry;
}

app::ui::ToolDescriptor makeEraserToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Eraser;
  descriptor.id = "eraser";
  descriptor.displayName = "Eraser";
  descriptor.subTools = {
      features::common::makeSubTool(
          "eraser_normal",
          "Normal",
          app::ui::BrushPreset {16, 100, 100, 100, 25, true, 35, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Raster, app::ui::CursorStyle::Brush},
          {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::Flow, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::PostCorrection, app::ui::ToolPropertyKey::VelocityCorrection, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::EraseMode},
          "Erase with hard edge."),
      features::common::makeSubTool(
          "eraser_soft",
          "Soft",
          app::ui::BrushPreset {22, 55, 25, 70, 28, true, 55, true, true, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Raster, app::ui::CursorStyle::Brush},
          {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::Flow, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::PostCorrection, app::ui::ToolPropertyKey::VelocityCorrection, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::EraseMode},
          "Erase softly with feathered edge."),
      features::common::makeSubTool(
          "eraser_vector_touch",
          "Vector Erase (Touched)",
          []() {
            app::ui::BrushPreset p {18, 100, 100, 100, 30, true, 30, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Vector, app::ui::CursorStyle::Cross};
            p.vectorEraseMode = app::ui::VectorEraserMode::TouchedOnly;
            p.vectorTrimOutside = false;
            return p;
          }(),
          {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::VectorEraseMode, app::ui::ToolPropertyKey::VectorTrimOutside},
          "Erase touched part of vector strokes."),
      features::common::makeSubTool(
          "eraser_vector_intersection",
          "Vector Erase (To Intersection)",
          []() {
            app::ui::BrushPreset p {18, 100, 100, 100, 30, true, 30, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Vector, app::ui::CursorStyle::Cross};
            p.vectorEraseMode = app::ui::VectorEraserMode::ToIntersection;
            p.vectorTrimOutside = false;
            return p;
          }(),
          {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::VectorEraseMode, app::ui::ToolPropertyKey::VectorTrimOutside},
          "Erase from touched point to nearest intersections."),
      features::common::makeSubTool(
          "eraser_vector_trim",
          "Vector Erase (Trim Outside)",
          []() {
            app::ui::BrushPreset p {18, 100, 100, 100, 30, true, 30, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Vector, app::ui::CursorStyle::Cross};
            p.vectorEraseMode = app::ui::VectorEraserMode::TrimOutside;
            p.vectorTrimOutside = true;
            return p;
          }(),
          {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::VectorEraseMode, app::ui::ToolPropertyKey::VectorTrimOutside},
          "Trim protruding vector segments."),
      features::common::makeSubTool(
          "eraser_vector_whole",
          "Vector Erase Whole",
          []() {
            app::ui::BrushPreset p {18, 100, 100, 100, 30, true, 30, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, true, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Vector, app::ui::CursorStyle::Cross};
            p.vectorEraseMode = app::ui::VectorEraserMode::TouchedOnly;
            p.vectorTrimOutside = false;
            return p;
          }(),
          {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::VectorEraseMode, app::ui::ToolPropertyKey::VectorTrimOutside},
          "Legacy vector eraser preset.")};
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::Flow, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::PostCorrection, app::ui::ToolPropertyKey::VelocityCorrection, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::EraseMode};
  descriptor.guide = "Erase pixels on active layer.";
  return descriptor;
}

} // namespace features::eraser
