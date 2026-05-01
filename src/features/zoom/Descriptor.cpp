#include "features/zoom/Descriptor.h"

#include <memory>

#include "core/tools/ZoomTool.h"
#include "features/common/DescriptorUtils.h"

namespace features::zoom {

core::registry::ToolEntry makeZoomToolEntry() {
  core::registry::ToolEntry entry;
  entry.id = kToolId;
  entry.kind = core::ToolKind::Zoom;
  entry.displayName = "Zoom";
  entry.factory = []() {
    return std::make_unique<core::ZoomTool>();
  };
  return entry;
}

app::ui::ToolDescriptor makeZoomToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Zoom;
  descriptor.id = "zoom";
  descriptor.displayName = "Zoom";
  descriptor.subTools = {features::common::makeSubTool(
      "zoom_default",
      "Wheel Zoom",
      app::ui::BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Both, app::ui::CursorStyle::Zoom},
      {},
      "Use Ctrl+Wheel to zoom.")};
  descriptor.availableProperties = {};
  descriptor.guide = "Zoom viewport.";
  return descriptor;
}

} // namespace features::zoom
