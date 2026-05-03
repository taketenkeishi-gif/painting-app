#include "features/clone_stamp/Descriptor.h"

namespace features::clone_stamp {

app::ui::ToolDescriptor makeCloneStampToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Brush;
  descriptor.id = "clone_stamp";
  descriptor.displayName = "Clone Stamp";
  descriptor.guide = "Copy sampled pixels from source point.";
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "clone_stamp_basic";
  subTool.displayName = "Clone Stamp";
  subTool.guide = "Alt-click to set source, then paint clone.";
  subTool.profile.targetLayerKind = app::ui::TargetLayerKind::Raster;
  subTool.editableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::clone_stamp
