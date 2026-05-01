#include "features/brush/Feature.h"

#include "core/registry/ToolRegistry.h"
#include "features/brush/Descriptor.h"

namespace features::brush {

std::string_view BrushFeature::id() const noexcept {
  return kFeatureId;
}

void BrushFeature::activate(features::FeatureActivationContext& context) {
  context.toolRegistry.registerTool(makeBrushToolEntry());
}

} // namespace features::brush
