#include "features/airbrush/Feature.h"

namespace features::airbrush {

const char* featureId() noexcept {
  return "feature.airbrush";
}

const char* featureDisplayName() noexcept {
  return "Airbrush";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::airbrush