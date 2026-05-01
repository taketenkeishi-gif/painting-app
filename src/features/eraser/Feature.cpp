#include "features/eraser/Feature.h"

#include "core/registry/ToolRegistry.h"
#include "features/eraser/Descriptor.h"

namespace features::eraser {

std::string_view EraserFeature::id() const noexcept {
  return kFeatureId;
}

void EraserFeature::activate(features::FeatureActivationContext& context) {
  context.toolRegistry.registerTool(makeEraserToolEntry());
}

} // namespace features::eraser
