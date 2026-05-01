#pragma once

#include "app/ui/ToolDescriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::hand {

constexpr const char* kFeatureId = "feature.hand";
constexpr const char* kToolId = "tool.hand";

core::registry::ToolEntry makeHandToolEntry();
app::ui::ToolDescriptor makeHandToolDescriptor();

} // namespace features::hand
