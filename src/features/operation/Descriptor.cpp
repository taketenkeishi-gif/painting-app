#include "features/operation/Descriptor.h"

namespace features::operation {

app::ui::ToolDescriptor makeOperationToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::MoveLayer;
  descriptor.id = "operation";
  descriptor.displayName = "Operation";
  descriptor.guide = "Select and move objects or layer contents.";
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "operation_object";
  subTool.displayName = "Operation";
  subTool.guide = "Drag to move active object/layer.";
  subTool.editableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::operation
