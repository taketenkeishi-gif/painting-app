#include "features/hand/Feature.h"

#include "core/registry/ToolRegistry.h"
#include "features/hand/Descriptor.h"

namespace features::hand {

std::string_view HandFeature::id() const noexcept {
  return kFeatureId;
}

void HandFeature::activate(features::FeatureActivationContext& context) {
  context.toolRegistry.registerTool(makeHandToolEntry());
}

} // namespace features::hand
