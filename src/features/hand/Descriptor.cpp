#include "features/hand/Descriptor.h"

#include <memory>

#include "core/tools/HandTool.h"
#include "features/common/DescriptorUtils.h"

namespace features::hand {

core::registry::ToolEntry makeHandToolEntry() {
  core::registry::ToolEntry entry;
  entry.id = kToolId;
  entry.kind = core::ToolKind::Hand;
  entry.displayName = "Hand";
  entry.factory = []() {
    return std::make_unique<core::HandTool>();
  };
  return entry;
}

app::ui::ToolDescriptor makeHandToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Hand;
  descriptor.id = "hand";
  descriptor.displayName = "Hand";
  descriptor.subTools = {features::common::makeSubTool(
      "hand_default",
      "Pan View",
      app::ui::BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Both, app::ui::CursorStyle::Hand},
      {},
      "Drag to pan viewport.")};
  descriptor.availableProperties = {};
  descriptor.guide = "Pan viewport.";
  return descriptor;
}

} // namespace features::hand
