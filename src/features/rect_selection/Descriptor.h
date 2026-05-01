#pragma once

#include "app/ui/ToolDescriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::rect_selection {

constexpr const char* kFeatureId = "feature.rect_selection";
constexpr const char* kToolId = "tool.rect_selection";

core::registry::ToolEntry makeRectSelectionToolEntry();
app::ui::ToolDescriptor makeRectSelectionToolDescriptor();

} // namespace features::rect_selection
