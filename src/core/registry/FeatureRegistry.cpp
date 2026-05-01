#include "core/registry/FeatureRegistry.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "core/registry/RendererRegistry.h"
#include "core/registry/ToolRegistry.h"
#include "features/brush/Feature.h"
#include "features/eraser/Feature.h"
#include "features/eyedropper/Feature.h"
#include "features/fill/Feature.h"
#include "features/hand/Feature.h"
#include "features/line/Feature.h"
#include "features/move_layer/Feature.h"
#include "features/pen/Feature.h"
#include "features/rect_selection/Feature.h"
#include "features/zoom/Feature.h"

namespace core::registry {

namespace {

std::unique_ptr<features::IFeature> createBuiltInFeatureById(const std::string& id) {
  if (id == "feature.brush") {
    return std::make_unique<features::brush::BrushFeature>();
  }
  if (id == "feature.eraser") {
    return std::make_unique<features::eraser::EraserFeature>();
  }
  if (id == "feature.eyedropper") {
    return std::make_unique<features::eyedropper::EyedropperFeature>();
  }
  if (id == "feature.fill") {
    return std::make_unique<features::fill::FillFeature>();
  }
  if (id == "feature.pen") {
    return std::make_unique<features::pen::PenFeature>();
  }
  if (id == "feature.line") {
    return std::make_unique<features::line::LineFeature>();
  }
  if (id == "feature.rect_selection") {
    return std::make_unique<features::rect_selection::RectSelectionFeature>();
  }
  if (id == "feature.move_layer") {
    return std::make_unique<features::move_layer::MoveLayerFeature>();
  }
  if (id == "feature.hand") {
    return std::make_unique<features::hand::HandFeature>();
  }
  if (id == "feature.zoom") {
    return std::make_unique<features::zoom::ZoomFeature>();
  }
  return nullptr;
}

} // namespace

FeatureRegistry::FeatureRegistry()
    : m_entries(features::builtInFeatureManifest()) {}

void FeatureRegistry::activateEnabledFeatures(ToolRegistry& toolRegistry, RendererRegistry& rendererRegistry) {
  if (m_activated) {
    return;
  }

  features::FeatureActivationContext context {toolRegistry, rendererRegistry};
  for (const features::FeatureEntry& entry : m_entries) {
    if (!entry.enabled) {
      continue;
    }
    std::unique_ptr<features::IFeature> feature = createBuiltInFeatureById(entry.id);
    if (!feature) {
      continue;
    }
    feature->activate(context);
    m_activeFeatureIds.push_back(entry.id);
    m_activeFeatures.push_back(std::move(feature));
  }

  m_activated = true;
}

bool FeatureRegistry::isFeatureActive(const std::string& id) const noexcept {
  return std::find(m_activeFeatureIds.begin(), m_activeFeatureIds.end(), id) != m_activeFeatureIds.end();
}

} // namespace core::registry
