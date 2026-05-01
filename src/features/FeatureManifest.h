#pragma once

#include <string>
#include <vector>

namespace features {

struct FeatureEntry {
  std::string id;
  bool enabled {false};
  std::string category;
  std::string toolName;
  std::string icon;
};

const std::vector<FeatureEntry>& builtInFeatureManifest() noexcept;

} // namespace features
