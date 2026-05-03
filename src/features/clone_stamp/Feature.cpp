#include "features/clone_stamp/Feature.h"

namespace features::clone_stamp {

const char* featureId() noexcept {
  return "feature.clone_stamp";
}

const char* featureDisplayName() noexcept {
  return "Clone Stamp";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::clone_stamp