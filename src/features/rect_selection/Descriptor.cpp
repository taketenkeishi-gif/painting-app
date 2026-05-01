#include "features/rect_selection/Descriptor.h"

#include <memory>

#include "core/tools/RectSelectionTool.h"
#include "features/common/DescriptorUtils.h"

namespace features::rect_selection {

core::registry::ToolEntry makeRectSelectionToolEntry() {
  core::registry::ToolEntry entry;
  entry.id = kToolId;
  entry.kind = core::ToolKind::RectSelection;
  entry.displayName = "Rect Selection";
  entry.factory = []() {
    return std::make_unique<core::RectSelectionTool>();
  };
  return entry;
}

app::ui::ToolDescriptor makeRectSelectionToolDescriptor() {
  app::ui::ToolDescriptor descriptor;
  descriptor.kind = core::ToolKind::RectSelection;
  descriptor.id = "rect_selection";
  descriptor.displayName = "Rect Selection";
  descriptor.subTools = {
      features::common::makeSubTool(
          "rect_default",
          "Rectangle",
          []() {
            app::ui::BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Both, app::ui::CursorStyle::Cross};
            p.selectionMode = app::ui::SelectionMode::Rectangle;
            return p;
          }(),
          {app::ui::ToolPropertyKey::SelectionMode},
          "Drag to create rectangular selection."),
      features::common::makeSubTool(
          "lasso_default",
          "Lasso",
          []() {
            app::ui::BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Both, app::ui::CursorStyle::Cross};
            p.selectionMode = app::ui::SelectionMode::Lasso;
            return p;
          }(),
          {app::ui::ToolPropertyKey::SelectionMode},
          "Drag freehand to create lasso selection."),
      features::common::makeSubTool(
          "auto_select",
          "Auto Select",
          []() {
            app::ui::BrushPreset p {8, 100, 100, 100, 25, true, 0, false, false, core::BrushShapeType::Circle, core::BlendMode::Normal, false, false, 0, 100, 0, 0, app::ui::TargetLayerKind::Both, app::ui::CursorStyle::Cross};
            p.selectionMode = app::ui::SelectionMode::AutoSelect;
            p.autoSelectThreshold = 20;
            p.autoSelectContiguous = true;
            p.autoSelectReferAllLayers = true;
            return p;
          }(),
          {app::ui::ToolPropertyKey::SelectionMode, app::ui::ToolPropertyKey::AutoSelectThreshold, app::ui::ToolPropertyKey::AutoSelectContiguous, app::ui::ToolPropertyKey::AutoSelectReferAllLayers},
          "Click to select similar colors by threshold.")};
  descriptor.availableProperties = {app::ui::ToolPropertyKey::SelectionMode, app::ui::ToolPropertyKey::AutoSelectThreshold, app::ui::ToolPropertyKey::AutoSelectContiguous, app::ui::ToolPropertyKey::AutoSelectReferAllLayers};
  descriptor.guide = "Create rectangular selection.";
  return descriptor;
}

} // namespace features::rect_selection
