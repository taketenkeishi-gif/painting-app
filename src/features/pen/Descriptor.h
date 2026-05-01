#pragma once

#include "app/ui/ToolDescriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::pen {

constexpr const char* kFeatureId = "feature.pen";
constexpr const char* kToolId = "tool.pen";

core::registry::ToolEntry makePenToolEntry();
app::ui::ToolDescriptor makePenToolDescriptor();

} // namespace features::pen
