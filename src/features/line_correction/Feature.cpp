#include "features/line_correction/Feature.h"

namespace features::line_correction {

const char* featureId() noexcept {
  return "feature.line_correction";
}

const char* featureDisplayName() noexcept {
  return "Line Correction";
}

bool featureEnabledByDefault() noexcept {
  return false;
}

}  // namespace features::line_correction