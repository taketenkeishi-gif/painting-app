#pragma once

#include "app/ui/ToolDescriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::eraser {

constexpr const char* kFeatureId = "feature.eraser";
constexpr const char* kToolId = "tool.eraser";

core::registry::ToolEntry makeEraserToolEntry();
app::ui::ToolDescriptor makeEraserToolDescriptor();

} // namespace features::eraser
