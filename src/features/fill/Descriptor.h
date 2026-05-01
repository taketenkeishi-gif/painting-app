#pragma once

#include "app/ui/ToolDescriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::fill {

constexpr const char* kFeatureId = "feature.fill";
constexpr const char* kToolId = "tool.fill";

core::registry::ToolEntry makeFillToolEntry();
app::ui::ToolDescriptor makeFillToolDescriptor();

} // namespace features::fill
