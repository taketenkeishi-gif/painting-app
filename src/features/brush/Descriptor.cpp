#include "features/brush/Descriptor.h"

#include <memory>

#include "core/tools/BrushTool.h"
#include "features/common/DescriptorUtils.h"

namespace features::brush {

core::registry::ToolEntry makeBrushToolEntry() {
  core::registry::ToolEntry entry;
  entry.id = kToolId;
  entry.kind = core::ToolKind::Brush;
  entry.displayName = "Brush";
  entry.factory = []() {
    return std::make_unique<core::BrushTool>();
  };
  return entry;
}

app::ui::ToolDescriptor makeBrushToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Brush;
  descriptor.id = "brush";
  descriptor.displayName = "Brush";
  descriptor.subTools = {
      features::common::makeSubTool(
          "brush_normal",
          "Normal",
          app::ui::BrushPreset {8, 100, 100, 100, 25, true, 35, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Raster, app::ui::CursorStyle::Brush},
          {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::Flow, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::PostCorrection, app::ui::ToolPropertyKey::VelocityCorrection, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::BlendMode, app::ui::ToolPropertyKey::EraseMode, app::ui::ToolPropertyKey::LockAlphaRespect},
          "LMB drag to paint. Wheel or [ ] adjusts size."),
      features::common::makeSubTool(
          "brush_hard",
          "Hard",
          app::ui::BrushPreset {6, 100, 100, 100, 18, false, 20, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Raster, app::ui::CursorStyle::Brush},
          {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::Flow, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::PostCorrection, app::ui::ToolPropertyKey::VelocityCorrection, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::BlendMode, app::ui::ToolPropertyKey::EraseMode, app::ui::ToolPropertyKey::LockAlphaRespect},
          "Crisp edge stroke with tighter spacing."),
      features::common::makeSubTool(
          "brush_soft",
          "Soft",
          app::ui::BrushPreset {12, 65, 35, 80, 35, true, 50, true, true, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Raster, app::ui::CursorStyle::Brush},
          {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::Flow, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::PostCorrection, app::ui::ToolPropertyKey::VelocityCorrection, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::BlendMode, app::ui::ToolPropertyKey::EraseMode, app::ui::ToolPropertyKey::LockAlphaRespect},
          "Soft edge brush for blending."),
      features::common::makeSubTool(
          "brush_airbrush",
          "Airbrush",
          app::ui::BrushPreset {24, 28, 10, 35, 12, true, 60, true, true, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Raster, app::ui::CursorStyle::Brush},
          {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::Flow, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::PostCorrection, app::ui::ToolPropertyKey::VelocityCorrection, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::BlendMode, app::ui::ToolPropertyKey::EraseMode, app::ui::ToolPropertyKey::LockAlphaRespect},
          "Low-flow brush for gradual buildup."),
      features::common::makeSubTool(
          "brush_marker",
          "Marker",
          app::ui::BrushPreset {14, 92, 82, 65, 20, true, 28, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 8, 12, app::ui::TargetLayerKind::Raster, app::ui::CursorStyle::Brush},
          {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::Flow, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::PostCorrection, app::ui::ToolPropertyKey::VelocityCorrection, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::BlendMode, app::ui::ToolPropertyKey::EraseMode, app::ui::ToolPropertyKey::LockAlphaRespect},
          "Opaque marker-style brush with mild pressure response.")};
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity, app::ui::ToolPropertyKey::Hardness, app::ui::ToolPropertyKey::Flow, app::ui::ToolPropertyKey::Spacing, app::ui::ToolPropertyKey::AntiAlias, app::ui::ToolPropertyKey::Stabilization, app::ui::ToolPropertyKey::PostCorrection, app::ui::ToolPropertyKey::VelocityCorrection, app::ui::ToolPropertyKey::ShapeType, app::ui::ToolPropertyKey::BlendMode, app::ui::ToolPropertyKey::EraseMode, app::ui::ToolPropertyKey::LockAlphaRespect};
  descriptor.guide = "Draw on active layer.";
  return descriptor;
}

} // namespace features::brush
