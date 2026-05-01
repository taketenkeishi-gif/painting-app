#include "features/eyedropper/Feature.h"

#include "core/registry/ToolRegistry.h"
#include "features/eyedropper/Descriptor.h"

namespace features::eyedropper {

std::string_view EyedropperFeature::id() const noexcept {
  return kFeatureId;
}

void EyedropperFeature::activate(features::FeatureActivationContext& context) {
  context.toolRegistry.registerTool(makeEyedropperToolEntry());
}

} // namespace features::eyedropper
