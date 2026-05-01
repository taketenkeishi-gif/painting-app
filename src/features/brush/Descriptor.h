#pragma once

#include "app/ui/ToolDescriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::brush {

constexpr const char* kFeatureId = "feature.brush";
constexpr const char* kToolId = "tool.brush";

core::registry::ToolEntry makeBrushToolEntry();
app::ui::ToolDescriptor makeBrushToolDescriptor();

} // namespace features::brush
