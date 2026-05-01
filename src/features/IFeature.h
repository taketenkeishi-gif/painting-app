#pragma once

#include <string_view>

namespace core::registry {
class ToolRegistry;
class RendererRegistry;
} // namespace core::registry

namespace features {

struct FeatureActivationContext {
  core::registry::ToolRegistry& toolRegistry;
  core::registry::RendererRegistry& rendererRegistry;
};

class IFeature {
public:
  virtual ~IFeature() = default;

  virtual std::string_view id() const noexcept = 0;
  virtual void activate(FeatureActivationContext& context) = 0;
};

} // namespace features
