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
      app::ui::TargetLayerKind::Both,
      app::ui::CursorStyle::Brush};

  app::ui::SubToolDescriptor subTool;
  subTool.id = "pen_default";
  subTool.displayName = "Pen";
  subTool.preset = preset;
  subTool.profile = features::common::makeProfile(preset);
  subTool.profile.shape.hardness = 100;
  subTool.profile.stroke.flow = 100;
  subTool.profile.stroke.spacing = 8;
  subTool.profile.stabilizer.stabilization = 45;
  subTool.profile.stabilizer.postCorrection = true;
  subTool.profile.vector.simplifyLevel = 25;
  subTool.editableProperties = {
      app::ui::ToolPropertyKey::Color,
      app::ui::ToolPropertyKey::Size,
      app::ui::ToolPropertyKey::Stabilization,
      app::ui::ToolPropertyKey::PostCorrection,
      app::ui::ToolPropertyKey::SimplifyLevel};
  subTool.guide = "Hard line-art pen. Uses vector paths on vector layers.";
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
  descriptor.availableProperties = {
      app::ui::ToolPropertyKey::Color,
      app::ui::ToolPropertyKey::Size,
      app::ui::ToolPropertyKey::Stabilization,
      app::ui::ToolPropertyKey::PostCorrection,
      app::ui::ToolPropertyKey::SimplifyLevel};
  descriptor.guide = "Line-art pen with hard strokes and vector-layer support.";
  return descriptor;
}

} // namespace features::pen
