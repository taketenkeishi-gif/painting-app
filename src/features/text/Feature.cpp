#include "features/text/Feature.h"

namespace features::text {

const char* featureId() noexcept {
  return "feature.text";
}

const char* featureDisplayName() noexcept {
  return "Text";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::text