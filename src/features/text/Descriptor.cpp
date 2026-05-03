#include "features/text/Descriptor.h"

namespace features::text {

app::ui::ToolDescriptor makeTextToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::MoveLayer;
  descriptor.id = "text";
  descriptor.displayName = "Text";
  descriptor.guide = "Place a simple editable text marker object.";
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "text_basic";
  subTool.displayName = "Text";
  subTool.guide = "Click to place text marker.";
  subTool.profile.targetLayerKind = app::ui::TargetLayerKind::Both;
  subTool.editableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::text
