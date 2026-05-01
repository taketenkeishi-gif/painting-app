#pragma once

#include "app/ui/ToolDescriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::line {

constexpr const char* kFeatureId = "feature.line";
constexpr const char* kToolId = "tool.line";

core::registry::ToolEntry makeLineToolEntry();
app::ui::ToolDescriptor makeLineToolDescriptor();

} // namespace features::line
