#include "features/pen/Descriptor.h"

#include <memory>

#include "features/common/DescriptorUtils.h"
#include "core/tools/PenTool.h"

namespace features::pen {
namespace {
app::ui::SubToolDescriptor makePenSubTool() {
  app::ui::BrushPreset preset {
      8,
      100,
      100,
      100,
      10,
      true,
      50,
      false,
      false,
      core::BrushShapeType::Circle,
      core::BlendMode::Normal,
      false,
      false,
      0,
      100,
      0,
      0,
      app::ui::TargetLayerKind::Raster,
      app::ui::CursorStyle::Brush};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "pen_default";
  subTool.displayName = "Pen";
  subTool.preset = preset;
  subTool.profile = features::common::makeProfile(preset);
  subTool.editableProperties = {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::Size};
  subTool.guide = "Basic pen tool.";
  return subTool;
}

} // namespace

core::registry::ToolEntry makePenToolEntry() {
  core::registry::ToolEntry entry;
  entry.id = kToolId;
  entry.kind = core::ToolKind::Pen;
  entry.displayName = "Pen";
  entry.factory = []() {
    return std::make_unique<core::PenTool>();
  };
  return entry;
}

app::ui::ToolDescriptor makePenToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Pen;
  descriptor.id = "pen";
  descriptor.displayName = "Pen";
  descriptor.subTools = {makePenSubTool()};
  descriptor.availableProperties = {app::ui::ToolPropertyKey::Color, app::ui::ToolPropertyKey::Size};
  descriptor.guide = "Pen tool.";
  return descriptor;
}

} // namespace features::pen
