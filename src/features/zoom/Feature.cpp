#include "features/zoom/Feature.h"

#include "core/registry/ToolRegistry.h"
#include "features/zoom/Descriptor.h"

namespace features::zoom {

std::string_view ZoomFeature::id() const noexcept {
  return kFeatureId;
}

void ZoomFeature::activate(features::FeatureActivationContext& context) {
  context.toolRegistry.registerTool(makeZoomToolEntry());
}

} // namespace features::zoom
