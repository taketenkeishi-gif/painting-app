#include "features/line_correction/Descriptor.h"

namespace features::line_correction {

app::ui::ToolDescriptor makeLineCorrectionToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Line;
  descriptor.id = "line_correction";
  descriptor.displayName = "Line Correction";
  descriptor.guide = "Smooth the latest vector path.";
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "line_correction_smooth";
  subTool.displayName = "Line Correction";
  subTool.guide = "Click to smooth the latest vector path.";
  subTool.profile.targetLayerKind = app::ui::TargetLayerKind::Vector;
  subTool.editableProperties = {app::ui::ToolPropertyKey::Size, app::ui::ToolPropertyKey::Opacity};

  descriptor.subTools = {subTool};
  return descriptor;
}

} // namespace features::line_correction
