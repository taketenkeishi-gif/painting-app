#include "features/move_layer/Feature.h"

#include "core/registry/ToolRegistry.h"
#include "features/move_layer/Descriptor.h"

namespace features::move_layer {

std::string_view MoveLayerFeature::id() const noexcept {
  return kFeatureId;
}

void MoveLayerFeature::activate(features::FeatureActivationContext& context) {
  context.toolRegistry.registerTool(makeMoveLayerToolEntry());
}

} // namespace features::move_layer
