#include "features/gradient/Descriptor.h"

namespace features::gradient {

app::ui::ToolDescriptor makeGradientToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Fill;
  descriptor.id = "gradient";
  descriptor.displayName = "Gradient";
  descriptor.guide = "Linear gradient fill from drag start to end.";
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "gradient_linear";
  subTool.displayName = "Gradient";
  subTool.guide = "Drag to apply linear gradient on raster layers.";
  subTool.profile.targetLayerKind = app::ui::TargetLayerKind::Raster;
  subTool.editableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::gradient
