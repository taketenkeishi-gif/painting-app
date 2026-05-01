#include "features/FeatureManifest.h"

namespace features {

const std::vector<FeatureEntry>& builtInFeatureManifest() noexcept {
  static const std::vector<FeatureEntry> manifest {
      FeatureEntry {"feature.brush", true, "tool", "brush", "brush"},
      FeatureEntry {"feature.eraser", true, "tool", "eraser", "eraser"},
      FeatureEntry {"feature.eyedropper", true, "tool", "eyedropper", "eyedropper"},
      FeatureEntry {"feature.fill", true, "tool", "fill", "fill"},
      FeatureEntry {"feature.pen", true, "tool", "pen", "pen_active"},
      FeatureEntry {"feature.line", true, "tool", "line", "line"},
      FeatureEntry {"feature.rect_selection", true, "tool", "rect_selection", "select"},
      FeatureEntry {"feature.move_layer", true, "tool", "move_layer", "move"},
      FeatureEntry {"feature.hand", true, "tool", "hand", "hand"},
      FeatureEntry {"feature.zoom", true, "tool", "zoom", "zoom"}};
  return manifest;
}

} // namespace features
