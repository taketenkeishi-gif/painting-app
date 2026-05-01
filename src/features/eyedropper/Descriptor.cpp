#include "features/eyedropper/Descriptor.h"

#include <memory>

#include "core/tools/EyedropperTool.h"
#include "features/common/DescriptorUtils.h"

namespace features::eyedropper {

core::registry::ToolEntry makeEyedropperToolEntry() {
  core::registry::ToolEntry entry;
  entry.id = kToolId;
  entry.kind = core::ToolKind::Eyedropper;
  entry.displayName = "Eyedropper";
  entry.factory = []() {
    return std::make_unique<core::EyedropperTool>();
  };
  return entry;
}

app::ui::ToolDescriptor makeEyedropperToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Eyedropper;
  descriptor.id = "eyedropper";
  descriptor.displayName = "Eyedropper";
  descriptor.subTools = {features::common::makeSubTool(
      "eyedropper_default",
      "Sample Composite",
      app::ui::BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Both, app::ui::CursorStyle::Cross},
      {},
      "Click canvas to sample color.")};
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Color};
  descriptor.guide = "Pick a color from composited result.";
  return descriptor;
}

} // namespace features::eyedropper
