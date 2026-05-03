#include "features/operation/Feature.h"

namespace features::operation {

const char* featureId() noexcept {
  return "feature.operation";
}

const char* featureDisplayName() noexcept {
  return "Operation";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::operation