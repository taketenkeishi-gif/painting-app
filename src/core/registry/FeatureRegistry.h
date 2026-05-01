#pragma once

#include <memory>
#include <string>
#include <vector>

#include "features/FeatureManifest.h"
#include "features/IFeature.h"

namespace core::registry {

class ToolRegistry;
class RendererRegistry;

class FeatureRegistry {
public:
  FeatureRegistry();

  void activateEnabledFeatures(ToolRegistry& toolRegistry, RendererRegistry& rendererRegistry);
  bool isFeatureActive(const std::string& id) const noexcept;
  const std::vector<std::string>& activeFeatureIds() const noexcept { return m_activeFeatureIds; }

private:
  std::vector<features::FeatureEntry> m_entries;
  std::vector<std::unique_ptr<features::IFeature>> m_activeFeatures;
  std::vector<std::string> m_activeFeatureIds;
  bool m_activated {false};
};

} // namespace core::registry
