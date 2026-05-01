#pragma once

#include "app/ui/ToolDescriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::eyedropper {

constexpr const char* kFeatureId = "feature.eyedropper";
constexpr const char* kToolId = "tool.eyedropper";

core::registry::ToolEntry makeEyedropperToolEntry();
app::ui::ToolDescriptor makeEyedropperToolDescriptor();

} // namespace features::eyedropper
