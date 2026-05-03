#include "features/liquify/Descriptor.h"

namespace features::liquify {

app::ui::ToolDescriptor makeLiquifyToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::MoveLayer;
  descriptor.id = "liquify";
  descriptor.displayName = "Liquify";
  descriptor.guide = "Push nearby pixels along drag direction.";
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "liquify_push";
  subTool.displayName = "Liquify";
  subTool.guide = "Drag to push pixels.";
  subTool.profile.targetLayerKind = app::ui::TargetLayerKind::Raster;
  subTool.editableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::liquify
