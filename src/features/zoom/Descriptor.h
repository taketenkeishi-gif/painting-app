#pragma once

#include "app/ui/ToolDescriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::zoom {

constexpr const char* kFeatureId = "feature.zoom";
constexpr const char* kToolId = "tool.zoom";

core::registry::ToolEntry makeZoomToolEntry();
app::ui::ToolDescriptor makeZoomToolDescriptor();

} // namespace features::zoom
