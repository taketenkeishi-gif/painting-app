#include "features/move_layer/Descriptor.h"

#include <memory>

#include "core/tools/MoveLayerTool.h"
#include "features/common/DescriptorUtils.h"

namespace features::move_layer {

core::registry::ToolEntry makeMoveLayerToolEntry() {
  core::registry::ToolEntry entry;
  entry.id = kToolId;
  entry.kind = core::ToolKind::MoveLayer;
  entry.displayName = "Move Layer";
  entry.factory = []() {
    return std::make_unique<core::MoveLayerTool>();
  };
  return entry;
}

app::ui::ToolDescriptor makeMoveLayerToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::MoveLayer;
  descriptor.id = "move_layer";
  descriptor.displayName = "Move Layer";
  descriptor.subTools = {features::common::makeSubTool(
      "move_layer_default",
      "Pixel/Vector Offset",
      app::ui::BrushPreset {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Both, app::ui::CursorStyle::Hand},
      {},
      "Drag to offset active layer content.")};
  descriptor.availableProperties = {};
  descriptor.guide = "Move active layer pixels or vector paths.";
  return descriptor;
}

} // namespace features::move_layer
