#include "features/fill/Descriptor.h"

#include <memory>

#include "core/tools/FillTool.h"
#include "features/common/DescriptorUtils.h"

namespace features::fill {

core::registry::ToolEntry makeFillToolEntry() {
  core::registry::ToolEntry entry;
  entry.id = kToolId;
  entry.kind = core::ToolKind::Fill;
  entry.displayName = "Fill";
  entry.factory = []() {
    return std::make_unique<core::FillTool>();
  };
  return entry;
}

app::ui::ToolDescriptor makeFillToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::Fill;
  descriptor.id = "fill";
  descriptor.displayName = "Fill";
  descriptor.subTools = {
      features::common::makeSubTool(
          "fill_contiguous",
          "Contiguous Fill",
          []() {
            app::ui::BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Raster, app::ui::CursorStyle::Fill};
            p.fillThreshold = 0;
            p.fillContiguous = true;
            p.fillReferAllLayers = false;
            p.fillGapClose = 0;
            return p;
          }(),
          {app::ui::ToolPropertyKey::FillThreshold, app::ui::ToolPropertyKey::FillContiguous, app::ui::ToolPropertyKey::FillReferAllLayers, app::ui::ToolPropertyKey::FillGapClose},
          "Click to fill connected area."),
      features::common::makeSubTool(
          "fill_gapclose",
          "Gap Close Fill",
          []() {
            app::ui::BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Raster, app::ui::CursorStyle::Fill};
            p.fillThreshold = 24;
            p.fillContiguous = true;
            p.fillReferAllLayers = true;
            p.fillGapClose = 2;
            return p;
          }(),
          {app::ui::ToolPropertyKey::FillThreshold, app::ui::ToolPropertyKey::FillContiguous, app::ui::ToolPropertyKey::FillReferAllLayers, app::ui::ToolPropertyKey::FillGapClose},
          "Fill with simple gap-close and all-layer reference.")};
  descriptor.availableProperties = {app::ui::ToolPropertyKey::FillThreshold, app::ui::ToolPropertyKey::FillContiguous, app::ui::ToolPropertyKey::FillReferAllLayers, app::ui::ToolPropertyKey::FillGapClose};
  descriptor.guide = "Fill connected pixels.";
  return descriptor;
}

} // namespace features::fill
