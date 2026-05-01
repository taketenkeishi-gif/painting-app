#include "features/fill/Feature.h"

#include "core/registry/ToolRegistry.h"
#include "features/fill/Descriptor.h"

namespace features::fill {

std::string_view FillFeature::id() const noexcept {
  return kFeatureId;
}

void FillFeature::activate(features::FeatureActivationContext& context) {
  context.toolRegistry.registerTool(makeFillToolEntry());
}

} // namespace features::fill
