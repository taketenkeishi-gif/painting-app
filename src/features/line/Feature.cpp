#include "features/line/Feature.h"

#include "core/registry/ToolRegistry.h"
#include "features/line/Descriptor.h"

namespace features::line {

std::string_view LineFeature::id() const noexcept {
  return kFeatureId;
}

void LineFeature::activate(features::FeatureActivationContext& context) {
  context.toolRegistry.registerTool(makeLineToolEntry());
}

} // namespace features::line
