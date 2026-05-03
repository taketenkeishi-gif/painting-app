#include "features/color_mix/Feature.h"

namespace features::color_mix {

const char* featureId() noexcept {
  return "feature.color_mix";
}

const char* featureDisplayName() noexcept {
  return "Color Mix";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::color_mix