#include "features/comic/Descriptor.h"

namespace features::comic {

app::ui::ToolDescriptor makeComicToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::RectSelection;
  descriptor.id = "comic";
  descriptor.displayName = "Comic";
  descriptor.guide = "Create comic panel frame by dragging.";
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "comic_panel";
  subTool.displayName = "Comic";
  subTool.guide = "Drag to create panel frame.";
  subTool.editableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::comic
