#pragma once

#include "app/ui/ToolDescriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::move_layer {

constexpr const char* kFeatureId = "feature.move_layer";
constexpr const char* kToolId = "tool.move_layer";

core::registry::ToolEntry makeMoveLayerToolEntry();
app::ui::ToolDescriptor makeMoveLayerToolDescriptor();

} // namespace features::move_layer
