#include "features/sketch/Descriptor.h"

namespace features::sketch {

app::ui::ToolDescriptor makeSketchToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Brush;
  descriptor.id = "sketch";
  descriptor.displayName = "Sketch";
  descriptor.guide = "Pencil-like brush for rough drawing.";
  descriptor.availableProperties = {
      app::ui::ToolPropertyKey::Size,
      app::ui::ToolPropertyKey::Opacity,
      app::ui::ToolPropertyKey::Flow,
      app::ui::ToolPropertyKey::Hardness};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "sketch_pencil";
  subTool.displayName = "Sketch";
  subTool.guide = "Low-opacity pencil stroke.";
  subTool.profile.stroke.flow = 62;
  subTool.profile.stroke.opacity = 70;
  subTool.profile.shape.hardness = 85;
  subTool.profile.targetLayerKind = app::ui::TargetLayerKind::Raster;
  subTool.preset.flow = 62;
  subTool.preset.opacity = 70;
  subTool.preset.hardness = 85;
  subTool.preset.targetLayerKind = app::ui::TargetLayerKind::Raster;
  subTool.editableProperties = {
      app::ui::ToolPropertyKey::Size,
      app::ui::ToolPropertyKey::Opacity,
      app::ui::ToolPropertyKey::Flow,
      app::ui::ToolPropertyKey::Hardness};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::sketch
