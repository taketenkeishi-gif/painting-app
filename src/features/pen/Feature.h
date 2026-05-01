#pragma once

#include "features/IFeature.h"

namespace features::pen {

class PenFeature : public features::IFeature {
public:
  std::string_view id() const noexcept override;
  void activate(features::FeatureActivationContext& context) override;
};

} // namespace features::pen
