#include "features/comic/Feature.h"

namespace features::comic {

const char* featureId() noexcept {
  return "feature.comic";
}

const char* featureDisplayName() noexcept {
  return "Comic";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::comic