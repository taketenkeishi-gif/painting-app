#include "features/sketch/Feature.h"

namespace features::sketch {

const char* featureId() noexcept {
  return "feature.sketch";
}

const char* featureDisplayName() noexcept {
  return "Sketch";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::sketch