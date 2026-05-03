#include "features/color_mix/Descriptor.h"

namespace features::color_mix {

app::ui::ToolDescriptor makeColorMixToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Brush;
  descriptor.id = "color_mix";
  descriptor.displayName = "Color Mix";
  descriptor.guide = "Blend nearby colors by dragging.";
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "color_mix_blend";
  subTool.displayName = "Color Mix";
  subTool.guide = "Drag to smear and blend colors.";
  subTool.profile.targetLayerKind = app::ui::TargetLayerKind::Raster;
  subTool.editableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::color_mix
