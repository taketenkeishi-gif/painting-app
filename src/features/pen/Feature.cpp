#include "features/pen/Feature.h"

#include "features/pen/Descriptor.h"
#include "core/registry/ToolRegistry.h"

namespace features::pen {

std::string_view PenFeature::id() const noexcept {
  return kFeatureId;
}

void PenFeature::activate(features::FeatureActivationContext& context) {
  context.toolRegistry.registerTool(makePenToolEntry());
}

} // namespace features::pen
