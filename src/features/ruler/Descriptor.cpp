#include "features/ruler/Descriptor.h"

namespace features::ruler {

app::ui::ToolDescriptor makeRulerToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Line;
  descriptor.id = "ruler";
  descriptor.displayName = "Ruler";
  descriptor.guide = "Create a non-printing straight ruler guide by dragging.";
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "ruler_straight";
  subTool.displayName = "Ruler";
  subTool.guide = "Drag to place a movable drawing guide.";
  subTool.profile.targetLayerKind = app::ui::TargetLayerKind::Both;
  subTool.editableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::ruler
