#include "features/airbrush/Descriptor.h"

namespace features::airbrush {

app::ui::ToolDescriptor makeAirbrushToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Brush;
  descriptor.id = "airbrush";
  descriptor.displayName = "Airbrush";
  descriptor.guide = "Soft low-density brush for spray effect.";
  descriptor.availableProperties = {
      app::ui::ToolPropertyKey::Size,
      app::ui::ToolPropertyKey::Opacity,
      app::ui::ToolPropertyKey::Flow,
      app::ui::ToolPropertyKey::Spacing,
      app::ui::ToolPropertyKey::Hardness};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "airbrush_soft";
  subTool.displayName = "Airbrush";
  subTool.guide = "Soft airbrush stroke.";
  subTool.profile.stroke.flow = 35;
  subTool.profile.stroke.spacing = 8;
  subTool.profile.shape.hardness = 20;
  subTool.profile.targetLayerKind = app::ui::TargetLayerKind::Raster;
  subTool.preset.flow = 35;
  subTool.preset.spacing = 8;
  subTool.preset.hardness = 20;
  subTool.preset.targetLayerKind = app::ui::TargetLayerKind::Raster;
  subTool.editableProperties = {
      app::ui::ToolPropertyKey::Size,
      app::ui::ToolPropertyKey::Opacity,
      app::ui::ToolPropertyKey::Flow,
      app::ui::ToolPropertyKey::Spacing,
      app::ui::ToolPropertyKey::Hardness};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::airbrush
