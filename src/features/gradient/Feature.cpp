#include "features/gradient/Feature.h"

namespace features::gradient {

const char* featureId() noexcept {
  return "feature.gradient";
}

const char* featureDisplayName() noexcept {
  return "Gradient";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::gradient