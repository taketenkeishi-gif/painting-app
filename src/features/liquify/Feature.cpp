#include "features/liquify/Feature.h"

namespace features::liquify {

const char* featureId() noexcept {
  return "feature.liquify";
}

const char* featureDisplayName() noexcept {
  return "Liquify";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::liquify