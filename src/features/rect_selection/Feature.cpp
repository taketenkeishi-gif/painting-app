#include "features/rect_selection/Feature.h"

#include "core/registry/ToolRegistry.h"
#include "features/rect_selection/Descriptor.h"

namespace features::rect_selection {

std::string_view RectSelectionFeature::id() const noexcept {
  return kFeatureId;
}

void RectSelectionFeature::activate(features::FeatureActivationContext& context) {
  context.toolRegistry.registerTool(makeRectSelectionToolEntry());
}

} // namespace features::rect_selection
