#include "features/ruler/Feature.h"

namespace features::ruler {

const char* featureId() noexcept {
  return "feature.ruler";
}

const char* featureDisplayName() noexcept {
  return "Ruler";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::ruler